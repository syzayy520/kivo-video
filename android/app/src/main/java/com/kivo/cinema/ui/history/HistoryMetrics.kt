package com.kivo.cinema.ui.history

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.home.v6.V6Metrics

/**
 * 观看记录页的尺寸 / 同心圆角 / 颜色常量（单一真相，数值来自本目录 index.html 视觉稿）。
 *
 * 同心圆角：外圆角 = 内圆角 + 内边距（视觉稿 tokens.css）。
 * 颜色纪律：只从既有令牌派生，不写 hex（门禁裸色不涨；白 = canvasFill，近黑 = textMain）。
 */
internal object HistoryMetrics {

    // ---- 间距（393dp 基准，视觉稿实测） ----
    val screenPad = 20.dp          // px-5
    val rowPad = 10.dp             // 行内边距
    val heroInset = 10.dp          // 大卡内容与「继续」胶囊距边
    val rowGap = 12.dp             // space-y-3
    val groupGap = 28.dp           // space-y-7
    val navBottomPad = 18.dp       // dock bottom-18

    // ---- 尺寸 ----
    val topButton = 44.dp          // 页头返回圆钮 / 编辑胶囊高（w-11 h-11）
    val thumbW = 116.dp             // 记录行缩略图宽；高 72 贴合文字块自然高度（设计稿齐平口径）
    val thumbH = 72.dp
    val checkSize = 22.dp
    val pillH = 32.dp              // 「继续」胶囊高
    val badgeH = 20.dp
    val barH = 4.dp
    val dlgButtonH = 48.dp
    val swipeReveal = 88.dp        // 左滑露出的删除区宽（内含 64dp 浮置胶囊）

    // ---- 同心圆角 ----
    val rThumb: Dp = 8.dp
    val rRow = rThumb + rowPad                       // 行 18 = 缩略图 8 + 10
    val rHero = pillH / 2 + heroInset                // 大卡 26 = 继续胶囊 16 + 10
    val badgeInset = rHero - badgeH / 2              // 角标内缩 16，圆心与卡角同心
    val rDlgButton = 16.dp
    val rDlg = rDlgButton + 14.dp                    // 弹窗 30 = 按钮 16 + 14

    // ---- 颜色（全部令牌派生） ----
    /** 白卡上的进度条底 rgba(20,18,30,.08)。 */
    val track = DesignTokens.textMain.copy(alpha = .08f)
    /** 已看完的进度条填充（视觉稿 #C4C3C9 ≈ 副文灰加深）。 */
    val doneFill = DesignTokens.textSub.copy(alpha = .55f)
    /** 白卡发丝描边 rgba(20,18,30,.035)。 */
    val rowStroke = DesignTokens.textMain.copy(alpha = .035f)
    /** 选择圈未选中描边（视觉稿 #C4C3C9）。 */
    val checkRing = DesignTokens.textSub.copy(alpha = .55f)
    /** 最浅三级文本（视觉稿 ink3：占位 / 禁用）。 */
    val textFaint = DesignTokens.textSub.copy(alpha = .8f)
    /** 浅底玻璃（页头 / 导航 / 底栏：白 66%）。 */
    val glassLight = DesignTokens.canvasFill.copy(alpha = .66f)
    /** 画面上的玻璃（「继续」胶囊：白 20%，减厚版，边缘更细腻）。 */
    val glassOnImage = DesignTokens.canvasFill.copy(alpha = .20f)
    /** 弹窗取消钮底（视觉稿 #ECEBF1 ≈ 近黑 6%）。 */
    val dlgButtonBg = DesignTokens.textMain.copy(alpha = .06f)
    /** 弹窗卡片底（视觉稿 rgba(251,250,253,.94)）。 */
    val dlgCardBg = DesignTokens.canvasFill.copy(alpha = .94f)
    /** 弹窗遮罩 rgba(20,18,30,.28)。 */
    val dlgScrim = DesignTokens.textMain.copy(alpha = .28f)
    /** 分组标题细线 bg-black/[.08]。 */
    val hairlineStrong = DesignTokens.textMain.copy(alpha = .08f)

    // 从首页 v6 直接复用的三色（同一签条语言）。
    val brass = V6Metrics.brass
    val quality = V6Metrics.quality
    val badgeGlass = V6Metrics.badgeGlass
}