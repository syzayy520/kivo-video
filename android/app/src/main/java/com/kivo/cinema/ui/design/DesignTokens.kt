package com.kivo.cinema.ui.design

import com.kivo.cinema.ui.tokens.DesignPalette

/**
 * 语义令牌（236 设计稿 §二，帧实测 @3x÷3）。
 * 色值本体在 `ui/tokens/DesignPalette`（明暗两套）；这里只做浅色态映射，公开成员保持不变。
 * 数值出处：Pictures/参照素材/frames/{grid_02, lib_02, set_a 等} + 05 号文档。
 */
object DesignTokens {
    // 背景动画渐变三色（05 §二：淡紫→淡粉→米白）
    val bgLavender = DesignPalette.Light.bgLavender
    val bgPink = DesignPalette.Light.bgPink
    val bgCream = DesignPalette.Light.bgCream

    // 文本
    val textMain = DesignPalette.Light.textMain
    val textSub = DesignPalette.Light.textSub

    // 控件绿（仅交互控件：toggle/主按钮——chrome 无全局 tint）
    val accentGreen = DesignPalette.Light.accentGreen
    val dangerRed = DesignPalette.Light.dangerRed

    // 语义色板（浅色态实测硬参数；深色态见 DesignPalette.Dark）
    val canvasFill = DesignPalette.Light.canvasFill            // 卡片/内容画布
    val groupedFill = DesignPalette.Light.groupedFill          // 分组列表底
    val pageBackground = DesignPalette.Light.pageBackground    // 页面底色 = 统一扁平画布（首页 resting/资源库/我的/设置）
    val hairline = DesignPalette.Light.hairline                // 分隔线/描边（黑 5%）
    val washFill = DesignPalette.Light.washFill                // 洗色叠加（P3 @50%）

    // 导航 chrome（269 v3：扁平灰 pill + 蓝选中；暗态供发现页深色环境 E11）
    val navPill = DesignPalette.Light.navPill
    val navIdle = DesignPalette.Light.navIdle
    val navActive = DesignPalette.Light.navActive
    val navPillDark = DesignPalette.Light.navPillDark
    val navIdleDark = DesignPalette.Light.navIdleDark.copy(alpha = .66f)

    // 玻璃
    const val glassAlpha = 0.70f          // 白 65–75%
    val glassFallback = DesignPalette.Light.glassFallback // API31 降级 92% 白
    const val radiusCard = 13f            // 卡/胶囊圆角 pt
    const val radiusPoster = 12f          // 海报卡圆角 pt：REX 实测 12（kivo-play-baseline §2 圆角行 / §6 C7；原 10 → 12）
    const val elevationCard = 1f          // 卡阴影极淡 1dp：REX 实测「几乎不可见」（kivo-play-baseline §2 阴影行；原 2dp）

    // 六页页面底色渐变端点（REX 实测；见 `DesignPalette.PageGradient`）
    // 供 B-2 起接入页面使用，当前无任何调用点（B-1 只出令牌）
    val pageGradient = DesignPalette.PageGradient.Light
    val pageGradientDark = DesignPalette.PageGradient.Dark

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

    // 字号（REX 实测，`docs/design/rex-dna-browse.md` §1.2；接连接口由 B-2 起改调用点）
    const val fontPageTitle = 34f       // REX 实测 34 Bold：页面大标题（§1.2「页面大标题」366 y110.3–139.7；baseline §2 大标题行）
    const val fontSectionTitle = 17f    // REX 实测 17 Bold：节标题（§1.2「节标题」332 y61.0–76.7；baseline §2/§6 C3）
    const val fontCardTitle = 13f       // REX 实测 13 Medium：卡片标题（§1.2「卡片标题」332 y242.3–254.0）
    const val fontMeta = 11f            // REX 实测 11 Regular：元信息/副标题（§1.2「副标题·元信息」332 y261.0–270.3）
    const val fontStat = 28f            // REX 实测 28 Bold：数字主字（§1.2「数字主字」0357/0358；单位 13 Regular）
}
