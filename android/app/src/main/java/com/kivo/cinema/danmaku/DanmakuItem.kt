package com.kivo.cinema.danmaku

/**
 * 弹幕条目（223 设计稿 D1）：B 站 XML 格式的单条弹幕。
 *
 * @param text 弹幕文本
 * @param time 出现时间（秒，视频内）
 * @param mode 1=滚动 4=底部 5=顶部
 * @param color 文字颜色（0xRRGGBB）
 * @param size 字号（B 站标准：18/24/36）
 */
data class DanmakuItem(
    val text: String,
    val time: Float,
    val mode: Int = 1,
    val color: Int = 0xFFFFFF,
    val size: Int = 25,
)
