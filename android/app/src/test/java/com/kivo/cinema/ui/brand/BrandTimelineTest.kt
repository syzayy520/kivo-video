package com.kivo.cinema.ui.brand

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 312 · 品牌启动动画时间轴（纯函数单测）。
 *
 * ## 为什么必须单测
 * 时间轴是设计的一部分（"差 0.1 秒就穿帮"）。而它一旦埋进绘制代码就没法验证，
 * 只能靠肉眼看动画 —— 真机验证成本极高、且不可复算。
 * 这里把设计稿的 `seg/bez` 公式固化下来，用断言钉住**每一个关键节点**。
 */
class BrandTimelineTest {

    @Test
    fun `bezier is clamped at both ends`() {
        val e = BrandTimeline.bezier(0.22f, 0.61f, 0.36f, 1f)
        assertEquals(0f, e(0f), 1e-5f)
        assertEquals(1f, e(1f), 1e-5f)
        assertEquals(0f, e(-1f), 1e-5f)
        assertEquals(1f, e(2f), 1e-5f)
    }

    @Test
    fun `bezier is monotonic increasing`() {
        val e = BrandTimeline.bezier(0.65f, 0f, 0.35f, 1f)
        var prev = -1f
        for (i in 0..100) {
            val v = e(i / 100f)
            assertTrue("缓动必须单调不减 (i=$i, prev=$prev, v=$v)", v >= prev - 1e-4f)
            prev = v
        }
    }

    // ---------------- 冷启动 ----------------

    @Test
    fun `cold starts fully black`() {
        val f = BrandTimeline.coldAt(0f)
        assertEquals("起始必须是纯黑（标识不可见）", 0f, f.markAlpha, 1e-5f)
        assertEquals("起始没有细线", 0f, f.lineScale, 1e-5f)
        assertEquals("起始主界面不可见", 0f, f.homeAlpha, 1e-5f)
    }

    @Test
    fun `cold line expands during the first second`() {
        assertEquals("0.25s 前细线没出现", 0f, BrandTimeline.coldAt(0.24f).lineScale, 1e-5f)
        val mid = BrandTimeline.coldAt(0.55f)
        assertTrue("0.55s 细线应展开到一半左右", mid.lineScale > 0.2f && mid.lineScale < 0.95f)
        assertEquals("0.85s 细线完全展开", 1f, BrandTimeline.coldAt(0.85f).lineScale, 1e-4f)
    }

    @Test
    fun `cold mark fades in after the first three quarters`() {
        assertEquals("0.7s 标识还没淡入", 0f, BrandTimeline.coldAt(0.70f).markAlpha, 1e-5f)
        val f = BrandTimeline.coldAt(1.30f)
        assertEquals("1.3s 标识应完全淡入", 1f, f.markAlpha, 1e-3f)
    }

    @Test
    fun `cold reveal opens from the middle band`() {
        // 显影区间 0.70 – 1.40
        val before = BrandTimeline.coldAt(0.70f)
        assertTrue("未显影时上边界应在缝位附近", before.revealTop > 0f)
        assertTrue("未显影时下边界也应 > 0", before.revealBottom > 0f)
        val after = BrandTimeline.coldAt(1.40f)
        assertEquals("1.4s 应完全展开", 0f, after.revealTop, 1e-4f)
        assertEquals("1.4s 应完全展开", 0f, after.revealBottom, 1e-4f)
        assertTrue(after.revealed)
    }

    @Test
    fun `cold reveal band shrinks monotonically`() {
        var prevBand = Float.MAX_VALUE
        for (i in 0..40) {
            val t = 0.70f + (1.40f - 0.70f) * i / 40f
            val f = BrandTimeline.coldAt(t)
            val band = f.revealBottom - f.revealTop
            assertTrue("可见带必须单调收缩 (t=$t, band=$band)", band <= prevBand + 1e-3f)
            prevBand = band
        }
    }

    @Test
    fun `cold sheen sweeps across the surface`() {
        assertTrue("0.95s 前柔光未入场", BrandTimeline.coldAt(0.9f).sheenPos < -0.2f)
        val mid = BrandTimeline.coldAt(1.35f)
        assertTrue("1.35s 柔光应在图形中部", mid.sheenPos > 0.1f && mid.sheenPos < 0.9f)
        assertTrue("1.75s 后柔光应扫出", BrandTimeline.coldAt(1.8f).sheenPos > 1.1f)
    }

    @Test
    fun `cold ends faded out with home visible`() {
        val f = BrandTimeline.coldAt(BrandTimeline.COLD_END)
        assertEquals("结束时标识应淡出", 0f, f.markAlpha, 1e-4f)
        assertEquals("结束时主界面应完全显现", 1f, f.homeAlpha, 1e-3f)
    }

    @Test
    fun `cold is faded out at the very end`() {
        assertTrue("1.95s 前不应开始淡出",
            BrandTimeline.coldAt(1.94f).markAlpha > 0.99f)
        assertTrue("2.1s 应在淡出中",
            BrandTimeline.coldAt(2.10f).markAlpha < 0.8f && BrandTimeline.coldAt(2.10f).markAlpha > 0f)
    }

    @Test
    fun `cold scale grows only slightly`() {
        assertEquals("0.75s 时应为 0.985", 0.985f, BrandTimeline.coldAt(0.75f).markScale, 1e-4f)
        assertEquals("1.3s 后应为 1.0", 1f, BrandTimeline.coldAt(1.3f).markScale, 1e-4f)
    }

    // ---------------- 热启动 ----------------

    @Test
    fun `warm is much shorter and simpler`() {
        assertTrue("热启动应短于冷启动", BrandTimeline.WARM_END < BrandTimeline.COLD_END)
        // 0.35s 时三角已淡入
        assertEquals(1f, BrandTimeline.warmAt(0.35f).markAlpha, 1e-3f)
        // 0.8s 结束，标识淡出
        assertEquals(0f, BrandTimeline.warmAt(BrandTimeline.WARM_END).markAlpha, 1e-4f)
    }

    @Test
    fun `warm slit is at the middle`() {
        val f = BrandTimeline.warmAt(0.35f)
        assertEquals("热启动只有三角，缝在正中", 50f, f.revealTop, 1e-4f)
        assertEquals(50f, f.revealBottom, 1e-4f)
    }

    @Test
    fun `no frame is out of range`() {
        for (kind in 0..1) {
            for (i in 0..200) {
                val t = i / 100f
                val f = if (kind == 0) BrandTimeline.coldAt(t) else BrandTimeline.warmAt(t)
                assertTrue("markAlpha 越界 t=$t", f.markAlpha in -1e-4f..1.0001f)
                assertTrue("lineScale 越界 t=$t", f.lineScale in -1e-4f..1.0001f)
                assertTrue("lineAlpha 越界 t=$t", f.lineAlpha in -1e-4f..1.0001f)
                assertTrue("homeAlpha 越界 t=$t", f.homeAlpha in -1e-4f..1.0001f)
                assertTrue("revealTop 越界 t=$t", f.revealTop in -1e-4f..100.001f)
                assertTrue("revealBottom 越界 t=$t", f.revealBottom in -1e-4f..100.001f)
                assertTrue("markBlur 越界 t=$t", f.markBlur in -1e-4f..4.001f)
                assertTrue("markScale 越界 t=$t", f.markScale in 0.98f..1.001f)
            }
        }
    }
}
