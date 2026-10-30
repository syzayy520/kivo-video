#pragma once

#include <cstdint>

namespace kivo::video::core::audio {

/// 音频时钟的数据来源。
///
/// 证据里的 `clock_source` 必须**真实**反映本次运行用的是哪一种：
/// 用回退档撑住却报 `audio_timestamp`，等于把"时钟精度降级"这件事
/// 从证据里抹掉，而 G7 的 40 ms 阈值恰恰依赖这一点。
enum class AudioClockSourceKind : std::int32_t {
    kAudioTimestamp = 0,      ///< AAudioStream_getTimestamp（主源，已含硬件延迟补偿）
    kAudioFramesRead = 1,     ///< getFramesRead / rate（回退 1 档，同为"已呈现"语义）
    kAudioFramesWritten = 2,  ///< (getFramesWritten − latency) / rate（回退 2 档）
    kSystem = 3,              ///< 无音轨时的独立单调时钟，不许冒充音频时钟
};

/// 来源的可读名，直接进证据 JSON。
[[nodiscard]] const char* audio_clock_source_name(AudioClockSourceKind kind) noexcept;

/// 一次时钟查询的结果。纯数据，不含行为 —— C5 只吃这一个结构。
struct AudioClockSnapshot {
    /// 媒体时间轴上的位置（ns），从 0 开始。
    /// 由"已呈现帧位置"驱动，因此零点天然是首帧实际出声（见 15-p12c4-design.md §三）。
    std::int64_t position_ns = 0;

    AudioClockSourceKind source = AudioClockSourceKind::kSystem;

    /// 本轮无法推进会冻结在最后已知位置。为 true 时 `position_ns` 是陈旧值，
    /// 上层可据此降级（例如暂停追帧），但不许把它当成"时钟停了"。
    bool stale = false;

    /// 本轮候选值小于历史最大值，被单调守卫拒绝。
    bool monotonic_rejected = false;

    /// 本轮候选值超前历史最大值过多，被钳制。
    bool jump_clamped = false;
};

}  // namespace kivo::video::core::audio
