package com.kivo.cinema.ui.search

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

/**
 * 搜索页尺寸/色令牌（search-final.html :root 与各类规则；数值即稿面 px→dp）。
 * 文本主/次色、hairline、页面底一律走 DesignTokens；这里只放本页新增的尺寸与语义色。
 */
internal object SearchTokens {
    val pad = 13.dp
    /** 底部内容留白 = 胶囊 50 + 距底 22 + 余量 16（family-tree D2，任何滚动容器不得被底栏裁内容）。 */
    val navBottomSpace = 88.dp

    val fieldHeight = 46.dp
    val chipHeight = 34.dp

    val categoryHeight = 84.dp
    val categoryGap = 9.dp
    val categoryRadius = 14.dp

    val hotThumbWidth = 40.dp
    val hotThumbHeight = 60.dp
    val resultThumbWidth = 52.dp
    val resultThumbHeight = 78.dp

    val bestHeight = 208.dp
    val bestRadius = 27.dp

    val sectionGap = 22.dp
    val sectionTitle = 20.dp // 仅作行高参考；字号在调用处以 sp 标注

    /** 蓝色只给光标与焦点环（稿面 --blue）。 */
    val focusBlue = Color(0xFF0A84FF)
    /** 三级文字（排名非前三、英文小字等，稿面 --t3）。 */
    val textFaint = Color(0xFFA3A2A9)
    /** 筛选未选中文字（稿面 --t4）。 */
    val textFilter = Color(0xFF3E3D44)
    /** 可播签条绿字 / 绿点（稿面 --ok / --okdot）。 */
    val playableText = Color(0xFF15783A)
    val playableDot = Color(0xFF34C759)
    val idleDot = Color(0xFFBEBDC4)
    /** 热搜白卡 / 最近搜索 chips 底（稿面 .hot / .chip）。 */
    val boardFill = Color.White.copy(alpha = .76f)
    val chipFill = Color.White.copy(alpha = .74f)
    /** 清除钮灰底（稿面 .clr）。 */
    val clearFill = Color(0xFFB9B8BF)
    /** HDR 角标金字（稿面 .q.hdr）。 */
    val hdrText = Color(0xFFFFD37A)
    /** 吸顶摘要底（稿面 .stick 白 78% 页面底）。 */
    val stickyFill = Color(0xF0ECF6).copy(alpha = .96f)
}