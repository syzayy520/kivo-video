package com.kivo.cinema.ui.design.home.v6

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 首页 v6 的尺寸 / 圆角 / 字号 / 颜色常量（单一真相，数值来自 kivo-home-mobile-v6.html）。
 *
 * 海报行：20 + 3×106 + 3×12 = 374dp → 393 宽屏一屏完整 3 张、第 4 张露约 19dp（对标 Apple TV）。
 */
internal object V6Metrics {
    // 间距
    val screenPad = 20.dp
    val cardGap = 12.dp
    val sectionGap = 40.dp
    val headerGap = 12.dp

    // 卡片尺寸
    val posterW = 106.dp
    val posterH = 159.dp
    val continueW = 228.dp
    val continueH = 128.dp
    val wideW = 248.dp
    val wideH = 140.dp
    val genreW = 220.dp
    val genreH = 124.dp

    // 圆角（同心：外框大、内图小）
    val rPoster = 12.dp
    val rCard = 16.dp

    // 字号
    val fsSection = 21.sp
    val fsSectionSub = 13.sp
    val fsCardTitle = 13.sp
    val fsWideTitle = 14.sp
    val fsMeta = 12.sp
    val fsBadge = 10.sp

    // 颜色（品牌只做点缀）
    val brass = Color(0xFFC98A3B)
    val brassInk = Color(0xFF8F5A1C)
    val quality = Color(0xFF2F6F5E)
    val qualityAlt = Color(0xFF3B8571)
    val badgeGlass = Color(0x85101014)
    val strokeOnLight = Color(0x12000000)
    val strokeOnDark = Color(0x1AFFFFFF)
}
