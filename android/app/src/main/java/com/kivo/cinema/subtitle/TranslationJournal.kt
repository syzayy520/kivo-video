package com.kivo.cinema.subtitle

import java.io.File
import java.io.FileOutputStream

// ============================================================================
// TranslationJournal —— 任务进度与意图的持久化（DEC-08 v2 分工）。
//
// 落位路径: android/app/src/main/java/com/kivo/cinema/subtitle/TranslationJournal.kt
//
// 【真源分工（评审 F4）】
//   - **journal = 进度真源**：行结果逐条追加（append-only），SRT 由它派生；
//   - **manifest = 意图真源**：任务态/缓存键/源 hash——行结果流水重建不了
//     「手动暂停/已停止」（控制意图不在行记录里）；
//   - 恢复规则：manifest 说运行中但进程已死 → 任务态按「已中断」处理，
//     **不悄悄自动续译**（评审 F4）。
//
// 【格式（自定协议，评审 C5/E7 的字段清单）】
//   journal.jsonl 每行一个记录，Tab 分隔；text 经 escape（\t \n \\ → \uXXXX
//   类转义）保证单行可解析；seq 单调递增，跳号/解析失败 → 回放截断到最后有效行。
//     L\t<seq>\t<cueId>\t<status>\t<escapedText>     行结果
//     E\t<seq>\t<type>                                状态机事件（十类）
//   manifest.json 时代理：key=value 行（STATE/TOTAL/CACHE_KEY/SOURCE_HASH/FAILED），
//   **tmp + rename 原子写**（评审 B15：不许留半截文件）。
//
// 【fsync】块检查点后 flush() 走 fd.sync() —— 应用层 flush 不必然落盘（评审 C5）。
//
// 【无 JSON 依赖】org.json 在 JVM 单测下是 stub（Android 单测的已知坑），
//   本文件手写转义/解析 —— 既是依赖约束也是可测性。
// ============================================================================

/// 行结果（五结果状态，评审 G13——状态串与 C++ scheduler 的 journal 对齐）。
enum class TranslationLineStatus { LLM_TRANSLATED, RULE_TRANSLATED, INTENTIONAL_KEEP, FAILED_FALLBACK, PENDING }

/// 单条行结果的回放形态。
data class TranslationJournalLine(
    val cueId: Int,
    val status: TranslationLineStatus,
    val text: String,
)

/// 回放产物：行结果（cueId → 最近一次记录为准，评审 E7 幂等）+ 事件清单。
data class TranslationJournalReplay(
    val lineResults: Map<Int, TranslationJournalLine>,
    val events: List<String>,
    val lastSeq: Long,
)

/// manifest 的意图态（评审 F4：**意图真源**——独立 fsync，不依赖 journal 回放）。
data class TranslationManifest(
    val taskState: String,
    val totalCues: Int,
    val cacheKey: String,
    val sourceHash: String,
    val failedFallbacks: Int,
)

class TranslationJournal(private val dir: File, private val jobId: String) {

    private val journalFile = File(dir, "$jobId.journal")
    private val manifestFile = File(dir, "$jobId.manifest")
    private var seq = 0L
    private var out: FileOutputStream? = null

    private fun openForAppend(): FileOutputStream {
        val existing = out
        if (existing != null) { return existing }
        dir.mkdirs()
        val stream = FileOutputStream(journalFile, true)
        out = stream
        return stream
    }

    /// 读取既有 journal 的最大 seq（续写续译场景）。
    fun primeSeqFromDisk() {
        seq = replay().lastSeq
    }

    /// 追加行结果（不主动 fsync —— 由块检查点统一 flush）。
    fun appendLineResult(cueId: Int, status: TranslationLineStatus, text: String) {
        seq += 1
        val stream = openForAppend()
        val line = "L\t$seq\t$cueId\t${status.name}\t${escape(text)}\n"
        stream.write(line.toByteArray(Charsets.UTF_8))
    }

    /// 追加状态机事件。
    fun appendEvent(type: String) {
        seq += 1
        val stream = openForAppend()
        val line = "E\t$seq\t$type\n"
        stream.write(line.toByteArray(Charsets.UTF_8))
    }

    /// 块检查点：fsync 保 journal 真正落盘（应用层 write 不必然到盘）。
    fun flush() {
        out?.fd?.sync()
    }

    /// 回放：行结果（cueId → 最近记录）+ 事件 + 最大 seq。
    /// 解析失败的行（截断/损坏）→ 停止回放，保留之前的全部有效记录。
    fun replay(): TranslationJournalReplay {
        val lines = mutableMapOf<Int, TranslationJournalLine>()
        val events = mutableListOf<String>()
        var maxSeq = 0L
        if (!journalFile.isFile) {
            return TranslationJournalReplay(lines, events, maxSeq)
        }
        journalFile.useLines { fileLines ->
            for (raw in fileLines) {
                if (raw.isBlank()) { continue }
                val parts = raw.split('\t')
                val seqText = parts.getOrNull(1)?.toLongOrNull()
                if (parts.size < 3 || seqText == null || seqText <= maxSeq) {
                    break   // 损坏/截断/跳号 → 截断到最后有效行
                }
                when (parts[0]) {
                    "L" -> {
                        if (parts.size < 5) { break }
                        val cueId = parts[2].toIntOrNull() ?: break
                        val status = runCatching {
                            TranslationLineStatus.valueOf(parts[3])
                        }.getOrNull() ?: break
                        lines[cueId] =
                            TranslationJournalLine(cueId, status, unescape(parts[4]))
                        // seq 更新在记录解析成功之后 —— 半行的 seqText 不计入
                        //（评审 C5：损坏截断的最后半行不留指纹）。
                        maxSeq = seqText
                    }
                    "E" -> {
                        events.add(parts.getOrNull(2) ?: continue)
                        maxSeq = seqText
                    }
                    else -> break   // 未知记录类型（版本不匹配）→ 截断
                }
            }
        }
        seq = maxSeq
        return TranslationJournalReplay(lines, events, maxSeq)
    }

    companion object {

        // 转义：\t \n \r \\ 与控制字符 → \uXXXX —— 保证单行可 split 解析。
        fun escape(text: String): String = buildString {
            for (ch in text) {
                when (ch) {
                    '\t' -> append("\\t")
                    '\n' -> append("\\n")
                    '\r' -> append("\\r")
                    '\\' -> append("\\\\")
                    else ->
                        if (ch.code < 0x20) { append("\\u%04x".format(ch.code)) }
                        else { append(ch) }
                }
            }
        }

        fun unescape(text: String): String = buildString {
            var index = 0
            while (index < text.length) {
                val ch = text[index]
                if (ch == '\\' && index + 1 < text.length) {
                    when (val next = text[index + 1]) {
                        't' -> { append('\t'); index += 2 }
                        'n' -> { append('\n'); index += 2 }
                        'r' -> { append('\r'); index += 2 }
                        '\\' -> { append('\\'); index += 2 }
                        'u' -> {
                            if (index + 5 < text.length) {
                                append(text.substring(index + 2, index + 6).toInt(16).toChar())
                                index += 6
                            } else { append(ch); index += 1 }
                        }
                        else -> { append(ch); index += 1 }
                    }
                } else {
                    append(ch)
                    index += 1
                }
            }
        }

        // ---- manifest：意图真源（tmp + rename 原子写，评审 B15）----

        fun manifestFileName(jobId: String) = "$jobId.manifest"

        fun writeManifest(dir: File, jobId: String, manifest: TranslationManifest) {
            dir.mkdirs()
            val body = buildString {
                append("STATE=").append(manifest.taskState).append('\n')
                append("TOTAL=").append(manifest.totalCues).append('\n')
                append("CACHE_KEY=").append(manifest.cacheKey).append('\n')
                append("SOURCE_HASH=").append(manifest.sourceHash).append('\n')
                append("FAILED=").append(manifest.failedFallbacks).append('\n')
            }
            val tmp = File(dir, "${manifestFileName(jobId)}.tmp")
            val final = File(dir, manifestFileName(jobId))
            FileOutputStream(tmp).use { stream ->
                stream.write(body.toByteArray(Charsets.UTF_8))
                stream.fd.sync()   // manifest 是意图真源 —— 独立 fsync（评审 F4）
            }
            if (!tmp.renameTo(final)) {
                tmp.delete()
                final.writeText(body, Charsets.UTF_8)   // rename 失败兜底（同目录极罕见）
            }
        }

        fun readManifest(dir: File, jobId: String): TranslationManifest? {
            val file = File(dir, manifestFileName(jobId))
            if (!file.isFile) { return null }
            val fields = mutableMapOf<String, String>()
            file.useLines { fileLines ->
                for (line in fileLines) {
                    val eq = line.indexOf('=')
                    if (eq > 0) { fields[line.substring(0, eq)] = line.substring(eq + 1) }
                }
            }
            val state = fields["STATE"] ?: return null
            return TranslationManifest(
                taskState = state,
                totalCues = fields["TOTAL"]?.toIntOrNull() ?: 0,
                cacheKey = fields["CACHE_KEY"] ?: "",
                sourceHash = fields["SOURCE_HASH"] ?: "",
                failedFallbacks = fields["FAILED"]?.toIntOrNull() ?: 0,
            )
        }
    }
}