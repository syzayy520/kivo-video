package com.kivo.cinema.ui.compose.player

import androidx.compose.ui.graphics.Color

/**
 * 播放页视觉 token（player-ui-v2.md §三）。
 *
 * **数值来源**：REX 1.0.2 真机横屏播放页逐帧像素测量
 * （录屏 2532×1170 @3x = 844×390pt），测量脚本见
 * `tools/measure_rex_player.py`。**禁止凭印象改动这里的数字。**
 *
 * 与全局 `DesignTokens` 的关系：全局那份是**浅色页面**的（首页/详情/设置），
 * 播放页是**深色覆盖层**，两套色系不共用 —— 这里只借用"圆角/间距"这类中性量，
 * 颜色一律在本文件里定义。
 */
object PlayerTokens {

    // ---------- 颜色 ----------
    /** 主文本（片名、时间、字幕）。 */
    val textPrimary = Color(0xFFFFFFFF)

    /** 次文本（副标题、说明）。 */
    val textSecondary = Color(0xFF8A8A8E)

    /** 进度条已播段。 */
    val trackPlayed = Color(0xFFFFFFFF)

    /** 进度条缓冲段（已缓冲但未播放）。 */
    val trackBuffered = Color(0xFF8A8A8E)

    /** 进度条未播放段。 */
    val trackRemaining = Color(0x4DFFFFFF)   // 白 30%

    /** 进度条未播放段（**拖动中**变亮：白 55%）。 */
    val trackDragging = Color(0x8CFFFFFF)     // 白 55%

    /** 顶栏渐变（自上而下）。比 REX 多这一层：我们的片源黑场多，纯透明图标会看不见。 */
    val topScrimTop = Color(0x00000000)
    val topScrimBottom = Color(0x47000000)   // 黑 28%

    /** 底部渐变（自上而下）。 */
    val bottomScrimTop = Color(0x00000000)
    val bottomScrimBottom = Color(0x8C000000)  // 黑 55%

    /** 面板背板（毛玻璃降级态）。 */
    val panelSurface = Color(0xF2101012)

    /** 玻璃面板的渐变底（上略亮 → 下更深）：平色块贴画面上会显廉价。 */
    val glassTop = Color(0xFA2A2A30)
    val glassBottom = Color(0xFA101013)

    /** 面板描边（深色场景里靠描边把面板边界说清楚）。 */
    val panelBorder = Color(0x26FFFFFF)

    /** 面板内的分隔线。 */
    val panelDivider = Color(0x1FFFFFFF)

    /** 动作图标的高亮态（切换成功后的 1.5s 反馈）。 */
    val activeAccent = Color(0xFF34C759)

    /** 轻提示的底色。 */
    val toastSurface = Color(0xE618181B)

    // ---------- 尺寸（单位 dp，横屏 844pt 基准）----------
    /** 顶栏高度。 */
    const val topBarHeight = 44f

    /** 底栏左右留白。 */
    const val bottomSidePad = 19f

    /** 进度条厚度。 */
    const val trackHeight = 3.5f

    /** 拖动时出现的滑块直径。 */
    const val thumbSize = 8f

    /** 进度条与时间文字之间的垂直间距。 */
    const val trackLabelGap = 6f

    /** 底部按钮行高度。 */
    const val buttonRowHeight = 24f

    /** 时间行与按钮行之间的间距。 */
    const val buttonRowGap = 14f

    /** 时间文字与进度条之间的间距。 */
    const val trackSideGap = 10f

    /** 左侧按钮之间的间距。 */
    const val buttonGap = 48f

    /** 右侧文字按钮之间的间距。 */
    const val textButtonGap = 24f

    /** 动作图标之间的间距（**图标边缘到边缘**，不是含触控区的间距）。 */
    const val actionGap = 28.5f

    /** 动作图标直径（线框外接圆）。 */
    const val actionIconSize = 21.5f

    /** 返回图标直径。 */
    const val closeIconSize = 16.5f

    /** 字幕距底距离 —— 让字幕不被底栏遮住。 */
    const val subtitleBottomGap = 24f

    /**
     * 底部渐变区的高度（固定值）。
     *
     * 【为什么固定】第一版让渐变跟着内容高度走，结果内容里的字幕把整条底栏
     * 顶高，时间行跑到 405pt（参考是 301pt）—— 底栏吃掉太多画面。
     * 现在：渐变固定 96pt（= 参考实测的 89pt + 余量），内容贴底排布。
     */
    const val bottomScrimHeight = 96f

    /** 面板圆角。 */
    const val panelRadius = 13f

    /** 面板行高（与设置页同规格）。 */
    const val panelRowHeight = 42f

    /** 面板宽度（右侧贴边时）。 */
    const val panelWidth = 300f

    /**
     * 面板最大高度（dp）。
     *
     * 【为什么要有上限】面板贴在右侧，上下要让开顶栏(44)与底栏(96)，
     * 可用高度 = 360 - 44 - 96 = 220dp（横屏 844×390pt 基准）。
     * 而倍速面板有 7 项 × 59dp = 413dp —— 不限高就会溢出屏幕，
     * 后面的档位**永远点不到**（实机实测：1.5x/1.75x/2x 被切掉）。
     * 限高 + 滚动 是正解。
     */
    const val panelMaxHeight = 330f

    /** 最小触控区（无障碍）。 */
    const val minTouchTarget = 44f

    // ---------- 字号（单位 sp）----------
    /** 片名。 */
    const val titleSize = 17f

    /** 时间。 */
    const val timeSize = 17f

    /** 底部文字按钮。 */
    const val textButtonSize = 17f

    /** 字幕。 */
    const val subtitleSize = 15f

    /** 面板行标题。 */
    const val panelTitleSize = 17f

    /** 面板行副标题。 */
    const val panelSubtitleSize = 11f

    /** 实时网速（等宽）。 */
    const val speedSize = 12f
}