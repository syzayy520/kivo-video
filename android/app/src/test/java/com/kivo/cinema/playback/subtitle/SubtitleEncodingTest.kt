package com.kivo.cinema.playback.subtitle

import java.nio.charset.Charset
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * `SubtitleEncoding` 的探测顺序与不变量。
 *
 * 这些用例的目的不是"跑绿"，而是**钉死行为**：
 * ① 严格 UTF-8 失败才会往下试；
 * ② 四种都不成立时**原样返回**（宁乱码不丢字幕）；
 * ③ 一个**设计事实**：BIG5 与 GB18030 在"严格解码"下几乎不可区分 ——
 *    用例 [big5BytesAreClaimedByGb18030] 把它固化成可执行的事实，
 *    避免以后有人以为"支持 BIG5"其实永远轮不到。
 */
class SubtitleEncodingTest {

    private fun bytesOf(text: String, charset: String): ByteArray =
        text.toByteArray(Charset.forName(charset))

    private fun decodeUtf8(b: ByteArray): String = String(b, Charset.forName("UTF-8"))

    @Test
    fun asciiIsUtf8Unchanged() {
        val src = bytesOf("1\n00:00:00,000 --> 00:00:40,000\nhello\n", "UTF-8")
        val r = SubtitleEncoding.normalize(src)
        assertEquals("UTF-8", r.codec)
        assertFalse(r.converted)
        assertArrayEquals(src, r.utf8Bytes)
    }

    @Test
    fun chineseUtf8Unchanged() {
        val src = bytesOf("中文字幕", "UTF-8")
        val r = SubtitleEncoding.normalize(src)
        assertEquals("UTF-8", r.codec)
        assertFalse(r.converted)
        assertArrayEquals(src, r.utf8Bytes)
    }

    @Test
    fun utf8BomIsPreserved() {
        val bom = byteArrayOf(0xEF.toByte(), 0xBB.toByte(), 0xBF.toByte())
        val src = bom + bytesOf("中文字幕", "UTF-8")
        val r = SubtitleEncoding.normalize(src)
        assertEquals("UTF-8", r.codec)
        assertFalse(r.converted)
        // BOM 原样保留：不替用户改写文件头。
        assertArrayEquals(src, r.utf8Bytes)
    }

    @Test
    fun utf16LittleEndianIsConverted() {
        val src = byteArrayOf(0xFF.toByte(), 0xFE.toByte()) + bytesOf("中文字幕", "UTF-16LE")
        val r = SubtitleEncoding.normalize(src)
        assertEquals("UTF-16LE", r.codec)
        assertTrue(r.converted)
        assertEquals("中文字幕", decodeUtf8(r.utf8Bytes))
    }

    @Test
    fun utf16BigEndianIsConverted() {
        val src = byteArrayOf(0xFE.toByte(), 0xFF.toByte()) + bytesOf("中文字幕", "UTF-16BE")
        val r = SubtitleEncoding.normalize(src)
        assertEquals("UTF-16BE", r.codec)
        assertTrue(r.converted)
        assertEquals("中文字幕", decodeUtf8(r.utf8Bytes))
    }

    @Test
    fun gb18030IsConverted() {
        val src = bytesOf("中文字幕 GBK 转码测试", "GB18030")
        val r = SubtitleEncoding.normalize(src)
        assertEquals("GB18030", r.codec)
        assertTrue(r.converted)
        assertEquals("中文字幕 GBK 转码测试", decodeUtf8(r.utf8Bytes))
    }

    /**
     * **设计事实**：BIG5 字节在 GB18030 的严格解码下通常是**合法**的，
     * 所以判定顺序里 BIG5 排在 GB18030 之后 ⇒ **永远轮不到**。
     *
     * 本用例锁住这个事实（断言 codec == "GB18030"），
     * 让"支持 BIG5"这句话不能靠注释假装成立；要真支持必须引入统计模型（如 uchardet）。
     */
    @Test
    fun big5BytesAreClaimedByGb18030() {
        val src = bytesOf("中文字幕測試", "BIG5")
        val r = SubtitleEncoding.normalize(src)
        assertEquals("GB18030", r.codec)
    }

    @Test
    fun illegalBytesStayUntouched() {
        // 0xFF 不是 GB18030 的合法首字节（0x81–0xFE），四路全失败 ⇒ 原样返回 + unknown。
        val src = byteArrayOf(0xFF.toByte(), 0xFF.toByte(), 0xFF.toByte())
        val r = SubtitleEncoding.normalize(src)
        assertEquals("unknown", r.codec)
        assertFalse(r.converted)
        assertArrayEquals(src, r.utf8Bytes)
    }

    @Test
    fun emptyIsUtf8() {
        val r = SubtitleEncoding.normalize(ByteArray(0))
        assertEquals("UTF-8", r.codec)
        assertFalse(r.converted)
        assertEquals(0, r.utf8Bytes.size)
    }
}