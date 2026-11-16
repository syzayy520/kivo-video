package com.kivo.cinema.hls

/**
 * 转发 HLS 时用的选档策略（由播放层构造，交给代理层执行）。
 *
 * ## 为什么单独一个类，而不是三个参数
 * 代理层（`LocalRangeResponder`）只需要"给定 variant 列表，选出哪一个"这一件事；
 * 把 `maxHeight / maxBps / canDecode` 打包传，签名稳定，也不必让代理知道
 * 这些数字从哪来（屏幕尺寸、带宽预算、设备能力各自由上层负责）。
 *
 * @param maxHeight 分辨率上限（超出这个高的档不选）；<=0 不限制
 * @param maxBps 带宽预算（bps）；<=0 不限制
 * @param canDecode 设备能不能解这一档（codec × 分辨率闸门）
 */
class HlsForwardPolicy(
    val maxHeight: Int,
    val maxBps: Long,
    val canDecode: (HlsVariant) -> Boolean = { true },
)
