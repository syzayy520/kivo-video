#pragma once

#include "video/video_plane/core/audio/clock/audio_clock_types.hpp"

namespace kivo::video::core::audio {

/// 音频时钟来源的注入接口。
///
/// C5 的呈现节拍器只依赖这一层，不认识 AAudio、也不认识 AudioUnit ——
/// 这是 `CURRENT.md` §六 第 1 条（core 下不许出现平台头）能成立的前提。
///
/// 实现方约定：`now()` 必须可在任意线程调用，且**不加锁**。
/// 视频线程每帧都要问一次时间，锁在这里会直接变成呈现抖动。
///
/// 刻意**不是** const：每次查询都会推进内部计数（回退次数、拒绝次数、外推量）。
/// 用 mutable 骗过编译器让签名看起来更"干净"，只是把状态藏起来而已。
class AudioClockSource {
public:
    virtual ~AudioClockSource() = default;

    /// 查询当前时钟位置。允许返回陈旧值，但必须把 `stale` 置 true。
    [[nodiscard]] virtual AudioClockSnapshot now() = 0;

    /// seek 之后重设时钟基准（P12-F2）。
    ///
    /// 音频时钟必须覆盖它 —— 停/启一次 AAudio 流之后帧位置会归零，
    /// 不重设基准，媒体时间会退回 0，视频被节拍器拖回片头。
    ///
    /// `SystemClockSource` 也必须覆盖（244）：视频无音轨时进度取自系统钟的墙钟绝对量，
    /// seek 后若只 `reset()` 把原点挪回 now、不把原点挪到 demux 落点 landing，
    /// 进度会停在 seek 前的位置（真机：landing=25 s、进度恒 ~5 s）。
    /// 原注释说"没有媒体基准可重设"在 seek 场景并不成立 —— landing 就是那个基准。
    /// 默认空实现只留给"纯墙钟、从不 seek 到具体时刻"的场合。
    virtual void rebase(std::int64_t /*media_position_ns*/) noexcept {}
};

}  // namespace kivo::video::core::audio
