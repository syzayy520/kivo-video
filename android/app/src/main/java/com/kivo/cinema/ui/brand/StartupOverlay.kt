package com.kivo.cinema.ui.brand

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import kotlinx.coroutines.withTimeoutOrNull
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.clipRect
import androidx.compose.ui.graphics.drawscope.withTransform
import androidx.compose.ui.unit.dp

/**
 * 312 · 品牌启动动画覆盖层。
 *
 * ## 它做什么
 * 在主界面之上盖一层黑底（`#050506`，与系统启动页同色），
 * 按 [BrandTimeline] 的时间轴绘制：
 * · **细线**在细缝位置横向展开（0.25–0.85 s）；
 * · **显影**：`clipRect` 从细缝处向上下打开，标识由中间"长"出来（0.70–1.40 s）；
 * · **柔光**：一道 16° 斜的白色光带扫过表面（0.95–1.75 s）；
 * · 最后淡出，主界面渐显。
 *
 * ## 与系统启动页的衔接
 * Android 12+ 必定先显示系统启动页，我们把它设成**纯背景色 + 全透明图标**
 * （见 `Theme.KivoCinema` 的 `windowSplashScreen*`），由本覆盖层接手，
 * 于是从系统页到动画是**同色无缝**，不会有跳变。
 *
 * ## 硬约束
 * **播放页（PlayerComposeActivity）绝不使用这个** —— 点开就要看片，不能等 2.3 秒。
 * 只有桌面图标启动的 MainActivity 才有。
 */
@Composable
fun StartupOverlay(
    plan: StartupPolicy.Plan,
    onFinished: () -> Unit,
    modifier: Modifier = Modifier,
) {
    var frame by remember { mutableStateOf(null as BrandTimeline.Frame?) }
    val context = androidx.compose.ui.platform.LocalContext.current
    // 音效：设计稿在 0.70 s 响（冷启动）/ 0.03 s（热启动）
    val chime = remember { StartupChime(context) }
    androidx.compose.runtime.DisposableEffect(Unit) {
        onDispose { chime.release() }
    }

    LaunchedEffect(plan) {
        if (plan == StartupPolicy.Plan.NONE) {
            onFinished()
            return@LaunchedEffect
        }
        val end = if (plan == StartupPolicy.Plan.FULL) BrandTimeline.COLD_END else BrandTimeline.WARM_END
        com.kivo.cinema.net.NetDiag.log("[brand] 启动动画开始 plan=$plan end=$end")
        var lastElapsed = 0f
        // ★超时兜底：**无论帧回调是否正常，到点一定收尾**。
        // 【为什么必须有】真机上出现过"标识只显示上半 + 那条横线永久留在界面"——
        // 根因是帧循环没走完 ⇒ onFinished 从未被调用 ⇒ 覆盖层永久停在显影中途。
        // 覆盖层卡死比动画不好看严重得多（会挡整个界面），所以宁可提前收尾。
        val sfxAt = if (plan == StartupPolicy.Plan.FULL) {
            BrandTimeline.COLD_SFX_AT
        } else {
            BrandTimeline.WARM_SFX_AT
        }
        var chimePlayed = false
        val timedOut = withTimeoutOrNull(((end + 0.6f) * 1000f).toLong()) {
            val startNanos = withFrameNanos { it }
            var elapsed = 0f
            while (elapsed < end) {
                withFrameNanos { now ->
                    elapsed = (now - startNanos) / 1_000_000_000f
                    lastElapsed = elapsed
                    // 音效：到点只响一次（设计稿 sfx：冷 0.70 s / 热 0.03 s）
                    if (!chimePlayed && elapsed >= sfxAt) {
                        chimePlayed = true
                        val played = chime.playIfAllowed()
                        com.kivo.cinema.net.NetDiag.log("[brand] 音效 played=$played（t=${elapsed}s）")
                    }
                    frame = if (plan == StartupPolicy.Plan.FULL) {
                        BrandTimeline.coldAt(elapsed)
                    } else {
                        BrandTimeline.warmAt(elapsed)
                    }
                }
            }
        }
        com.kivo.cinema.net.NetDiag.log(
            "[brand] 启动动画结束 elapsed=$lastElapsed 正常=${timedOut != null}",
        )
        onFinished()
    }

    val current = frame
    Canvas(modifier.fillMaxSize()) {
        drawStartup(current, plan)
    }
}

/** 启动页底色（与系统启动页同色）。 */
val StartupBackground = Color(0xFF050506)

/**
 * 画一帧启动动画。
 *
 * @param frame 当前帧参数；null = 还没开始（只画底色）
 */
private fun DrawScope.drawStartup(frame: BrandTimeline.Frame?, plan: StartupPolicy.Plan) {
    val f = frame
    if (f == null) {
        drawRect(StartupBackground)
        return
    }
    // 黑底全程不透明，只有最后 0.35 s 才随标识一起淡出（否则主页会提前透出来）
    drawRect(StartupBackground.copy(alpha = 1f - f.fadeOut))
    if (f.markAlpha <= 0.001f && f.lineAlpha <= 0.001f) return

    val full = plan == StartupPolicy.Plan.FULL
    // 标识整体的缩放尺寸：设计稿用容器宽度百分比（冷 58% / 热 30%），
    // 但大屏上要封顶，否则平板上会大得离谱。
    val boxW = if (full) {
        minOf(size.width * 0.58f, 300.dp.toPx())
    } else {
        minOf(size.width * 0.30f, 150.dp.toPx())
    }

    // ---- 细线（横穿细缝）----
    if (f.lineAlpha > 0.001f && f.lineScale > 0.001f) {
        val slitY = if (full) {
            size.height / 2f + (slitOffsetOfFull(boxW) )
        } else {
            size.height / 2f
        }
        val lineW = boxW * f.lineScale
        drawRect(
            color = Color.White.copy(alpha = f.lineAlpha),
            topLeft = Offset(size.width / 2f - lineW / 2f, slitY - 0.75.dp.toPx()),
            size = androidx.compose.ui.geometry.Size(lineW, 1.5.dp.toPx()),
        )
    }

    // ---- 标识（带显影裁剪 + 柔光）----
    withTransform({
        scale(f.markScale, f.markScale, Offset(size.width / 2f, size.height / 2f))
    }) {
        val markRect = brandBox(boxW, full)
        val clipTop = markRect.top + markRect.height * (f.revealTop / 100f)
        val clipBottom = markRect.top + markRect.height * (f.revealBottom / 100f)
        // ★显影完成后**必须不裁剪**：设计稿写的是 `op<1 ? inset(...) : 'none'`。
        // 若一直用 clipRect，完全展开时 top==bottom ⇒ 裁剪高度为 0 ⇒ 标识整个消失。
        if (f.revealed) {
            drawBrandMark(markRect, full)
            if (f.sheenPos > -0.2f && f.sheenPos < 1.2f) drawSheen(markRect, full, f.sheenPos)
        } else {
            clipRect(top = clipTop, bottom = clipBottom) {
                drawBrandMark(markRect, full)
                // 柔光：再画一遍标识，用带状渐变刷子，斜 16°
                if (f.sheenPos > -0.2f && f.sheenPos < 1.2f) {
                    drawSheen(markRect, full, f.sheenPos)
                }
            }
        }
    }
}

/**
 * 标识在屏幕上的矩形。
 *
 * · 冷启动：组合标识（三角在右上、KIVO 字标在左下），设计稿 viewBox `-6 -130 267.2 206`；
 * · 热启动：只有三角，viewBox `-13 -6 86 96`。
 */
private fun DrawScope.brandBox(boxW: Float, full: Boolean): Rect {
    val boxH = if (full) boxW * 206f / 267.2f else boxW * 96f / 86f
    val left = (size.width - boxW) / 2f
    val top = (size.height - boxH) / 2f
    return Rect(left, top, left + boxW, top + boxH)
}

/** 冷启动组合里，细缝相对组合框的位置（设计稿 slit=23.3%，落在三角的缝上）。 */
private fun DrawScope.slitOffsetOfFull(boxW: Float): Float {
    val boxH = boxW * 206f / 267.2f
    // 细缝在组合框 23.3% 处；组合框中心对齐屏幕中心
    return (0.233f - 0.5f) * boxH
}

/**
 * 画标识本体（冷启动 = 三角 + KIVO 字标；热启动 = 只有三角）。
 *
 * ## 变换顺序
 * Compose 的 `withTransform` 是**后乘**：`translate` 再 `scale` ⇒ 结果 `T(S(p))`，
 * 也就是**先缩放、后平移**——平移量给的是屏幕坐标。
 */
private fun DrawScope.drawBrandMark(box: Rect, full: Boolean) {
    if (full) {
        // 组合标识，设计稿 lock 的 viewBox = (-6, -130, 267.2, 206)：
        // 三角在局部 (97.6, -124)，KIVO 字标在局部 (0, 0)。
        val s = box.width / 267.2f
        withTransform({
            translate(box.left + 6f * s, box.top + 130f * s)
            scale(s, s, Offset.Zero)
        }) {
            withTransform({ translate(97.6f, -124f) }) {
                drawPath(BrandMark.triangleUp(), Titanium)
                drawPath(BrandMark.triangleDown(), Titanium)
            }
            drawPath(BrandMark.parse(BrandMark.WORD_K), Titanium)
            drawPath(BrandMark.parse(BrandMark.WORD_I), Titanium)
            drawPath(BrandMark.parse(BrandMark.WORD_V), Titanium)
            drawPath(BrandMark.wordO(), Titanium)
        }
    } else {
        // 只有三角，设计稿 mark 的 viewBox = (-13, -6, 86, 96)
        val s = box.width / 86f
        withTransform({
            translate(box.left + 13f * s, box.top + 6f * s)
            scale(s, s, Offset.Zero)
        }) {
            drawPath(BrandMark.triangleUp(), Titanium)
            drawPath(BrandMark.triangleDown(), Titanium)
        }
    }
}

/** 钛金属色（与图标一致的暖调浅灰白）。 */
private val Titanium = Color(0xFFE0DCD3)

/** 柔光：16° 斜的白色光带扫过。 */
private fun DrawScope.drawSheen(box: Rect, full: Boolean, pos: Float) {
    val w = box.width
    // 光带中心随 pos 从左缘移到右缘
    val centerX = box.left + w * pos
    val band = w * 0.18f
    val brush = Brush.linearGradient(
        colors = listOf(
            Color.Transparent,
            Color.Transparent,
            Color.White.copy(alpha = 0.42f),
            Color.Transparent,
            Color.Transparent,
        ),
        start = Offset(centerX - band, 0f),
        end = Offset(centerX + band, 0f),
    )
    withTransform({ rotate(16f, Offset(box.center.x, box.center.y)) }) {
        if (!full) {
            val scale = box.width / 86f
            withTransform({ translate(box.left + 13f * scale, box.top + 6f * scale); scale(scale, scale, Offset.Zero) }) {
                drawPath(BrandMark.triangleUp(), brush)
                drawPath(BrandMark.triangleDown(), brush)
            }
        }
    }
}
