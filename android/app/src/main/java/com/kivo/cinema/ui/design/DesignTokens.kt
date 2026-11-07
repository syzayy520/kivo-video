package com.kivo.cinema.ui.design

import androidx.compose.ui.graphics.Color

/**
 * 全局 tokens（236 设计稿 §二，帧实测 @3x÷3）。
 * 数值出处：Pictures/参照素材/frames/{grid_02, lib_02, set_a 等} + 05 号文档。
 */
object DesignTokens {
    // 背景动画渐变三色（05 §二：淡紫→淡粉→米白）
    val bgLavender = Color(0xFFEAE4F5)
    val bgPink = Color(0xFFF4E3EA)
    val bgCream = Color(0xFFF7F1E8)

    // 文本
    val textMain = Color(0xFF1A1A1A)
    val textSub = Color(0xFF8A8A8E)

    // 控件绿（仅交互控件：toggle/主按钮——chrome 无全局 tint）
    val accentGreen = Color(0xFF34C759)
    val dangerRed = Color(0xFFFF3B30)

    // 语义色板（RexDesign Assets.car 硬参数，浅色态；深色态 v2）
    val canvasFill = Color(0xFFFFFFFF)      // 卡片/内容画布
    val groupedFill = Color(0xFFF2F2F7)     // 分组列表底
    val pageBackground = Color(0xFFF2F2F7)  // 页面底色
    val hairline = Color(0x0D000000)        // 分隔线/描边（黑 5%）
    val washFill = Color(0x80EDEDFB)        // 洗色叠加（P3 @50%）

    // 玻璃
    const val glassAlpha = 0.70f          // 白 65–75%
    val glassFallback = Color(0xEEFFFFFF) // API31 降级 92% 白
    const val radiusCard = 13f            // 卡/胶囊圆角 pt
    const val radiusPoster = 10f          // 海报卡圆角 pt

    // 布局常量（05 §三）
    const val screenPad = 13f
    const val gap = 10f
    const val navHeight = 50f
    const val navBottomPad = 22f
    const val navSidePad = 16f
    const val circleButton = 50f
    const val posterWidth = 92f
    const val continueCardWidth = 230f
    const val rowHeight = 59f             // 设置/表单行高

    // 字号（帧实测）
    const val fontSectionHeader = 22f     // 继续观看/正在热映 节标题
    const val fontNavLabel = 10f
    const val fontPosterTitle = 15f       // 帧实测修正（05 粗测 12）
    const val fontPosterDate = 13f        // 帧实测修正（05 粗测 10）
    const val fontRowTitle = 17f          // 设置/表单行标题、继续观看卡标题
    const val fontFootnote = 10.5f
    const val fontHeroMeta = 17f
    const val fontHeroSynopsis = 15f
}
