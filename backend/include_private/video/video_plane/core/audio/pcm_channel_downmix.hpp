#pragma once

#include <cstdint>

namespace kivo::video::core::audio {

/// 5.1 → 立体声的下混矩阵系数。
///
/// 系数取自 AC3/Dolby Digital 的标准下混表（1/√2 ≈ 0.7071）：
///   L = FL + 0.7071·FC + 0.7071·BL
///   R = FR + 0.7071·FC + 0.7071·BR
/// LFE 不进立体声（标准做法：低频效果本来就该由独立声道承担，
/// 折进双声道只会让低频糊在人声上）。
///
/// 系数选择的后果（这一条决定了矩阵长什么样）：
///   - **FC 权重最高**：真高清音轨的人声几乎全部在 FC。
///     实测 Shelter 这条 TrueHD 5.1(side) 轨：FC 占总能量 70.7%，
///     FL/FR 各只有 12%/11%。FC 权重给 1.0 之外还各给 0.7071，
///     就是为了让折成双声道后人声仍然是主音。
///   - FC 同时进 L 和 R（各 0.7071），人声因此在双声道里保持居中感，
///     而不是偏到一边。
///   - 环绕声道（BL/BR）各取 0.7071：保留空间感，又不至于盖过中置。
///
/// 为什么不交给系统做（这是本文件存在的全部理由）：
///   我们的 App 把 6 声道原样交给 AAudio，设备 HAL 是立体声，
///   于是平台必须自己做 6→2。真机实测（K40 / dumpsys media.audio_flinger）：
///     Fast track Chn mask = 8000003F   ← 我们这条 5.1(side) 轨道
///     Mixer channel Mask = 0x3        ← 输出端只有 front-left/front-right
///   平台的下混路径（AudioFlinger → AudioEffect downmix）对 5.1(side)
///   的处理并不等价于上面的标准矩阵；而 AOSP 自己的 downmix 折法
///   把 FC 按 −3 dB 计入（Downmix_foldFrom5Point1: centerPlusLfeContrib），
///   对"人声几乎全在 FC"的真高清音轨，人声会被明显压掉。
///   参考实现（mpv / libmpv / SenPlayer）都是自己折，不指望平台。
///
/// 归一化：**不做**。标准矩阵在正常素材下峰值 ≤ 1.0；
/// 若素材本身过载，那是素材的事，削掉它比悄悄压低整条更糟。
/// 削波交给下游（浮点 → 设备定点）按其既有策略处理。
constexpr float kDownmixCenterWeight = 0.70710678F;  // 1/√2
constexpr float kDownmixSurroundWeight = 0.70710678F;  // 1/√2

/// 把 5.1 交错 PCM 折成立体声交错 PCM。
///
/// 输入：6 声道交错 float，声道顺序 FL, FR, FC, LFE, BL, BR
///       （即 AV_CH_LAYOUT_5POINT1 / 5POINT1_SIDE 的原生顺序，
///        与 FFmpeg 解码输出一致）。
/// 输出：2 声道交错 float，顺序 L, R。
///
/// @param src            源交错缓冲（每帧 6 个 float）
/// @param src_frames     源帧数
/// @param dst            目标交错缓冲（每帧 2 个 float），不得与 src 重叠
/// @param dst_capacity_frames 目标容量（帧），必须 ≥ src_frames
/// @return 实际写入的目标帧数；src_frames 为 0 时返回 0
///
/// 契约：
///   - 只接受 5.1 → 立体声这一种映射。其它声道数**不做隐式映射**，
///     交由调用方判定失败（与 PcmFormatConverter 的"明确不做声道映射"一致）。
///   - 源帧数超出目标容量时**只写能写下的部分并返回实际帧数**，
///     由调用方据此发现截断；不静默丢弃。
std::int32_t downmix_5point1_to_stereo(const float* src, std::int32_t src_frames, float* dst,
                                       std::int32_t dst_capacity_frames) noexcept;

/// 5.1 → 立体声下混的声道数常量，供调用方做尺寸校验。
inline constexpr std::int32_t kSourceChannels5Point1 = 6;
inline constexpr std::int32_t kStereoChannels = 2;

}  // namespace kivo::video::core::audio
