package com.kivo.cinema.playback.subtitle

import java.io.ByteArrayOutputStream
import java.io.File
import java.io.IOException
import java.net.HttpURLConnection
import java.net.SocketTimeoutException
import java.net.URL
import java.net.URLDecoder
import java.security.MessageDigest

/** 下载并校验通过的远程字幕：`file` 已落在 `targetDir` 下，可直接交给导入库。 */
internal class RemoteSubtitle(val file: File, val title: String)

internal object SubtitleRemoteFetcher {

    private const val CONNECT_TIMEOUT_MS = 5_000
    private const val READ_TIMEOUT_MS = 10_000
    private const val MAX_BYTES = 8L * 1024 * 1024
    private const val SNIFF_BYTES = 512
    private const val DEFAULT_TITLE = "字幕"

    private const val MSG_SCHEME = "只支持 http/https 链接"
    private const val MSG_TOO_LARGE = "字幕文件超过 8 MB"
    private const val MSG_NOT_SUBTITLE = "链接不是字幕文件"
    private const val MSG_EMPTY = "下载到的文件是空的"

    private val ASS_MARKER = "[Script Info]".toByteArray(Charsets.ISO_8859_1)
    private val VTT_MARKER = "WEBVTT".toByteArray(Charsets.ISO_8859_1)

    /** 阻塞（网络 + 磁盘），只能在 IO 线程调用。 */
    fun fetch(url: String, targetDir: File): Result<RemoteSubtitle> {
        val parsed = try {
            URL(url.trim())
        } catch (e: Exception) {
            return fail(MSG_SCHEME)
        }
        val scheme = parsed.protocol?.lowercase()
        if (scheme != "http" && scheme != "https") return fail(MSG_SCHEME)

        val bytes = try {
            downloadBytes(parsed)
        } catch (e: FetchFailure) {
            return fail(e.userMessage)
        } catch (e: Exception) {
            return fail("下载失败：${shortReason(e)}")
        }
        if (bytes.isHtml()) return fail(MSG_NOT_SUBTITLE)

        val target = File(targetDir, "remote-${md5Prefix(bytes, 12)}.${extensionOf(bytes)}")
        try {
            ensureDir(targetDir)
            target.writeBytes(bytes)
        } catch (e: Exception) {
            return fail("下载失败：${shortReason(e)}")
        }
        if (target.length() == 0L) {
            target.delete()
            return fail(MSG_EMPTY)
        }
        return Result.success(RemoteSubtitle(target, titleOf(parsed)))
    }

    private fun downloadBytes(url: URL): ByteArray {
        val connection = url.openConnection() as? HttpURLConnection
            ?: throw FetchFailure(MSG_SCHEME)
        try {
            connection.connectTimeout = CONNECT_TIMEOUT_MS
            connection.readTimeout = READ_TIMEOUT_MS
            connection.instanceFollowRedirects = true
            connection.useCaches = false
            connection.setRequestProperty("User-Agent", "KivoCinema/1.0")
            connection.setRequestProperty("Accept-Encoding", "identity")

            val code = connection.responseCode
            if (code !in 200..299) throw FetchFailure("下载失败：HTTP $code")

            val out = ByteArrayOutputStream()
            try {
            connection.inputStream.use { input ->
                val buffer = ByteArray(16 * 1024)
                while (true) {
                    val read = input.read(buffer)
                    if (read < 0) break
                    if (read == 0) continue
                    if (out.size().toLong() + read > MAX_BYTES) throw FetchFailure(MSG_TOO_LARGE)
                    out.write(buffer, 0, read)
                }
            }
            } catch (e: IOException) {
                // 诊断：带上"声明长度/已读字节"—— 区分"连接层没通"与"读体中截断"。
                throw FetchFailure("下载失败：读到 ${out.size()} 字节后中断（${e.javaClass.simpleName}）")
            }
            return out.toByteArray()
        } finally {
            connection.disconnect()
        }
    }

    private fun ensureDir(dir: File) {
        if (!dir.isDirectory && !dir.mkdirs() && !dir.isDirectory) {
            throw IOException("无法创建下载目录")
        }
    }

    private fun ByteArray.isHtml(): Boolean {
        val head = if (size <= SNIFF_BYTES) this else copyOfRange(0, SNIFF_BYTES)
        val text = String(head, Charsets.ISO_8859_1).lowercase()
        return text.contains("<html") || text.contains("<!doctype html")
    }

    private fun extensionOf(bytes: ByteArray): String = when {
        bytes.containsAscii(ASS_MARKER) -> "ass"
        bytes.containsAscii(VTT_MARKER) -> "vtt"
        else -> "srt"
    }

    private fun ByteArray.containsAscii(needle: ByteArray): Boolean {
        if (needle.isEmpty() || needle.size > size) return false
        outer@ for (offset in 0..size - needle.size) {
            for (index in needle.indices) {
                if (this[offset + index] != needle[index]) continue@outer
            }
            return true
        }
        return false
    }

    private fun md5Prefix(bytes: ByteArray, length: Int): String {
        val digest = MessageDigest.getInstance("MD5").digest(bytes)
        val hex = StringBuilder(digest.size * 2)
        for (byte in digest) {
            hex.append(HEX_DIGITS[(byte.toInt() shr 4) and 0x0F])
            hex.append(HEX_DIGITS[byte.toInt() and 0x0F])
        }
        return hex.substring(0, length.coerceAtMost(hex.length))
    }

    private fun titleOf(url: URL): String {
        val segment = url.path.orEmpty().split('/').lastOrNull { it.isNotBlank() } ?: return DEFAULT_TITLE
        val decoded = try {
            URLDecoder.decode(segment, "UTF-8")
        } catch (e: Exception) {
            segment
        }
        return decoded.takeIf { it.isNotBlank() } ?: DEFAULT_TITLE
    }

    private fun shortReason(error: Throwable): String {
        if (error is SocketTimeoutException) return "超时"
        val message = error.message?.takeIf { it.isNotBlank() } ?: error.javaClass.simpleName
        return message.take(80)
    }

    private fun fail(message: String): Result<RemoteSubtitle> = Result.failure(IOException(message))

    private class FetchFailure(val userMessage: String) : Exception(userMessage)

    private val HEX_DIGITS = "0123456789abcdef".toCharArray()
}