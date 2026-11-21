package com.kivo.cinema.playback.subtitle

import android.content.ContentResolver
import android.content.Context
import android.net.Uri
import android.provider.OpenableColumns
import java.io.File
import java.security.MessageDigest

/**
 * 路径比较的**唯一口径**：mpv 的 `external-filename` 可能给 `/data/data/...`，
 * 而 `File.absolutePath` 给 `/data/user/0/...`（或经 symlink）—— 逐字符比较会静默失配，
 * 让「同文件不重挂」「多轨共用文件不删盘」双双退化成猜测。两侧都先规范化再比。
 * 规范化失败（文件不存在 / 无权限）时退回原串，不抛；空串保持空串（= 无法确认）。
 */
internal fun subtitlePathKey(path: String): String =
    if (path.isBlank()) "" else runCatching { File(path).canonicalFile.path }.getOrDefault(path)

/**
 * 导入字幕的**持久化仓库**（P9 · D/H）。
 *
 * ## 为什么不能放 cacheDir
 * 旧实现写 `cacheDir/subs`：系统在存储紧张或清理缓存时随时删除，
 * 用户"导入过的字幕"下次播放就没了 —— 这不是字幕坏了，是我们没存对地方。
 * 现在写 `filesDir/subtitles`（私有、持久、卸载才清）。
 *
 * ## 去重
 * 文件名前缀是**内容 MD5 前 12 位**：同一份字幕重复导入不会占第二份空间，
 * 也不会因为换了文件名就出现两条一模一样的外挂轨。
 *
 * ## 按片源登记
 * `subtitle_imports` 记录「片源键 → 文件路径集合」。删除外挂轨时：
 * 1. 只从**本片源**的集合里摘除；
 * 2. 只有当没有任何其他片源引用该文件时才真正删盘（去重后文件可能被共享）。
 */
internal class SubtitleImportStore(
    context: Context,
    /**
     * 取证通道。**不要用 android.util.Log**：本机 MIUI 的 logcat 恒为空（真机实测），
     * 留下读不到的日志等于没有证据。有 Activity 的调用方传 `{ smbTrace(it) }`。
     */
    private val trace: (String) -> Unit = {},
) {

    private val app = context.applicationContext
    private val dir = File(app.filesDir, DIR)
    /** 导入时的原始字节（未归一化）。库内文件是 UTF-8，重编码只能从这里出发。 */
    private val origDir = File(app.filesDir, ORIG_DIR)
    private val originals = SubtitleOriginalStore(dir, origDir, lock, trace)
    private val sp = app.getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    class Imported(val file: File, val deduped: Boolean, val title: String)

    /**
     * 把 SAF 的 content:// 落到持久目录。
     *
     * @return null = 文件读不出来（调用方必须提示，不许静默）。
     */
    fun import(resolver: ContentResolver, uri: Uri): Imported? {
        dir.mkdirs()
        val temp = File(dir, ".incoming-${System.currentTimeMillis()}")
        val copied = try {
            resolver.openInputStream(uri)?.use { input ->
                temp.outputStream().use { output -> input.copyTo(output, 64 * 1024) }
                true
            } ?: false
        } catch (e: Exception) {
            temp.delete()
            return null
        }
        if (!copied) {
            temp.delete()
            return null
        }
        val raw = normalizeInPlace(temp)
        if (raw == null) {
            temp.delete()
            return null
        }
        return commit(temp, sanitizedName(displayName(resolver, uri)), raw)
    }

    /**
     * 把一个**已落盘**的文件纳入导入库（同哈希去重/同命名规则）。
     *
     * @return null = 读不到/复制失败（调用方必须提示，不许静默）。
     */
    fun importFile(source: File, title: String): Imported? {
        if (!source.isFile) return null
        dir.mkdirs()
        val temp = File(dir, ".incoming-${System.currentTimeMillis()}")
        val copied = runCatching {
            source.inputStream().use { input ->
                temp.outputStream().use { output -> input.copyTo(output, 64 * 1024) }
            }
        }.isSuccess
        if (!copied) {
            temp.delete()
            return null
        }
        val raw = normalizeInPlace(temp)
        if (raw == null) {
            temp.delete()
            return null
        }
        return commit(temp, sanitizedName(title), raw)
    }

    /**
     * 把临时文件的内容归一化成 UTF-8 再写回 —— 落盘与内容 hash 都必须基于归一化后的字节，
     * 同一份字幕的 GBK / BIG5 / UTF-16 副本才会命中去重。
     *
     * @return 归一化**之前**的原始字节（另存 `.orig` 用）；null = 临时文件读写失败
     *         （调用方沿用既有语义返回 null）。
     */
    private fun normalizeInPlace(file: File): ByteArray? {
        val raw = runCatching { file.readBytes() }.getOrNull() ?: return null
        val normalized = SubtitleEncoding.normalize(raw)
        when {
            normalized.converted -> {
                if (runCatching { file.writeBytes(normalized.utf8Bytes) }.isFailure) return null
                trace("sub-enc converted codec=${normalized.codec} bytes=${normalized.utf8Bytes.size}")
            }
            normalized.codec == UNKNOWN_CODEC -> trace("sub-enc unknown bytes=${raw.size}")
            else -> Unit
        }
        return raw
    }

    /**
     * 临界区＝查重 → 复用/落盘。SAF 与「已落盘文件」两条路径共用，
     * 保证内容哈希、`<hash12>-<name>` 命名与去重复用完全一致。
     * 哈希计算与临时文件拷贝在锁外（不在锁内做长 IO）。
     */
    private fun commit(temp: File, name: String, original: ByteArray): Imported? {
        val hash = runCatching { md5Prefix(temp) }.getOrNull()
        if (hash == null) {
            temp.delete()
            return null
        }
        val imported = synchronized(lock) {
            // 去重按**内容哈希前缀**找，而不是按完整文件名：同一份内容换名重导时，
            // 完整名不同 ⇒ 旧实现会再落一份，面板出现两条内容相同的外挂轨。
            // 复用已落盘文件时，标题必须取**实际文件名**（去掉哈希前缀），保证标题↔文件一致。
            val existing = dir.listFiles()?.firstOrNull { it.isFile && it.name.startsWith("$hash-") }
            if (existing != null) {
                temp.delete()
                Imported(existing, deduped = true, title = existing.name.substringAfter('-'))
            } else {
                val target = File(dir, "$hash-$name")
                val moved = temp.renameTo(target) || runCatching {
                    temp.copyTo(target, overwrite = true)
                    temp.delete()
                }.isSuccess
                if (moved && target.isFile) {
                    Imported(target, deduped = false, title = name)
                } else {
                    temp.delete()
                    null
                }
            }
        } ?: return null
        originals.saveOriginal(imported.file.nameWithoutExtension, original)
        return imported
    }

    /** 库里该字幕的原始字节（导入时另存）；没有则 null。 */
    fun originalBytes(fileKey: String): ByteArray? = originals.originalBytes(fileKey)

    /**
     * 用指定编码重解**原始字节**，覆盖库内 UTF-8 文件（`codec = "auto"` 走自动探测）。
     *
     * 实现在 [SubtitleOriginalStore]（那属于"原始字节"的变化原因，不属于登记/去重）；
     * 这里只做一次包装，保持公开面不变。
     *
     * @return null = 没有原始字节 / 该编码解不开 / IO 失败（调用方必须提示）。
     */
    fun reencode(fileKey: String, codec: String): Imported? =
        originals.reencode(fileKey, codec)?.let { target ->
            Imported(target, deduped = false, title = target.name.substringAfter('-'))
        }

    /** 内容 MD5 前 12 位（与导入库文件名前缀口径一致）。 */
    private fun md5Prefix(file: File): String {
        val digest = MessageDigest.getInstance("MD5")
        file.inputStream().use { input ->
            val buffer = ByteArray(64 * 1024)
            while (true) {
                val read = input.read(buffer)
                if (read <= 0) break
                digest.update(buffer, 0, read)
            }
        }
        return digest.digest().joinToString("") { "%02x".format(it) }.take(12)
    }

    /** 登记「本片源用到了这个文件」（导入成功后调用）。 */
    fun remember(videoKey: String?, file: File) {
        val key = videoKey?.takeIf { it.isNotBlank() } ?: return
        synchronized(lock) {
            val set = sp.getStringSet(key, emptySet()).orEmpty().toMutableSet()
            if (set.add(file.absolutePath)) sp.edit().putStringSet(key, set).apply()
        }
    }

    /**
     * 按**精确文件路径**找回本片源登记的文件（P9 · H 删除外挂）。
     *
     * 【为什么不再按标题匹配】同片源可以有两条同名轨（内容不同、哈希不同），
     * 按标题删会删错另一条。路径来自内核的 `track-list/<i>/external-filename`，
     * 是这条轨的**身份**，不是猜测。
     *
     * @return 本片源登记过且仍存在的文件；路径为空或未登记返回 null。
     */
    fun findRegisteredByPath(videoKey: String?, path: String): File? {
        if (path.isBlank()) return null
        val key = videoKey?.takeIf { it.isNotBlank() } ?: return null
        // 两侧都走 subtitlePathKey：mpv 的 /data/data/… 与我们的 /data/user/0/… 视为同一文件。
        val wanted = subtitlePathKey(path)
        return sp.getStringSet(key, emptySet()).orEmpty()
            .map { File(it) }
            .firstOrNull { it.isFile && subtitlePathKey(it.path) == wanted }
    }

    /**
     * 摘除「本片源的这次导入」，必要时删盘。
     *
     * 全流程持类级锁：与 [remember]/[import] 互斥，避免「刚登记的路径被另一线程当成无人引用删掉」。
     *
     * @return true = 文件已从磁盘删除（false = 仍被其他片源引用，保留）。
     */
    fun forget(videoKey: String?, file: File): Boolean = synchronized(lock) {
        val key = videoKey?.takeIf { it.isNotBlank() }
        if (key != null) {
            val set = sp.getStringSet(key, emptySet()).orEmpty().toMutableSet()
            if (set.remove(file.absolutePath)) {
                if (set.isEmpty()) sp.edit().remove(key).apply() else sp.edit().putStringSet(key, set).apply()
            }
        }
        if (referencedByOthers(file.absolutePath, key)) return@synchronized false
        runCatching { file.isFile && file.delete() }.getOrDefault(false)
    }

    private fun referencedByOthers(path: String, exceptKey: String?): Boolean =
        sp.all.any { (key, value) ->
            key != exceptKey && (value as? Set<*>)?.contains(path) == true
        }

    private fun displayName(resolver: ContentResolver, uri: Uri): String? = runCatching {
        resolver.query(uri, null, null, null, null)?.use { cursor ->
            val index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
            if (index >= 0 && cursor.moveToFirst()) cursor.getString(index) else null
        }
    }.getOrNull()

    /** 文件名（SAF 显示名或下载链接尾部名）：去路径分隔与保留字符，限长，保留扩展名。 */
    private fun sanitizedName(raw: String?): String {
        val cleaned = raw
            ?.replace(Regex("[\\\\/:*?\"<>|\\u0000]"), "_")
            ?.trim()
            ?.takeIf { it.isNotBlank() }
            ?: "subtitle.srt"
        val dot = cleaned.lastIndexOf('.')
        val base = if (dot in 1 until cleaned.length - 1) cleaned.substring(0, dot) else cleaned
        val ext = if (dot in 1 until cleaned.length - 1) cleaned.substring(dot) else ""
        return base.take(60) + ext
    }

    private companion object {
        const val DIR = "subtitles"
        const val ORIG_DIR = "subtitles-orig"
        const val PREFS = "subtitle_imports"
        const val UNKNOWN_CODEC = "unknown"

        /**
         * 登记 / 落盘临界区互斥锁。**必须类级**：调用方每次 `SubtitleImportStore(context)`
         * 新建实例，实例锁等于没锁。本类不碰样式 prefs，不与 `SubtitleStyleStore.ioLock` 嵌套。
         */
        val lock = Any()
    }
}
