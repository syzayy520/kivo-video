package com.kivo.cinema.mpv

/**
 * 253-P6-3：倍速控制器 —— **只做一件事**：读与写 mpv 的 `speed` 属性。
 *
 * ## 为什么单独一个类而不是塞进 MpvPlaybackSession
 * 旧内核的倍速是**音视频两条链路各改各的**（视频解帧率 + 音频重采样），
 * 所以它有自己的状态机与边界（变速时的音画对齐、变速中 seek 的处理）。
 *
 * mpv 内核下这些**全部由 mpv 自己管**（单一 demux + 单一时钟，
 * 变速时音频重采样与视频帧率是同一个调度器的事）。
 * 我们这一层只剩"把用户选的值写进去、把当前值读出来" ——
 * 逻辑薄到不值得占一个会话字段，但仍然值得有自己的文件：
 * 它有**自己的边界条件**（哪些倍速是合法的、变速后要不要重新读回）。
 *
 * ## 合法值
 * mpv 的 `speed` 范围是 0.01 ~ 100，但我们**限制到 0.25 ~ 4**：
 *  · 低于 0.25 倍的实际用途只有"听不清的片段慢慢听"，
 *    而 mpv 在这种速度下要维持同步就得大量丢帧，手机上会明显卡；
 *  · 高于 4 倍的画面已经看不清字幕，且多数片源没有对应帧率，
 *    mpv 只能靠重复帧凑，观感是"卡"不是"快"。
 * 超出范围的输入一律**夹到边界**而不是拒绝 —— 用户拖动滑块时
 * 拒绝会表现为"滑到头就不动了"，夹边则表现为"到头了"，
 * 后者符合直觉。
 */
class MpvSpeedController(private val session: MpvSession) {

    /** 倍速下限。 */
    val minSpeed: Double = 0.25

    /** 倍速上限。 */
    val maxSpeed: Double = 4.0

    /** 常用档位（供 UI 直接铺一排按钮，不必让用户拖连续滑块）。 */
    val presets: List<Double> = listOf(0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0)

    /** 把任意输入夹到合法区间。 */
    fun clamp(value: Double): Double = value.coerceIn(minSpeed, maxSpeed)

    /**
     * 设置倍速。
     *
     * @return true = 写入成功。
     */
    fun setSpeed(value: Double): Boolean = session.setProperty("speed", clamp(value).toString())

    /**
     * 当前倍速。
     *
     * @return 当前值；读不到返回 1.0（正常播放速度）。
     */
    fun speed(): Double = session.property("speed")?.toDoubleOrNull() ?: 1.0

    /** 是否处于正常速度。 */
    fun isNormalSpeed(): Boolean = kotlin.math.abs(speed() - 1.0) < 0.01

    /**
     * 回到正常速度。
     *
     * 【为什么不用"上一次非 1.0 的速度"这种记忆位】
     * 倍速是**状态**而不是模式：用户点了 2×，可能过了十分钟才想调回 1×。
     * 记一个"上次的倍速"在用户已经回到 1× 之后会给出错误的回跳目标。
     * 直接回 1.0 是唯一无歧义的行为。
     */
    fun reset(): Boolean = setSpeed(1.0)
}
