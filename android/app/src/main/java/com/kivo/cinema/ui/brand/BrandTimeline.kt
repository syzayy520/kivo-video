package com.kivo.cinema.ui.brand

/**
 * 312 · 品牌启动动画的**时间轴**（纯逻辑，无 Android、无 Compose、无 IO）。
 *
 * ## 为什么单独一层
 * 设计稿用一段 JS 把时间轴写死在 `render()` 里。搬到 Android 时如果直接把公式
 * 塞进绘制代码，就没法**单测**——而时间轴是最容易"差 0.1 秒就穿帮"的部分。
 * 这里把它变成纯函数：给定 t，返回该帧的所有参数。
 *
 * ## 冷启动时间轴（2.3 s，取自设计稿）
 * ```
 * 0.00 – 0.25 s  纯黑，与系统启动页同色
 * 0.25 – 0.85 s  细线在细缝位置亮起，向两侧展开
 * 0.70 – 1.40 s  细线向上下打开，三角由细缝处显影，字标浮现；音效在 0.70 s
 * 0.95 – 1.75 s  一道倾斜的柔光掠过表面
 * 1.95 – 2.30 s  淡出，主界面渐显
 * ```
 *
 * ## 缓动
 * 设计稿用了三组三次贝塞尔（CSS cubic-bezier 的写法）：
 * `eOut=bez(.22,.61,.36,1)`、`eIO=bez(.65,0,.35,1)`、`eIn=bez(.4,0,1,1)`。
 * 这里是**照搬**而不是另选，因为动效的"手感"是设计的一部分。
 */
object BrandTimeline {

    // ---------- 缓动 ----------

    /** 三次贝塞尔缓动：x 是时间、y 是进度（与 CSS cubic-bezier 同义）。 */
    fun bezier(x1: Float, y1: Float, x2: Float, y2: Float): (Float) -> Float = { t ->
        if (t <= 0f) {
            0f
        } else if (t >= 1f) {
            1f
        } else {
            // 解 x(u) = t（牛顿迭代，8 次足够，设计稿同款）
            var u = t
            var iter = 0
            while (iter < 8) {
                val f = 3f * x1 * u * (1f - u) * (1f - u) +
                    3f * x2 * u * u * (1f - u) + u * u * u - t
                val d = 3f * x1 * (1f - u) * (1f - u) +
                    6f * (x2 - x1) * u * (1f - u) + 3f * (1f - x2) * u * u
                if (kotlin.math.abs(d) < 1e-6f) break
                u -= f / d
                iter++
            }
            u = u.coerceIn(0f, 1f)
            3f * y1 * u * (1f - u) * (1f - u) + 3f * y2 * u * u * (1f - u) + u * u * u
        }
    }

    private val E_OUT: (Float) -> Float = bezier(0.22f, 0.61f, 0.36f, 1f)
    private val E_IO: (Float) -> Float = bezier(0.65f, 0f, 0.35f, 1f)
    private val E_IN: (Float) -> Float = bezier(0.4f, 0f, 1f, 1f)

    private fun seg(t: Float, a: Float, b: Float, e: (Float) -> Float): Float =
        e(((t - a) / (b - a)).coerceIn(0f, 1f))

    // ---------- 关键参数 ----------

    /** 冷启动总时长（秒）。 */
    const val COLD_END = 2.3f

    /** 热启动总时长（秒）。 */
    const val WARM_END = 0.8f

    /** 冷启动：细缝在标识框里的位置（%）。三角在上、字标在下，缝落在三角处。 */
    const val COLD_SLIT = 23.3f

    /** 热启动：只有三角，缝在正中。 */
    const val WARM_SLIT = 50f

    /** 冷启动音效时刻（秒）——设计稿 `sfx: .7`。 */
    const val COLD_SFX_AT = 0.7f

    /** 热启动音效时刻（秒）——设计稿 `sfx: .03`。 */
    const val WARM_SFX_AT = 0.03f

    /**
     * 某一帧的全部绘制参数。
     *
     * @param markAlpha 标识整体不透明度（含淡出）
     * @param fadeOut 整层（含黑底）的淡出进度：0 = 全不透明，1 = 完全透明。
     *   与 [markAlpha] 分开的原因：黑底必须一直不透明到最后一刻，
     *   否则动画前半段（标识还没淡入）主页就会透出来。
     * @param markScale 标识缩放（从 0.985 微微长到 1.0）
     * @param markBlur 模糊半径（像素），从 4 收到 0
     * @param revealTop 显影裁剪的上边界（%）；与 [revealBottom] 夹出可见带
     * @param revealBottom 显影裁剪的下边界（%）
     * @param lineScale 细线横向展开比例（0→1）
     * @param lineAlpha 细线不透明度
     * @param sheenPos 柔光位置（-0.25 → 1.25，0 = 图形左缘，1 = 右缘）
     * @param homeAlpha 主界面渐显比例
     */
    data class Frame(
        val markAlpha: Float,
        val fadeOut: Float,
        val markScale: Float,
        val markBlur: Float,
        val revealTop: Float,
        val revealBottom: Float,
        val lineScale: Float,
        val lineAlpha: Float,
        val sheenPos: Float,
        val homeAlpha: Float,
    ) {
        /** 显影是否已经完成（完全展开）。 */
        val revealed: Boolean get() = revealTop <= 0f && revealBottom <= 0f
    }

    /**
     * 冷启动帧。t 单位秒。
     *
     * 【显影的实现】设计稿用 `clipPath: inset(top% right% bottom% left%)`：
     * 令 `c = 1 - reveal`，则 `top = slit * c`、`bottom = (100 - slit) * c`。
     * `reveal=0` 时只剩细缝那一条，`reveal=1` 时上下都收到 0 ⇒ 完全展开。
     */
    fun coldAt(t: Float): Frame {
        val a = seg(t, 0.75f, 1.30f, E_OUT)   // 标识淡入
        val o = seg(t, 1.95f, 2.30f, E_IN)    // 淡出
        val hm = seg(t, 2.00f, 2.30f, E_IO)   // 主界面渐显
        val p = -0.25f + 1.5f * seg(t, 0.95f, 1.75f, E_IO)   // 柔光扫过
        val ln0 = seg(t, 0.25f, 0.85f, E_OUT)                // 细线展开
        val ln1 = seg(t, 0.25f, 0.50f, E_OUT) *
            (1f - seg(t, 0.90f, 1.30f, E_IO))                // 细线淡出
        val reveal = seg(t, 0.70f, 1.40f, E_IO)               // 显影
        val c = 1f - reveal
        return Frame(
            markAlpha = a * (1f - o),
            fadeOut = o,
            markScale = 0.985f + 0.015f * a,
            markBlur = 4f * (1f - a),
            revealTop = COLD_SLIT * c,
            revealBottom = (100f - COLD_SLIT) * c,
            lineScale = ln0,
            lineAlpha = ln1,
            sheenPos = p,
            homeAlpha = hm,
        )
    }

    /** 热启动帧：0.8 秒，三角淡入 + 一次扫光。 */
    fun warmAt(t: Float): Frame {
        val a = seg(t, 0.03f, 0.35f, E_OUT)
        val o = seg(t, 0.50f, 0.75f, E_IN)
        val hm = seg(t, 0.52f, 0.80f, E_IO)
        val p = -0.25f + 1.5f * seg(t, 0.10f, 0.55f, E_IO)
        val ln0 = seg(t, 0.25f, 0.85f, E_OUT)
        val ln1 = seg(t, 0.25f, 0.50f, E_OUT) *
            (1f - seg(t, 0.90f, 1.30f, E_IO))
        val reveal = seg(t, 0.70f, 1.40f, E_IO)
        val c = 1f - reveal
        return Frame(
            markAlpha = a * (1f - o),
            fadeOut = o,
            markScale = 0.985f + 0.015f * a,
            markBlur = 3f * (1f - a),
            revealTop = WARM_SLIT * c,
            revealBottom = (100f - WARM_SLIT) * c,
            lineScale = ln0,
            lineAlpha = ln1,
            sheenPos = p,
            homeAlpha = hm,
        )
    }
}
