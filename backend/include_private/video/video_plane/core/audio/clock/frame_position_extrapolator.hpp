#pragma once

#include <cstdint>

namespace kivo::video::core::audio {

/// 把"某个时刻的帧位置"外推到"现在"。
///
/// `getTimestamp` 每个 burst 才更新一次（K40 上 burst 192 帧 = 4 ms）。
/// 若把原始帧位置直接当时钟，同一个 burst 内查十次会拿到十个相同值，
/// 时钟呈阶梯状 —— 对 40 ms 阈值不致命，但会让 C5 的 `due_at` 带上
/// 4 ms 的量化抖动，白白吃掉预算。
///
/// 纯数学、零平台依赖。**调用方必须传与时间戳同一时钟刻度下的 `now`**
/// （AAudio 的 timestamp 用 CLOCK_MONOTONIC，就必须用 clock_gettime(CLOCK_MONOTONIC) 取 now，
/// 不能用 std::chrono::steady_clock —— 标准不保证两者同源，差 1 ms 外推方向就会反）。
class FramePositionExtrapolator {
public:
    /// 一次时间戳样本。
    struct Sample {
        std::int64_t frame_position = 0;  ///< 已呈现帧数
        std::int64_t time_nanos = 0;      ///< 该帧位置对应的时钟刻度
        std::int32_t sample_rate = 48000;
    };

    struct Result {
        std::int64_t position_ns = 0;
        std::int64_t extrapolated_ns = 0;  ///< 本次外推了多少（0 表示未外推）
        bool stale = false;                ///< 时间戳过旧，已停止外推
    };

    /// `stale_after_ns` 建议取 2 个 burst 的时长：
    /// 一个 burst 的陈旧是正常的（下一个还没来），两个 burst 还没更新就是真的卡了。
    explicit FramePositionExtrapolator(std::int32_t sample_rate = 48000,
                                       std::int64_t stale_after_ns = 8'000'000) noexcept;

    [[nodiscard]] Result extrapolate(const Sample& sample, std::int64_t now_mono_ns) noexcept;

    /// 切换采样率，并**保证位置连续**。
    ///
    /// 易错点：位置 = frame × 1e9 / rate，rate 一变，同一个 frame 会映射到不同时间
    /// （48000 帧在 48k 下是 1000 ms，切到 44.1k 就成 1088 ms，直接跳 88 ms）。
    /// 因此切换时把基准挪到"最后已知帧位置"上，之后只按增量用新速率换算。
    ///
    /// 返回是否真的发生了切换（同值直接返回 false）。
    bool set_sample_rate(std::int32_t sample_rate) noexcept;

    /// 更新陈旧阈值（切采样率后两个 burst 的时长会变，要同步）。
    void set_stale_after_ns(std::int64_t stale_after_ns) noexcept;

    /// 重设"帧位置 ↔ 媒体时间"的基准（P12-F2，seek 之后调用）。
    ///
    /// 为什么需要它：`position_of_frame()` 算的是 `base_position_ns_ + Δframe`，
    /// 而停/启音频流会让 AAudio 的帧位置归零。不重设基准的话，seek 之后
    /// 时钟会从 0 起算 —— 视频被节拍器拖回片头，而所有计数看起来都正常。
    ///
    /// 基准取 `last_frame_`（当前流已呈现的帧数），所以**调用后位置立刻等于新基准**，
    /// 之后按新流的增量前进。seek 到位那一帧就是 `media_position_ns`。
    void rebase(std::int64_t media_position_ns) noexcept;

    [[nodiscard]] std::int32_t sample_rate() const noexcept { return rate_; }
    [[nodiscard]] std::int64_t stale_freezes() const noexcept { return stale_freezes_; }

private:
    [[nodiscard]] std::int64_t position_of_frame(std::int64_t frame, std::int32_t rate) const noexcept;

    std::int64_t base_frame_ = 0;
    std::int64_t base_position_ns_ = 0;
    std::int32_t rate_ = 48000;
    std::int64_t stale_after_ns_ = 8'000'000;

    std::int64_t last_frame_ = 0;
    std::int64_t stale_freezes_ = 0;
};

}  // namespace kivo::video::core::audio
