package com.kivo.cinema.hls

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 310 · HLS master 解析与裁剪（纯函数单测）。
 *
 * ## 为什么必须单测而不是只上真机
 * 裁剪错一步的后果是**静默的**：清单少一行、档位错一个数量级，
 * 内核照样能播（播错档、或丢音轨），真机上看起来 "能播" —— 正是最坏的一类 bug。
 * 这里把三个已知坑钉死：
 * 1. `BANDWIDTH=` 是 `AVERAGE-BANDWIDTH=` 的子串（选档会偏一个数量级）；
 * 2. `CODECS="avc1.640020,mp4a.40.2"` 里的逗号不能当属性分隔符；
 * 3. 裁剪必须保留 `#EXT-X-MEDIA`（音频/字幕组），否则流被弄坏。
 */
class HlsMasterTest {

    /** 仿 Apple 官方流结构（含 AVERAGE-BANDWIDTH、CODECS 逗号、EXT-X-MEDIA）。 */
    private val appleLike = """
        #EXTM3U
        #EXT-X-VERSION:6
        #EXT-X-INDEPENDENT-SEGMENTS
        #EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="aud1",NAME="English",URI="a1/prog_index.m3u8"
        #EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=2168183,BANDWIDTH=2177116,CODECS="avc1.640020,mp4a.40.2",RESOLUTION=960x540,AUDIO="aud1"
        v5/prog_index.m3u8
        #EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=7968416,BANDWIDTH=8001098,CODECS="avc1.64002a,mp4a.40.2",RESOLUTION=1920x1080,AUDIO="aud1"
        v9/prog_index.m3u8
        #EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=3168702,BANDWIDTH=3216424,CODECS="avc1.640020,mp4a.40.2",RESOLUTION=1280x720,AUDIO="aud1"
        v6/prog_index.m3u8
    """.trimIndent()

    @Test
    fun `isMaster recognizes stream-inf`() {
        assertTrue(HlsMaster.isMaster(appleLike))
        assertFalse(HlsMaster.isMaster("#EXTM3U\n#EXTINF:4,\nseg1.ts"))
    }

    @Test
    fun `parse reads BANDWIDTH not AVERAGE-BANDWIDTH`() {
        val v = HlsMaster.parse(appleLike)
        assertEquals(3, v.size)
        // 第一档的真实峰值是 2177116，不是平均值 2168183
        assertEquals(2_177_116L, v[0].bandwidth)
        assertEquals(8_001_098L, v[1].bandwidth)
        assertEquals(3_216_424L, v[2].bandwidth)
    }

    @Test
    fun `parse reads resolution and codecs with comma`() {
        val v = HlsMaster.parse(appleLike)
        assertEquals(960, v[0].width)
        assertEquals(540, v[0].height)
        assertEquals(1920, v[1].width)
        assertEquals(1080, v[1].height)
        // CODECS 里的逗号不能被当属性分隔符
        assertEquals("avc1.640020,mp4a.40.2", v[0].codecs)
        assertEquals("avc1", v[0].videoCodec)
    }

    @Test
    fun `parse reads uris in order`() {
        val v = HlsMaster.parse(appleLike)
        assertEquals("v5/prog_index.m3u8", v[0].uri)
        assertEquals("v9/prog_index.m3u8", v[1].uri)
        assertEquals("v6/prog_index.m3u8", v[2].uri)
    }

    @Test
    fun `keepOnly keeps one variant and preserves EXT-X-MEDIA`() {
        val out = HlsMaster.keepOnly(appleLike, 1)
        // 只留 1080p 那一条
        assertTrue(out.contains("v9/prog_index.m3u8"))
        assertFalse(out.contains("v5/prog_index.m3u8"))
        assertFalse(out.contains("v6/prog_index.m3u8"))
        // 音频组声明必须保留，否则流被弄坏
        assertTrue(out.contains("#EXT-X-MEDIA:TYPE=AUDIO"))
        // 头部标签也要在
        assertTrue(out.contains("#EXTM3U"))
        assertTrue(out.contains("#EXT-X-VERSION:6"))
    }

    @Test
    fun `keepOnly with out-of-range index returns text unchanged`() {
        val out = HlsMaster.keepOnly(appleLike, 9)
        assertEquals(appleLike.lines().size, out.lines().size)
        assertTrue(out.contains("v5/prog_index.m3u8"))
    }

    @Test
    fun `keepOnly on non-master returns text unchanged`() {
        val variant = "#EXTM3U\n#EXT-X-VERSION:3\n#EXTINF:4,\nseg1.ts"
        assertEquals(variant, HlsMaster.keepOnly(variant, 0))
    }

    // ---------------- Policy ----------------

    @Test
    fun `policy picks highest affordable within budget`() {
        val c = HlsVariantPolicy.choose(HlsMaster.parse(appleLike), maxHeight = 1080, maxBps = 4_000_000)
        // 4 Mbps 预算下：960x540(2.17M) 与 720p(3.22M) 可负担，1080p(8.0M) 超 ⇒ 选 720p
        assertEquals(2, c.index)
        assertTrue(c.reason.contains("带宽内最高档"))
    }

    @Test
    fun `policy respects maxHeight gate`() {
        // 短边 720：1080p 与 540 里，1080 被屏幕挡掉 ⇒ 只能选 720p
        val c = HlsVariantPolicy.choose(HlsMaster.parse(appleLike), maxHeight = 720, maxBps = 0)
        assertEquals(2, c.index)
    }

    @Test
    fun `policy falls back to smallest when all exceed budget`() {
        val c = HlsVariantPolicy.choose(HlsMaster.parse(appleLike), maxHeight = 1080, maxBps = 100_000)
        assertEquals(0, c.index)  // 2.17 Mbps 是最小的
        assertTrue(c.reason.contains("全部超预算"))
    }

    @Test
    fun `policy refuses when decoder gate rejects everything`() {
        val c = HlsVariantPolicy.choose(
            HlsMaster.parse(appleLike), maxHeight = 1080, maxBps = 0,
            canDecode = { false },
        )
        assertEquals(-1, c.index)
        assertTrue(c.reason.contains("没有任何一档能解"))
    }

    @Test
    fun `policy skips single variant (no need to trim)`() {
        val one = """
            #EXTM3U
            #EXT-X-STREAM-INF:BANDWIDTH=1000000,RESOLUTION=640x360
            only.m3u8
        """.trimIndent()
        val c = HlsVariantPolicy.choose(HlsMaster.parse(one), 1080, 0)
        assertEquals(-1, c.index)
        assertTrue(c.reason.contains("只有一档"))
    }

    // ---------------- 真实 Apple master（6291 字节，24 档）----------------
    //
    // 【为什么用真文件】310 真机上一度只选出 720p，怀疑"解析漏档"。
    // 用真实 master 断言数量，才能区分"解析漏了"与"策略挡了"。

    private fun appleReal(): String =
        javaClass.getResource("/apple-master.m3u8")!!.readText()

    @Test
    fun `real apple master parses all 24 variants`() {
        val v = HlsMaster.parse(appleReal())
        assertEquals("Apple 官方 master 有 24 个 EXT-X-STREAM-INF", 24, v.size)
    }

    @Test
    fun `real apple master has 1080p variants above 4mbps`() {
        val v = HlsMaster.parse(appleReal())
        val hd = v.filter { it.height == 1080 }
        assertTrue("应解析出多条 1080p（aud1/aud2/aud3 三组）", hd.size >= 3)
        assertTrue("1080p 里应有 >4 Mbps 的档", hd.any { it.bandwidth > 4_000_000 })
    }

    @Test
    fun `real apple master keepOnly yields single variant`() {
        val text = appleReal()
        val v = HlsMaster.parse(text)
        val hdIndex = v.indexOfFirst { it.height == 1080 && it.bandwidth > 4_000_000 }
        val out = HlsMaster.keepOnly(text, hdIndex)
        val after = HlsMaster.parse(out)
        assertEquals("裁剪后必须只剩 1 档", 1, after.size)
        assertEquals(v[hdIndex].uri, after[0].uri)
    }
}
