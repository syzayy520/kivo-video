package com.kivo.cinema.danmaku

/**
 * 弹幕显示设置（v3.1 §四，用户可调项的**唯一真值**）。
 *
 * 【为什么是本地态而不是 mpv 属性】弹幕是 Compose Canvas 自绘层，
 * libmpv 没有 danmaku-* 属性 —— 旧版把 "danmaku-speed" 写进 mpv，
 * 每次必失败（属性不存在）。本地态 + 面板 stepper 直写才是真链路。
 *
 * 【hasItems 为什么在这里】"能不能开弹幕"取决于有没有解析出条目，
 * 顶栏开关、面板开关行、覆盖层三处都要读同一个事实，不能各查各的。
 */
data class DanmakuSettings(
    /** 一条弹幕从右缘滚到左缘的时长（秒）。 */
    val speedSec: Float = 8f,
    /** 字号倍率（乘在每条弹幕自带的 size 上）。 */
    val fontScale: Float = 1f,
    /** 整体不透明度（描边与填充一起乘）。 */
    val opacity: Float = 1f,
    /** 滚动弹幕占据的画面区域。 */
    val area: DanmakuArea = DanmakuArea.FULL,
    /** 当前片源是否解析出了弹幕条目（无条目时开关必须置灰带原因）。 */
    val hasItems: Boolean = false,
)

/** 滚动区域（顶部/底部固定弹幕不受此限制，它们本来就钉在边上）。 */
enum class DanmakuArea { FULL, TOP_HALF, BOTTOM_HALF }