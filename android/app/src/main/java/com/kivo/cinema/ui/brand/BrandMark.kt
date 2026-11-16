package com.kivo.cinema.ui.brand

import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.PathFillType
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.vector.PathParser

/**
 * 312 · 品牌标识的**几何**（三角 + KIVO 字标）。
 *
 * ## 数据来自哪
 * 设计稿 `Kivo Video 品牌标识与启动动画.html` 里的 SVG：
 * · 播放三角：上片 `[[0,0],[69.6,39.5],[0,39.5]]`、下片 `[[0,44.5],[69.6,44.5],[0,84]]`
 *   ⇒ 中间**细缝 5**（39.5→44.5），就是启动动画里那条"光线"的位置；
 *   顶点二次贝塞尔圆角化，半径 [3,7,3]；
 * · 字标 KIVO：K/I/V 是多边形，O 是**同心圆环**（外 r36.6 / 内 r21.6，evenodd）。
 *
 * ## 为什么自己算 pathData 而不用图标那套
 * 图标用的是 **VectorDrawable**（资源里不能算），这里是**运行时绘制**（启动动画要变换、
 * 要裁剪、要扫光），所以几何要在代码里能拿到 ⇒ 在此集中定义，绘制侧只做变换。
 *
 * ## ⚠️ 圆弧不能用 pathData
 * Android 的 `PathParser` **不支持 SVG 的 A/a 弧线命令**，所以 O 用两个同心圆 +
 * [PathFillType.EvenOdd] 挖空（等价于设计稿的 `fill-rule="evenodd"`）。
 */
object BrandMark {

    // ---------------- 播放三角（原始坐标系 69.6 × 84） ----------------

    /** 上片。 */
    const val TRIANGLE_UP =
        "M0.00,3.00Q0.00,0.00 2.61,1.48L63.51,36.04Q69.60,39.50 62.60,39.50L3.00,39.50Q0.00,39.50 0.00,36.50Z"

    /** 下片。 */
    const val TRIANGLE_DOWN =
        "M0.00,47.50Q0.00,44.50 3.00,44.50L62.60,44.50Q69.60,44.50 63.51,47.96L2.61,82.52Q0.00,84.00 0.00,81.00Z"

    /** 三角的原始宽 / 高。 */
    const val TRIANGLE_W = 69.6f
    const val TRIANGLE_H = 84f

    /** 细缝中心在三角高度里的位置（39.5~44.5 ⇒ 中心 42）。 */
    const val TRIANGLE_SLIT_Y = 42f

    // ---------------- KIVO 字标（原始坐标系 255.2 × 72） ----------------

    /** K：竖笔 + 斜笔相接。 */
    const val WORD_K = "M0,0 L16,0 L16,18.22 L32.4,0 L50.4,0 L18,36 L50.4,72 L32.4,72 L16,53.78 L16,72 L0,72 Z"

    /** I。 */
    const val WORD_I = "M70,0 L86,0 L86,72 L70,72 Z"

    /** V。 */
    const val WORD_V = "M104,0 L119,0 L136,45.3 L153,0 L168,0 L141,72 L131,72 Z"

    /** O 的圆心 x / y（**y 是 36，别写成 218.6**）。 */
    const val WORD_O_OUTER = 218.6f
    const val WORD_O_CENTER_Y = 36f
    const val WORD_O_OUTER_R = 36.6f
    const val WORD_O_INNER_R = 21.6f

    /** 字标原始宽 / 高。 */
    const val WORD_W = 255.2f
    const val WORD_H = 72f

    // ---------------- 解析 ----------------

    private val parser = PathParser()

    /** 把 pathData 字符串解析成 [Path]（Android PathParser 不支持 A/a，调用方注意）。 */
    fun parse(data: String): Path = parser.parsePathString(data).toPath()

    /** 播放三角的上片。 */
    fun triangleUp(): Path = parse(TRIANGLE_UP)

    /** 播放三角的下片。 */
    fun triangleDown(): Path = parse(TRIANGLE_DOWN)

    /** 字标 O：同心圆环（EvenOdd 挖空）。 */
    fun wordO(): Path = Path().apply {
        // 【易错】O 的圆心是 (x=218.6, y=36) —— **y 用的是 36，不是 x 中心的 218.6**。
        // 写错成 218.6 会让圆跑到 y=182~255（字标只有 0~72），结果整圈被裁掉、O 消失。
        addOval(
            Rect(
                WORD_O_OUTER - WORD_O_OUTER_R,
                WORD_O_CENTER_Y - WORD_O_OUTER_R,
                WORD_O_OUTER + WORD_O_OUTER_R,
                WORD_O_CENTER_Y + WORD_O_OUTER_R,
            ),
        )
        addOval(
            Rect(
                WORD_O_OUTER - WORD_O_INNER_R,
                WORD_O_CENTER_Y - WORD_O_INNER_R,
                WORD_O_OUTER + WORD_O_INNER_R,
                WORD_O_CENTER_Y + WORD_O_INNER_R,
            ),
        )
        fillType = PathFillType.EvenOdd
    }
}
