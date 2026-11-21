package com.kivo.cinema.playback.subtitle

import java.nio.ByteBuffer
import java.nio.charset.CharacterCodingException
import java.nio.charset.Charset
import java.nio.charset.CodingErrorAction

/**
 * 字幕字节 → UTF-8 归一化。
 *
 * 【为什么不用 iconv】bionic 的 iconv 不认中文编码，所以这里走 Android 运行时
 * 自带的 [Charset]（ICU 提供 GB18030 / BIG5）。
 *
 * 【不变量】纯函数：不读盘、不抛异常、不返回 null。四路全失败时原样返回字节并标 `unknown`，
 * 让调用方仍能把字幕挂上（宁可文字乱码也不能丢字幕）。
 *
 * 【判定顺序】UTF-8 严格 → UTF-16（仅看 BOM）→ GB18030。严格解码，遇错即换下一路，
 * 不做"猜"。
 *
 * 【强制编码】[normalize] 的 `forcedCodec` 非空且非 `"auto"` 时**只试该编码**（用户手选：
 * BIG5 等编码的字节是 GB18030 的子集，自动顺序永远命中 GB18030，只有强制才有意义）。
 * 解不开就如实返回 `unknown`，绝不回落到自动顺序 —— 否则用户选了 BIG5 却看到 GB18030 的结果。
 */
internal object SubtitleEncoding {

    class Normalized(val utf8Bytes: ByteArray, val codec: String, val converted: Boolean)

    private const val UTF8 = "UTF-8"
    private const val UNKNOWN = "unknown"
    private const val AUTO = "auto"
    private const val UTF16LE = "UTF-16LE"
    private const val UTF16BE = "UTF-16BE"
    /**
     * 只留 GB18030。**BIG5 已删**（不是"暂不支持"的托词，是它没有意义）：
     * BIG5 双字节的首 0xA1–0xF9 ⊂ GB18030 的 0x81–0xFE、尾 0x40–0x7E/0xA1–0xFE ⊂ GB18030 的 0x40–0xFE，
     * ⇒ **每一段合法 BIG5 字节都是合法 GB18030**，严格解码永远先命中 GB18030，BIG5 分支 100% 死代码。
     * 单测 `big5BytesAreClaimedByGb18030` 把这个事实钉住了。
     * 要真正支持繁体 Big5，只能靠"用户手选编码"（导入时指定）或统计模型（uchardet 类），
     * 二者都需要独立的一批，不在这里假装支持。
     */
    private val LEGACY_CODECS = arrayOf("GB18030")

    fun normalize(bytes: ByteArray, forcedCodec: String? = null): Normalized {
        val forced = forcedCodec?.trim()?.takeIf { it.isNotEmpty() && !it.equals(AUTO, ignoreCase = true) }
        if (forced != null) {
            val text = decodeStrict(bytes, bomOffset(bytes, forced), forced)
                ?: return Normalized(bytes, UNKNOWN, false)
            return Normalized(text.toByteArray(Charsets.UTF_8), forced, true)
        }

        if (decodeStrict(bytes, 0, UTF8) != null) {
            // 已有 UTF-8（含 EF BB BF BOM）原样保留，不改写内容。
            return Normalized(bytes, UTF8, false)
        }

        if (bytes.size >= 2) {
            val littleEndian = bytes[0] == 0xFF.toByte() && bytes[1] == 0xFE.toByte()
            val bigEndian = bytes[0] == 0xFE.toByte() && bytes[1] == 0xFF.toByte()
            if (littleEndian || bigEndian) {
                val codec = if (littleEndian) UTF16LE else UTF16BE
                val text = decodeStrict(bytes, 2, codec)
                if (text != null) return normalized(text, codec)
            }
        }

        for (codec in LEGACY_CODECS) {
            val text = decodeStrict(bytes, 0, codec) ?: continue
            return normalized(text, codec)
        }

        return Normalized(bytes, UNKNOWN, false)
    }


    private fun normalized(text: String, codec: String): Normalized =
        Normalized(text.toByteArray(Charsets.UTF_8), codec, true)

    /**
     * 强制 UTF-16LE/BE 时跳过**与之匹配**的 BOM：不跳会把 U+FEFF 当成正文首字符。
     * 不匹配的 BOM（例如选了 BE 但文件是 LE BOM）不跳 —— 让它按所选编码严格解码后失败，
     * 交给调用方提示"该编码解不开"，而不是悄悄换成另一种。
     */
    private fun bomOffset(bytes: ByteArray, codec: String): Int {
        if (bytes.size < 2) return 0
        val littleEndian = bytes[0] == 0xFF.toByte() && bytes[1] == 0xFE.toByte()
        val bigEndian = bytes[0] == 0xFE.toByte() && bytes[1] == 0xFF.toByte()
        return when {
            codec.equals(UTF16LE, ignoreCase = true) && littleEndian -> 2
            codec.equals(UTF16BE, ignoreCase = true) && bigEndian -> 2
            else -> 0
        }
    }

    private fun decodeStrict(bytes: ByteArray, offset: Int, charsetName: String): String? {
        val charset = charsetFor(charsetName) ?: return null
        return try {
            charset.newDecoder()
                .onMalformedInput(CodingErrorAction.REPORT)
                .onUnmappableCharacter(CodingErrorAction.REPORT)
                .decode(ByteBuffer.wrap(bytes, offset, bytes.size - offset))
                .toString()
        } catch (e: CharacterCodingException) {
            null
        } catch (e: RuntimeException) {
            // 平台缺该字符集或解码器状态异常：按"这不是该编码"处理，继续下一路。
            null
        }
    }

    /** 面板上的显示名带连字符（`Shift-JIS`），而平台规范名是 `Shift_JIS`；两版都试。 */
    private fun charsetFor(name: String): Charset? {
        val candidates = if (name.indexOf('-') >= 0) arrayOf(name, name.replace('-', '_')) else arrayOf(name)
        for (candidate in candidates) {
            try {
                return Charset.forName(candidate)
            } catch (e: RuntimeException) {
                // 继续试下一个写法。
            }
        }
        return null
    }
}