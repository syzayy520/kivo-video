package com.kivo.cinema.ui.compose.player

import androidx.compose.ui.graphics.Color

/**
 * 播放页视觉 token（player-ui-v3-rex-replication.md §一，唯一常量源）。
 *
 * **数值来源**：REX 1.0.2 真机横屏播放页逐像素测量
 * （2532×1170 @3x = 844×390pt，**pt = px/3**，误差 ±2pt）。
 * 复算：`python tools/measure_rex_playback.py {icons|bottom|panel|text} <png>`，
 * 原文 `artifacts/rex-measurements.txt`。**禁止凭印象改动这里的数字。**
 *
 * v3 推翻了 v2 的四处：轨道 3.5→**6pt**、标题 17→**16pt**、
 * 图标间距 36→**pitch 50pt**、面板圆角 13→**16pt**。以本文件为准。
 */
object PlayerTokens {

    // ---------- 颜色 ----------
    val textPrimary = Color(0xFFFFFFFF)
    val textSecondary = Color(0xFF8A8A8E)

    /**
     * 进度条三段（§1.2，2026-11-12 用户供帧复测推翻 v3 的"黑45%罩"）：
     * REX 是**半透明白 + 1px 提亮描边**的玻璃质感，视频从轨下透出来；
     * 实色深灰会显得"死"。实测：未播填充白12% / 边白35%；
     * 缓冲填充白55% / 边白75%；已播纯白。复算见 v3.2 证据。
     */
    val trackPlayed = Color(0xFFFFFFFF)
    val trackBuffered = Color(0x8CFFFFFF)
    val trackBufferedRim = Color(0xBFFFFFFF)
    val trackRemaining = Color(0x1FFFFFFF)
    val trackRemainingRim = Color(0x59FFFFFF)

    /** 拖动中未播段提亮（§1.5）：让用户看清"松手会落到哪一段"。 */
    val trackDragging = Color(0x40FFFFFF)
    val trackDraggingRim = Color(0x80FFFFFF)

    /**
     * 顶栏**无遮罩**（§1.1：白图标直压画面）。
     * 保留常量是为了让"刻意不加渐变"在代码里可见，而不是被误认为漏做。
     */
    val topScrimTop = Color(0x00000000)
    val topScrimBottom = Color(0x00000000)

    /** 底部渐变：REX 底栏有黑渐变，选集整替态也靠它压住画面。 */
    val bottomScrimTop = Color(0x00000000)
    val bottomScrimBottom = Color(0x8C000000)

    /** 面板玻璃底（§1.3：黑 ≈75%）。 */
    val panelSurface = Color(0xBF000000)

    /** 玻璃顶部高光（替代 Android 做不出的背景模糊）。 */
    val glassSheen = Color(0x14FFFFFF)

    /** 面板描边 / 分隔线（§1.3：白 8%）。 */
    val panelBorder = Color(0x14FFFFFF)
    val panelDivider = Color(0x14FFFFFF)

    /** 滚动条（§1.3：3pt 白 40%，贴右内 6pt）。 */
    val scrollbar = Color(0x66FFFFFF)

    /** chevron（§1.3：白 60%）。 */
    val chevron = Color(0x99FFFFFF)

    /** 微调卡的内嵌卡底（§1.4 形态 5：黑 30%）。 */
    val innerCardSurface = Color(0x4D000000)

    /** 资源源卡描边（形态 11：白 35%）。 */
    val sourceCardBorder = Color(0x59FFFFFF)

    /** chips 未选底（形态 11：白 12%）；选中＝白描边 1.5pt。 */
    val chipSurface = Color(0x1FFFFFFF)

    /** 资源卡的"当前源"绿点。 */
    val sourceDot = Color(0xFF34C759)

    /** 动作切换成功后的高亮反馈。 */
    val activeAccent = Color(0xFF34C759)

    val toastSurface = Color(0xE618181B)

    // ---------- 顶栏（§1.1）----------
    /** 自绘状态行高度（时钟 y10–23pt）。 */
    const val statusRowHeight = 30f

    /**
     * 状态行→图标行的间隙。K40 实测图标行心 51.5dp vs REX 55pt（Δ3.5）：
     * 状态行 ink 只到 23pt，行高 30 已含余量，补 3dp 把图标行心推到 54.5。
     */
    const val statusIconGap = 3f

    /** 图标行高度：中心 y≈55pt ⇒ 行占 33–77pt。 */
    const val iconRowHeight = 44f

    /** 顶栏总高（状态行 + 图标行）。 */
    const val topBarHeight = statusRowHeight + iconRowHeight

    /**
     * 左右基础留白。实测 ink 边距 ≈66pt = 横屏安全区 ≈47 + 内边距 ≈19；
     * 安全区交给 [playerSafeArea]，这里只加 19。
     */
    const val sidePad = 19f

    /** 图标中心距（实测 419.7→769.0 共 8 个，pitch 50pt）。 */
    const val actionPitch = 50f

    /** 图标命中盒（≥44pt 无障碍下限；pitch 靠 spacedBy 补齐）。 */
    const val actionHitBox = 44f

    /** 图标 glyph 外接盒。v3.2：22→23 —— REX 投屏/PiP 掩码 ink 22.7×18.3pt，
     *  24 画布 ink ≈23.95 单位 ⇒ 盒 23dp 时 ink 22.95dp（Δ0.25pt）。 */
    const val actionIconSize = 23f

    /** 按压反馈：按下时图标/文字不透明度（REX 的反馈族＝本体变色，无涟漪）。 */
    const val pressAlpha = 0.6f

    /**
     * 图标行的横向留白。
     *
     * 【为什么是 8 而不是 [sidePad] 的 19】命中盒 44pt 比 glyph 22pt 每侧宽 11pt，
     * 留白必须减掉这 11pt，**ink** 才落在实测的 66pt 边距上
     * （47 安全区 + 8 留白 + 22 半盒 = 77 = ✕ 中心，实测 74.3，Δ2.7pt）。
     */
    const val iconRowPad = 8f

    /** ✕ 与主页的命中盒间隙（中心距实测 47.5pt − 44pt 盒）。 */
    const val leftIconGap = 1f

    /** ✕ 盒 20dp：杆 span 18 + 线宽 2.4 = 20.4/24 ⇒ ink 17dp（REX 实测 17pt）。 */
    const val closeIconSize = 20f
    /** 主页盒 22dp：ink 23.1/24 ⇒ 21.2dp（REX 实测 21×18.3pt）。 */
    const val homeIconSize = 22f

    /** 状态行右端：网速文本 + 电池。 */
    const val batteryWidth = 23f
    const val batteryHeight = 12f

    // ---------- 底栏（§1.2）----------
    /** 轨道厚度（实测 6pt，**无滑块**）。 */
    const val trackHeight = 6f

    /**
     * 按下/拖动中的轨道厚度（v3.1 用户指定的选中增强，**非 REX 实测**）：
     * 6→10pt，[PlayerMotion.trackMs] 过渡；松手回 6。参考没有滑块，不加。
     */
    const val trackActiveHeight = 10f

    /** 轨道命中区高度（细轨也要好按）。 */
    const val trackHitHeight = 24f

    /** 时间文字与轨道的间距：左 11pt（99→110）、右 4pt（740→744）。 */
    const val trackStartGap = 11f
    const val trackEndGap = 4f

    /** 按钮行高度（⏸ ink 24pt）。 */
    const val buttonRowHeight = 24f

    /** 轨道行与按钮行的间距（轨道中心 304.8 / 按钮中心 345 ⇒ Δ40pt）。 */
    const val buttonRowGap = 16f

    /** 底栏距屏底（按钮行中心距底 45pt = 33 + 24/2）。 */
    const val bottomPad = 33f

    /** ⏸ → 下一集 的中心距（75.5 → 140.7）。 */
    const val playButtonPitch = 65f

    /**
     * 底栏横向留白：让时间 ink 落在实测 63 / 781pt
     * （844 − 47 安全区 − 16 = 781）。
     */
    const val bottomSidePad = 16f

    /**
     * 左按钮组相对底栏内容左缘再左移的量。
     * ⏸ 的 ink 左缘实测 66.6pt，而 44pt 命中盒比 24pt glyph 每侧宽 10pt ⇒
     * 63 + 10 − 7 = 66。不减这 7pt，⏸ 就会比参考右偏 10pt。
     */
    const val playGroupInset = 7f

    /** 右文本钮间距（ink 间隙 37–38pt = 29 + 两侧各 4 的内边距）。 */
    const val textButtonGap = 29f
    const val textButtonPad = 4f

    /** 底部渐变高度（盖住轨道行 + 按钮行 + 余量）。 */
    const val bottomScrimHeight = 110f

    /** 暂停态左中浮相机钮（iPhone 现屏：48pt 圆，中心 ≈(88,195)pt）。 */
    const val captureFloatSize = 48f
    const val captureFloatGlyph = 24f
    /** 安全区外再补的左距：88 − 24(半径) − 47(安全区) = 17。 */
    const val captureFloatStart = 17f

    // ---------- 面板（§1.3 / §1.4）----------
    const val panelRadius = 16f
    const val panelWidth = 248f
    const val panelMaxHeight = 330f
    const val panelPadding = 16f

    /** 列表行高：单行 42 / 双行 60。 */
    const val panelRowHeight = 42f
    const val panelRowTallHeight = 60f

    /** 行内几何：无 icon 时题在 +43（勾心 +25）；有 icon 时题在 +65（icon 心 +30）。 */
    const val rowTitleInset = 43f
    const val rowIconTitleInset = 65f
    const val rowIconSize = 18f
    const val rowCheckSize = 14f
    const val rowTrailingPad = 16f

    const val sectionHeadSize = 12f
    const val chevronSize = 12f
    const val scrollbarWidth = 3f
    const val scrollbarInset = 6f

    /** 微调卡（形态 5/6/9）。 */
    const val detailCardWidth = 357f
    const val detailCardTop = 16f
    const val detailCloseSize = 38f
    const val detailRowHeight = 50f
    const val innerCardMargin = 15f
    const val innerCardRadius = 10f
    const val stepperWidth = 93f
    const val stepperHeight = 30f

    /** 资源内联展开（形态 11）。 */
    const val chipHeight = 30f
    const val chipGap = 8f
    const val sourceCardWidth = 208f
    const val sourceCardTopGap = 12f
    const val sourceDotSize = 6f

    /** 选集整替（形态 12）。 */
    const val episodeThumbWidth = 125f
    const val episodeThumbHeight = 52f
    const val episodeThumbPitch = 137f
    const val episodeThumbRadius = 8f
    const val episodeThumbBorder = 2f

    /** 听视频态（形态 13）。 */
    const val audioOnlyGlyphSize = 60f

    /** 拖动预览浮窗（§1.5）。 */
    const val previewWidth = 167f
    const val previewHeight = 90f
    const val previewBorder = 2f
    const val previewRadius = 8f
    const val previewCameraSize = 48f

    /**
     * 锁定态挂锁（sk_16 逐像素复测，推翻旧稿"16pt"）：
     * glyph 30×28.3pt、线宽 2.5pt；锁体中心 x≈65pt = 播放键槽位，
     * 整 glyph 中心 y≈344.8pt = 按钮行中心 ⇒ 与播放键同槽。
     */
    const val lockGlyphSize = 30f
    /** 挂锁 44pt 命中盒的底边距：33 + (24−44)/2 = 23，使盒心落在按钮行心 345pt。 */
    const val lockSlotBottomPad = 23f

    const val minTouchTarget = 44f

    // ---------- 字号（sp，§一 实测）----------
    const val titleSize = 16f
    const val timeSize = 13f
    const val textButtonSize = 14f
    const val panelTitleSize = 16f
    const val panelSubtitleSize = 11f
    const val speedSize = 12f
    const val danmakuSize = 11.5f
    const val episodeIndexSize = 12f
    const val episodeTitleSize = 14f
    const val sourceBadgeSize = 20f
    const val sourceMetaSize = 14f
}