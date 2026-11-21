package com.kivo.cinema.playback.subtitle

import java.io.File

/**
 * 字幕**原始字节**的备份与重编码（W3-R1）。
 *
 * 【为什么单独成文件】库内文件在导入时已被归一化成 UTF-8，所以"用户换一个编码重看"
 * 必须从**导入时的原始字节**出发 —— 那是另一份数据、另一条变化原因，不属于"导入库的登记与去重"。
 * 本文件只做：`.orig` 的读写、以及"用指定编码重解并覆盖库内文件"。
 *
 * 【锁】与 [SubtitleImportStore] **共用同一把类级锁**（由调用方传入）：
 * 重编码会覆盖库内文件，必须与导入/登记互斥，否则会出现"刚导入的字幕被另一线程改写一半"。
 *
 * 【失败语义】一律返回 null，由调用方 toast；不抛异常、不静默降级。
 */
internal class SubtitleOriginalStore(
    private val dir: File,
    private val origDir: File,
    private val lock: Any,
    private val trace: (String) -> Unit,
) {

    /** 库里该字幕的原始字节（导入时另存）；没有则 null。 */
    fun originalBytes(fileKey: String): ByteArray? {
        val key = safeFileKey(fileKey) ?: return null
        val file = File(origDir, "$key.orig")
        if (!file.isFile) return null
        return runCatching { file.readBytes() }.getOrNull()
    }

    /** 原始字节另存为 `subtitles-orig/<fileKey>.orig`；失败只 trace，不阻断导入。 */
    fun saveOriginal(fileKey: String, bytes: ByteArray) {
        val key = safeFileKey(fileKey) ?: return
        val ok = runCatching {
            origDir.mkdirs()
            File(origDir, "$key.orig").writeBytes(bytes)
        }.isSuccess
        if (!ok) trace("sub-enc orig-fail $key")
    }

    /**
     * 用指定编码重解**原始字节**，覆盖库内 UTF-8 文件。
     *
     * `codec = "auto"` 走自动探测（与导入同一套顺序）。
     *
     * @return 被覆盖的库内文件；null = 没有原始字节 / 该编码解不开（`unknown`）/ IO 失败。
     */
    fun reencode(fileKey: String, codec: String): File? = synchronized(lock) {
        val key = safeFileKey(fileKey) ?: return@synchronized null
        val orig = File(origDir, "$key.orig")
        if (!orig.isFile) return@synchronized null
        val raw = runCatching { orig.readBytes() }.getOrNull() ?: return@synchronized null
        val normalized = SubtitleEncoding.normalize(raw, codec)
        if (normalized.codec == UNKNOWN) {
            trace("sub-enc reencode-fail $key codec=$codec")
            return@synchronized null
        }
        val target = libraryFile(key)
        if (target == null) {
            trace("sub-enc reencode-fail $key codec=$codec reason=missing")
            return@synchronized null
        }
        // 同路径覆盖：路径是这条轨的身份（面板 / 内核 external-filename 都认它），不能换名。
        val lastModified = target.lastModified()
        if (runCatching { target.writeBytes(normalized.utf8Bytes) }.isFailure) {
            return@synchronized null
        }
        runCatching { target.setLastModified(lastModified) }
        trace("sub-enc reencode $key codec=${normalized.codec} bytes=${normalized.utf8Bytes.size}")
        target
    }

    /** 库内该 fileKey 对应的文件（优先 `.srt`，兼容 `.ass`/`.ssa` 等既有扩展名）。 */
    private fun libraryFile(fileKey: String): File? {
        val exact = File(dir, "$fileKey.srt")
        if (exact.isFile) return exact
        return dir.listFiles()?.firstOrNull { it.isFile && it.nameWithoutExtension == fileKey }
    }

    /** fileKey = 库内文件名去掉扩展名；带路径分隔的键一律拒绝（防拼接穿越）。 */
    private fun safeFileKey(fileKey: String): String? =
        fileKey.trim().takeIf { it.isNotEmpty() && '/' !in it && '\\' !in it }

    private companion object {
        const val UNKNOWN = "unknown"
    }
}