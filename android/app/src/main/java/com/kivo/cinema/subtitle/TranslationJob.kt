package com.kivo.cinema.subtitle

import android.content.Context
import java.io.File
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean

// ============================================================================
// TranslationJob —— 一次翻译任务的驱动循环（T-B 最后一件核心）。
//
// 落位路径: android/app/src/main/java/com/kivo/cinema/subtitle/TranslationJob.kt
//
// 【驱动流程（v3.2 §3 数据流的 v1 落地）】
//   cacheKey（sha256 源字幕+解析版本+语言）→ 缓存命中即返回 →
//   nativeParseCues（C++ cue_assembler 权威装配）→ 简化切块 → 领先窗口逐块
//   translateBlock（context = 前块末 2 条）→ journal 行结果 + BLOCK_COMMITTED
//   → manifest 更新 → SRT 生成 → 完成。
//
// 【v1 偏差如实登记（decision log）】
//   1. 切块 = 简化版（间隙 2s + 25 条封块）；C++ planner 的 2000 字符/90s
//      维度与 Preview Slice 细分在 JNI 暴露后替换（planner 是权威实现）；
//   2. 重试链在 Kotlin（块 → 缺号聚合 → 逐行，语义对齐 C++ scheduler 的
//      单测口径）；C++ scheduler 的 JNI 暴露列后续；
//   3. SRT 生成在 Kotlin（格式化简单，不占 JNI 面；C++ translation_result_writer
//      的契约留给需要统一处）；
//   4. SimpleBlock/Cue 提升为文件顶层私有类 —— 局部类在 Kotlin 的类型解析
//      路径上触发过编译器怪异（first 被当函数调用，2026-11-21 实测）。
// ============================================================================

/// 块区间：cue 数组的 [first, lastExclusive)。
private data class SimpleBlock(val first: Int, val lastExclusive: Int)

/// 归一后的 cue 三元组（来自 C++ cue_assembler 的平铺数组）。
private data class JobCue(val startMs: Long, val endMs: Long, val text: String)

/// 任务进度（UI 状态行的数据源；连续已处理终点的主指标口径在 Controller 层）。
data class TranslationJobProgress(
    val state: String,
    val processedCues: Int,
    val totalCues: Int,
    val translationPath: String?,
)

/// 任务结果。
data class TranslationJobResult(
    val translationPath: String?,
    val llmTranslated: Int,
    val ruleTranslated: Int,
    val intentionalKeep: Int,
    val failedFallbacks: Int,
    val totalCues: Int,
    val cancelled: Boolean,
)

class TranslationJob(
    private val client: AiEngineClient,
    private val context: Context,
    private val onProgress: ((TranslationJobProgress) -> Unit)? = null,
) {

    val cancelled = AtomicBoolean(false)

    /// 取消任务（停止 = 关闭中文字幕的底层动作；已译内容保留可续）。
    fun cancel() {
        cancelled.set(true)
        client.cancel()
    }

    /// 执行翻译任务。**阻塞调用** —— 调用方放独立线程（与 importSubtitleFile
    /// 的 Thread{} 模式一致；单翻译线程由 AiEngineClient 的 executor 保证）。
    fun run(sourceSrt: String, playheadMs: Long): TranslationJobResult {
        val cacheKey = TranslationCache.cacheKey(sourceSrt)
        android.util.Log.i("kivo-ai", "job: cacheKey computed")
        val sourceHash = TranslationCache.sha256Hex(sourceSrt)

        // ---- 缓存命中：秒级返回（二次观看，评审 E24/G11）----
        android.util.Log.i("kivo-ai", "job: cache checking")
        if (TranslationCache.hasTranslation(context, cacheKey)) {
            return TranslationJobResult(
                TranslationCache.translationFile(context, cacheKey).absolutePath,
                0, 0, 0, 0, 0, cancelled = false,
            )
        }

        // ---- 装配（C++ cue_assembler 权威实现，JNI 平铺）----
        android.util.Log.i("kivo-ai", "job: cache missed, parsing cues")
        val rows = if (AiTranslationNative.available) {
            AiTranslationNative.nativeParseCues(sourceSrt)
        } else {
            null
        }
        if (rows == null) { return failResult("engine unavailable") }

        val cues = rows.mapNotNull { row ->
            val parts = row.split('\t')
            if (parts.size < 3) { return@mapNotNull null }
            val start = parts[0].toLongOrNull() ?: return@mapNotNull null
            val end = parts[1].toLongOrNull() ?: return@mapNotNull null
            JobCue(start, end, parts[2])
        }
        if (cues.isEmpty()) { return failResult("no cues parsed") }

        // ---- 状态机：启动 + 准备完成（journal 事件由 reducer 产出）----
        var output = reduceTranslation(TranslationStateMachineState(), TranslationEvent.StartRequested)
        var state = output.state
        val journal = TranslationJournal(translationDir(), cacheKey)
        // 续译场景：seq 从既有 journal 最大值接续（否则重跑的 append 从 1 起，
        // 与旧记录 seq 重叠 —— 回放的截断语义会把新记录丢掉）。
        journal.primeSeqFromDisk()
        journal.appendEvent(output.journalEvent ?: TranslationJournalEvents.JOB_CREATED)

        output = reduceTranslation(state, TranslationEvent.PreparationDone(cues.size))
        state = output.state
        writeManifest(state, cacheKey, sourceHash, cues.size)

        // ---- 简化切块（间隙 2s 断 + 25 条封块；偏差登记见文件头）----
        val blocks = mutableListOf<SimpleBlock>()
        var blockFirst = 0
        for (index in 1 until cues.size) {
            val gap = cues[index].startMs - cues[index - 1].endMs
            if (gap > 2000 || index - blockFirst >= 25) {
                blocks.add(SimpleBlock(blockFirst, index))
                blockFirst = index
            }
        }
        blocks.add(SimpleBlock(blockFirst, cues.size))

        // ---- 领先窗口逐块（播放头所在块起 → 后续块 → 回填；简化 pick）----
        val orderedBlocks = orderBlocksForPlayhead(blocks, cues, playheadMs)

        android.util.Log.i("kivo-ai", "job: blocks=${blocks.size} cues=${cues.size}")
        val translations = arrayOfNulls<String>(cues.size)
        var llmCount = 0
        var failedCount = 0

        for (block in orderedBlocks) {
            if (cancelled.get()) { break }

            // context = 前块末 2 条原文（DEC-14：只带原文进 user 段，N1）。
            val contextStart = if (block.first >= 2) block.first - 2 else 0
            val prevContext = cues.subList(contextStart, block.first).map { it.text }

            val request = AiTranslateRequest(
                sourceLines = cues.subList(block.first, block.lastExclusive).map { it.text },
                useFewshot = true,
                contextLines = prevContext,
            )
            var response: AiTranslateResponse? = null
            // 【latch 同步】translateBlock 是异步回调（DEC-13 接口）——
            // 不等待就读 response 是竞态（回调几乎必然未执行，实测恒 null）。
            android.util.Log.i("kivo-ai", "job: block=${block.first} submitting")
            val latch = CountDownLatch(1)
            client.translateBlock(request) { result ->
                response = result.getOrNull()
                latch.countDown()
            }
            latch.await(15, TimeUnit.MINUTES)
            android.util.Log.i("kivo-ai", "job: block=${block.first} latch released")
            val slots = response?.slots?.toMutableList()
            if (slots == null) {
                // 引擎未就绪/整批失败 → 任务失败。
                output = reduceTranslation(state, TranslationEvent.Failed("engine not ready"))
                state = output.state
                journal.appendEvent(TranslationJournalEvents.JOB_FAILED)
                journal.flush()
                writeManifest(state, cacheKey, sourceHash, cues.size, failedCount)
                return failResult(state.lastError ?: "engine not ready")
            }

            // 缺号聚合重试（重试不变量：缺失行重组小编号块 —— 评审 E29）。
            val missing = mutableListOf<Int>()
            for (offset in slots.indices) {
                if (slots[offset].isEmpty()) { missing.add(block.first + offset) }
            }
            if (missing.isNotEmpty()) {
                val retryRequest = AiTranslateRequest(
                    sourceLines = missing.map { cues[it].text },
                    useFewshot = false,
                    contextLines = prevContext,
                )
                var retryResponse: AiTranslateResponse? = null
                val retryLatch = CountDownLatch(1)
                client.translateBlock(retryRequest) { result ->
                    retryResponse = result.getOrNull()
                    retryLatch.countDown()
                }
                retryLatch.await(15, TimeUnit.MINUTES)
                val retrySlots = retryResponse?.slots
                if (retrySlots != null) {
                    for ((offset, cueIndex) in missing.withIndex()) {
                        if (offset < retrySlots.size && retrySlots[offset].isNotEmpty()) {
                            slots[cueIndex - block.first] = retrySlots[offset]
                        }
                    }
                }
            }

            // 逐 cue 结果 + journal 行结果（五结果状态；失败行回退原文 DEC-05）。
            for (index in block.first until block.lastExclusive) {
                val text = slots[index - block.first]
                val display: String
                val status: TranslationLineStatus
                if (text.isNotEmpty()) {
                    status = TranslationLineStatus.LLM_TRANSLATED
                    display = text
                    ++llmCount
                } else {
                    status = TranslationLineStatus.FAILED_FALLBACK
                    display = cues[index].text
                    ++failedCount
                }
                translations[index] = display
                journal.appendLineResult(index, status, display)
            }
            journal.flush()   // 块检查点 fsync（评审 C5）

            val processed = countProcessed(translations)
            output = reduceTranslation(
                state,
                TranslationEvent.BlockCommitted(doneCues = processed, totalCues = cues.size),
            )
            state = output.state
            if (output.journalEvent != null) { journal.appendEvent(output.journalEvent) }
            writeManifest(state, cacheKey, sourceHash, cues.size, failedCount)
            onProgress?.invoke(
                TranslationJobProgress(state.taskState.name, processed, cues.size, null),
            )
        }

        // ---- SRT 生成（journal 派生的最终文本；失败行显示原文，DEC-05）----
        val translationPath: String
        if (cancelled.get()) {
            output = reduceTranslation(state, TranslationEvent.StopRequested)
            state = output.state
            journal.appendEvent(TranslationJournalEvents.JOB_STOPPED)
            translationPath = writeSrt(cues, translations, cacheKey)
        } else {
            output = reduceTranslation(state, TranslationEvent.Completed(failedCount))
            state = output.state
            journal.appendEvent(TranslationJournalEvents.JOB_COMPLETED)
            translationPath = writeSrt(cues, translations, cacheKey)
        }
        writeManifest(state, cacheKey, sourceHash, cues.size, failedCount)
        journal.flush()

        onProgress?.invoke(TranslationJobProgress(state.taskState.name, cues.size, cues.size, translationPath))
        return TranslationJobResult(
            translationPath,
            llmCount, 0, 0, failedCount, cues.size,
            cancelled = cancelled.get(),
        )
    }

    // ---- 内部工具 ----

    private fun countProcessed(translations: Array<String?>): Int =
        translations.count { it != null }

    private fun translationDir() = File(context.filesDir, "ai_subs")

    private fun failResult(reason: String): TranslationJobResult {
        onProgress?.invoke(TranslationJobProgress("FAILED", 0, 0, null))
        return TranslationJobResult(null, 0, 0, 0, 0, 0, cancelled = false)
    }

    private fun writeManifest(
        state: TranslationStateMachineState,
        cacheKey: String,
        sourceHash: String,
        totalCues: Int,
        failedFallbacks: Int = 0,
    ) {
        TranslationJournal.writeManifest(
            translationDir(), cacheKey,
            TranslationManifest(
                taskState = state.taskState.name,
                totalCues = totalCues,
                cacheKey = cacheKey,
                sourceHash = sourceHash,
                failedFallbacks = failedFallbacks,
            ),
        )
    }

    private fun writeSrt(
        cues: List<JobCue>,
        translations: Array<String?>,
        cacheKey: String,
    ): String {
        val file = TranslationCache.translationFile(context, cacheKey)
        file.bufferedWriter(Charsets.UTF_8).use { writer ->
            var sequence = 1
            for ((index, cue) in cues.withIndex()) {
                val text = translations[index] ?: continue
                writer.write(sequence.toString() + "\n")
                writer.write(formatTimestamp(cue.startMs) + " --> " + formatTimestamp(cue.endMs) + "\n")
                writer.write(text + "\n\n")
                ++sequence
            }
        }
        return file.absolutePath
    }

    private fun formatTimestamp(ms: Long): String {
        val totalSec = ms / 1000
        val millis = ms % 1000
        val hour = totalSec / 3600
        val minute = (totalSec % 3600) / 60
        val second = totalSec % 60
        return "%02d:%02d:%02d,%03d".format(hour, minute, second, millis)
    }

    /// 领先窗口的块排序（简化 pick，对齐 C++ pick 的三段语义）：
    /// 播放头所在/之后的块按时间序 → 播放头之前的块（回填）。
    private fun orderBlocksForPlayhead(
        blocks: List<SimpleBlock>,
        cues: List<JobCue>,
        playheadMs: Long,
    ): List<SimpleBlock> {
        val containing = blocks.filter {
            playheadMs >= cues[it.first].startMs && playheadMs < cues[it.lastExclusive - 1].endMs
        }
        val picked = containing.toSet()
        val after = blocks.filter { it !in picked && cues[it.first].startMs >= playheadMs }
            .sortedBy { cues[it.first].startMs }
        val before = blocks.filter { it !in picked && it !in after.toSet() }
            .sortedBy { cues[it.first].startMs }
        return containing + after + before
    }
}