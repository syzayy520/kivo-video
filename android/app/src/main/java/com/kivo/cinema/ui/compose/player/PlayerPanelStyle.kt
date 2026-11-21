package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.core.CubicBezierEasing
import androidx.compose.animation.core.Easing
import androidx.compose.animation.core.FastOutSlowInEasing
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

/**
 * P10 面板材质 token（**面板族唯一数值源**，`docs/design/subtitle-plane/p10-panel-material-spec.md`）。
 *
 * 【为什么不再用 [PlayerTokens.panelSurface]】那块黑 75% 是本轮要消灭的"实心黑板"；
 * 播放页其余几何（轨道 6pt、图标 pitch 50、面板宽 248 / 微调卡 357）仍在
 * [PlayerTokens]，不搬过来 —— 本文件的边界只有"面板内部的材质/字/距/动效"。
 *
 * 【后续调材质的唯一入口】规范 §1：对照结论只允许改本文件 + [playerPanelSurface]。
 */
internal object PlayerPanelStyle {

    // ---------- 材质（§3，v2 亮面烟熏玻璃）----------
    /**
     * 面板底：冷灰蓝 `#151B23`，上 62% → 下 65% 不透明。
     *
     * 【v3 为什么改成浅色】2026-11-21 用户判定"质感不如 SenPlayer"，并拍板照搬：
     * 对方卡片实测是**白 ≈40%**（深底 30 上合成 118 灰），白字压在灰玻璃上。
     * 我方 v2 的 #151B23 62% 在暗画面合成仅 (37,40,45) —— 与背景几乎同值，
     * 这才是"没有质感"的根因。v3 取白 40% → 37%，深底合成 ≈120，与对方逐位对齐。
     * ⚠️ 对比度判据已按用户拍板撤销（spec §16.1）：白字压中灰只有 ≈2.1:1，与对方同量级。
     */
    val surfaceTop = Color(0x66FFFFFF)
    val surfaceBottom = Color(0x5FFFFFFF)

    /**
     * 顶部高光：白 6%，从顶边衰减到 [sheenRatio] × 面板高。
     * v1 的 3%/24dp 在观影亮度下看不见；6% 且按比例铺开，暗场里才有一道"受光面"。
     */
    val topSheen = Color(0x14FFFFFF)
    const val sheenRatio = 0.45f

    /**
     * 边缘体系（v2 新增"整圈内描边 + 底缘暗线"）：
     * · [edgeLine] 整圈 1dp 白 10% —— 只给轮廓，不再有"贴纸"感（内描边，圆角自然收）；
     * · [edgeTop] 顶边 1dp 白 16% —— 受光边，左右各 [edgeFade] 渐隐；
     * · [edgeBottom] 底缘 1dp 黑 30% —— 厚度感，夜景里靠它把板与背景切开。
     */
    val edgeLine = Color(0x24FFFFFF)
    val edgeTop = Color(0x52FFFFFF)
    val edgeBottom = Color(0x22000000)
    val edgeFade = 24.dp

    /** 页眉带：标题区白 2.5% 渐隐到 0（只是"光"，不是分区线，滚动时无需跟随）。 */
    val headerWash = Color(0x0A000000)

    /** v3：圆角 13.5（SenPlayer 实测 36–38px @2.75）；阴影 24 / 黑 35%（浅卡片阴影过重会发脏）。 */
    val radius = 13.5.dp
    val shadow = 24.dp
    val shadowColor = Color(0x59000000)

    /** 品牌色：与投屏面板/开关一致（iOS 绿）。选中行、勾、开关轨都用它。 */
    val accent = Color(0xFF34C759)

    /** 选中行底：绿 7%，让"当前轨"在暗底上一眼可见，而不是只靠行尾一个勾。 */
    val selectedRowWash = Color(0x3334C759)

    /** 浮层内的次级块（设备行 / 重扫按钮底）：白 6%，靠明度差分层，不再另起一套纯色。 */
    val insetSurface = Color(0x14000000)

    /** 选中行底的内缩：左右各 8dp（与 20dp 行首留白一起造成 12dp 外边）＋ 10dp 圆角。 */
    val rowWashInset = 8.dp
    val rowWashCorner = 10.dp

    // ---------- Toast（§3）----------
    val toastSurface = Color(0xE60E0E10)
    val toastRadius = 14.dp
    val toastShadow = 8.dp
    val toastPadH = 14.dp
    val toastPadV = 9.dp
    const val toastTextSize = 13f
    val toastDot = Color(0xFFFFB020)
    val toastDotSize = 6.dp
    val toastDotGap = 6.dp
    const val toastSuccessMs = 1_800L
    const val toastErrorMs = 2_600L

    // ---------- 文字色（§4 / §6，v1.1：层级靠字号，不靠压灰）----------
    /**
     * v2 口径：文字压的是 #151B23 的烟熏玻璃底（顶部 62% → 底部 65%），
     * 不是旧的 72% 实心黑 —— 旧的对比度比值表随底透明度一起失效，勿沿用。
     * 层级靠字号与 90%/68%/55% 三档，不靠继续压灰；亮场可读性按 spec §6.1 的让步判据验收。
     */
    val titleColor = Color(0xFFFFFFFF)
    val subtitleColor = Color(0xC4FFFFFF)
    /** 组头降到 55%：与副题 68% 拉开层级（v1 两者同色，读起来是一层）。 */
    val groupHeadColor = Color(0xB3FFFFFF)
    val rowTitleColor = Color(0xFFFFFFFF)
    val rowSubtitleColor = Color(0xC4FFFFFF)
    val valueColor = Color(0xF0FFFFFF)
    val chevronColor = Color(0xB3FFFFFF)
    /** 勾用品牌色：纯白勾在暗底上会被读成"另一个开关"。 */
    val checkColor = Color(0xFF34C759)
    val stepperGlyphColor = Color(0xE6FFFFFF)
    val resetColor = Color(0xC4FFFFFF)
    val closeColor = Color(0xB3FFFFFF)
    val dividerColor = Color(0x1A000000)

    // ---------- 字号（§4，sp）----------
    const val titleSize = 18f
    const val titleLineHeight = 24f
    const val titleTracking = 0.2f
    const val subtitleSize = 12f
    const val subtitleLineHeight = 17f
    const val subtitleTracking = 0.1f
    const val groupHeadSize = 11.5f
    const val groupHeadLineHeight = 16f
    const val groupHeadTracking = 0.5f
    const val rowTitleSize = 16f
    const val rowTitleLineHeight = 22f
    const val rowTitleTracking = 0.1f
    const val valueSize = 15f
    const val valueLineHeight = 20f
    const val resetSize = 14f
    const val resetLineHeight = 19f

    /** 数值列：15sp Medium + `tnum` 等宽，固定宽 64 右对齐（§5）。 */
    const val valueFontFeature = "tnum"
    val valueColumn = 64.dp

    // ---------- 间距与密度（§5）----------
    val hPadStart = 20.dp
    val rowTrailingPad = 16.dp
    val headerHeight = 56.dp
    val rowComfort = 56.dp
    val rowTallComfort = 72.dp
    val rowCompact = 42.dp
    val stepperRowHeight = 64.dp
    val toggleRowHeight = 64.dp
    val verticalPadComfort = 8.dp
    val verticalPadCompact = 4.dp
    val groupHeadTop = 12.dp
    val groupHeadBottom = 4.dp
    val dividerPad = 4.dp
    val groupGap = 12.dp
    val resetRowHeight = 48.dp
    val scrollBottomComfort = 64.dp
    val scrollBottomCompact = 8.dp
    val detailBottom = 64.dp

    /** 行内控件几何（§5 / §7）：勾 16 / chevron 16 / 步进 glyph 16 + 44 命中 + 32 按压圆。 */
    val rowCheckSize = 16.dp
    val rowChevronSize = 16.dp
    val stepperGlyphSize = 16.dp
    val stepperHit = 44.dp
    val pressCircle = 32.dp
    val barPressCircle = 28.dp
    val detailCloseHit = 44.dp
    val detailCloseGlyph = 16.dp

    /**
     * 按压覆盖（v3）：**面板内**一律反相为黑（浅底上白叠加看不见）——
     * · [pressScrim] 行 4% 黑；[stepperPressColor] 步进按压 黑 16%；
     * · [stepperCircleFill] 黑 12% + [stepperCircleStroke] 黑 20%（常驻圆底）。
     * ⚠️ [pressCircleColor] 例外：它画在**画面**上的顶栏/底栏控制钮，不受面板底色影响，
     * 保持白 8%（改成黑会在暗画面上直接消失）。
     * v2 把共用的圆提到 12%，连带控制栏按压变亮；v2.1 拆开，面板内步进仍保持 12%。
     */
    val pressScrim = Color(0x0A000000)
    val pressCircleColor = Color(0x14FFFFFF)
    /** 步进按压指示：白 12%（常驻圆底 7% + 描边 12% + 按压 12%）。 */
    val stepperPressColor = Color(0x29000000)
    const val pressScale = 0.985f

    /**
     * 步进按钮的常驻圆底：白 7% 填充 + 1dp 白 12% 描边。
     * v1 只有按下才出现圆底 ⇒ 静止时 −/+ 是两个孤零零的符号，点不点得中看不出来。
     */
    val stepperCircleFill = Color(0x1F000000)
    val stepperCircleStroke = Color(0x33000000)

    /** 开关（§7）：46×28 / 圆角 14；开＝品牌绿轨 + 白旋钮，关＝白 16% 轨 + 白旋钮。 */
    val toggleWidth = 46.dp
    val toggleHeight = 28.dp
    val toggleRadius = 14.dp
    val toggleTrackOn = Color(0xFF34C759)
    val toggleTrackOff = Color(0x29000000)
    val toggleKnobOff = Color(0xFFFFFFFF)
    val toggleKnobOn = Color(0xFFFFFFFF)

    // ---------- 滚动（§9）----------
    val scrollFade = 16.dp
    val scrollbarWidth = 2.dp
    val scrollbarInset = 4.dp
    val scrollbarThumbMin = 28.dp
    val scrollbarThumb = Color(0x33000000)
    const val scrollbarHoldMs = 600L

    // ---------- 动效（§8）----------
    /**
     * 命名沿用规范原文 `PlayerPanelStyle.motion`（设计稿写的就是这个名字）。
     * **不动 [PlayerMotion] 的全局值**：面板是新节奏，控制栏仍是旧节奏。
     */
    object motion {
        const val enterMs = 240
        const val exitMs = 160
        const val swapEnterMs = 140
        const val swapExitMs = 120
        const val swapShiftDp = 4f
        const val enterRiseDp = 10f
        const val enterRiseTopIconDp = -8f
        const val exitDropDp = 6f
        const val pressInMs = 90
        const val pressOutMs = 140
        const val valueFadeMs = 100
        const val checkMs = 140
        const val toggleMs = 240
        const val contentFadeMs = 120

        const val scrollbarInMs = 120
        const val scrollbarOutMs = 300
        const val reduceMs = 100

        /** 步进长按：400ms 后 80ms/步；2s 后 60ms/步；位移 > 8dp 取消。 */
        const val holdDelayMs = 400L
        const val holdFastAfterMs = 2_000L
        const val holdStepMs = 80L
        const val holdFastStepMs = 60L
        const val dragCancelDp = 8f

        val enterEase = CubicBezierEasing(0.2f, 0f, 0f, 1f)
        val exitEase = CubicBezierEasing(0.4f, 0f, 1f, 1f)
        val rowEase: Easing = FastOutSlowInEasing
    }

    // ---------- 控制栏统一语言（§10，几何不动）----------
    const val barActiveAlpha = 1f
    const val barInactiveAlpha = 0.62f
    const val barDimmedAlpha = 0.55f

    /** 双密度档：由 kind 选（不是两套 UI）。映射在 [PlayerPanelHost.density]。 */
    enum class Density { Comfort, Compact }

    fun rowHeight(density: Density, tall: Boolean): Dp = when {
        density == Density.Compact -> rowCompact
        tall -> rowTallComfort
        else -> rowComfort
    }

    fun verticalPad(density: Density): Dp =
        if (density == Density.Compact) verticalPadCompact else verticalPadComfort

    fun scrollBottomPad(density: Density): Dp =
        if (density == Density.Compact) scrollBottomCompact else scrollBottomComfort

    /** 组间分隔线只有 Comfort 有（Compact 靠 [groupGap] 留白，§5）。 */
    fun hasGroupDivider(density: Density): Boolean = density == Density.Comfort
}