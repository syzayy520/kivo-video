#pragma once

#include <cstdint>

#include "video/video_plane/core/pace/pace_measurability.hpp"

namespace kivo::video::core::pace {

/// 视频相对**墙钟**的偏移摘要（P14-U）。
///
/// 定义：`W_k = present_k − (mono_0 + pts_k − anchor_pts) = lateness_k − lead_k`。
/// 正数 = 画面比"墙钟按 pts 排布时该出现的时刻"晚。
///
/// **它不叫 `vs_audio`，因为没测到 DAC。** 项目里只有一个时钟源（AAudio 时间戳），
/// 而那只钟正是本卡要绕开的那只。`vs_wall` 是"用墙钟当耳朵的替身"，
/// 替身的可信度由 `wall_reference_bound_ns` 明码标出 —— 名字说得比做的多，
/// 就是 P14-T 抓到的那个病。
///
/// **不建直方图**：`W = L − lead`，`lead` 是缓变或阶跃，所以 `W` 的形状
/// 几乎就是 `lateness` 的形状整体平移。再养 1024 桶 + 抽稀序列 + 趋势回归，
/// 是为一个已能由两个数算出来的东西付三份代价（同一个判断见
/// `pace_stats.hpp` 里 P12-C8 那段：`av_drift` 刻意没有直方图）。
/// 首末 + 最差三格足够回答"差多少、在哪儿"。
struct WallOffset {
    /// 喂进来的帧数。0 时整块无意义。
    std::int64_t samples = 0;

    /// 第一帧的偏移。**不是 0**：第一帧相对基准帧已经有一个固定的交出延迟，
    /// 那个延迟就是锚点吸收不掉的那一帧。判据要的是"首末之差"，不是"首值是否 0"。
    std::int64_t first_ns = 0;
    std::int64_t last_ns = 0;

    /// `|W|` 最大的那一帧。**带媒体时刻**——"在片头还是片尾"决定结论完全不同
    /// （冷启动可忽略 / 片尾收尾不是能力问题 / 片中要修），与 `DriftExtreme` 同一条理由。
    std::int64_t worst_ns = 0;
    std::int64_t worst_media_ns = 0;
    /// 第几个样本（0 基）。与 `worst_media_ns` 一起定位。
    std::int64_t worst_index = 0;

    /// 可测性。`samples == 0` 时 `false`，理由在 `reason` 里。
    MeasuredCounter measured{};
};

/// 偏移摘要的累加器。**非线程安全**，只该在呈现线程里用。
class WallOffsetRecorder {
public:
    /// 喂一帧。`offset_ns` 是已经算好的 `lateness − lead`，`media_ns` 是本帧 pts。
    void note(std::int64_t offset_ns, std::int64_t media_ns) noexcept;

    [[nodiscard]] std::int64_t last_ns() const noexcept { return last_ns_; }

    /// 换时间轴时清空（P15-A2）。与 `LeadTraceRecorder::restart()` 同一条理由：
    /// `vs_wall` 是"这一段视频相对墙偏了多少"，跨 seek 累加会把两段的首末
    /// 拼成一个不存在的跨度 —— 而它印出来的 `last_ns` 会被当成"此刻偏多少"读。
    void restart() noexcept {
        samples_ = 0;
        first_ns_ = 0;
        last_ns_ = 0;
        worst_ns_ = 0;
        worst_media_ns_ = 0;
        worst_index_ = 0;
        has_worst_ = false;
    }

    [[nodiscard]] WallOffset summary() const noexcept;

private:
    std::int64_t samples_ = 0;
    std::int64_t first_ns_ = 0;
    std::int64_t last_ns_ = 0;
    std::int64_t worst_ns_ = 0;
    std::int64_t worst_media_ns_ = 0;
    std::int64_t worst_index_ = 0;
    bool has_worst_ = false;
};

/// 墙钟当作"耳朵替身"时的误差上界（ppm）。
///
/// **这是一个假设，不是实测。** 消费级晶振（含 SoC 的音频域与 `CLOCK_MONOTONIC`
/// 所用的那两颗）的频差在 ±50 ppm 量级；这一族从来没有直接测过两者之差
/// —— 要测得有一个"真正播到第几帧"的独立信源，而项目里没有。
///
/// 所以它被出版成报告里的一格（`wall_reference_bound_ppm`）而不是写死在文档里：
/// 争论这个数的人应该能看见它、改它、并立刻看到 `bound_ns` 跟着变。
inline constexpr double kWallReferencePpm = 50.0;

/// 由帧长与墙钟跨度算出偏移的误差上界（ns）。
///
/// `max(一帧, ppm × 跨度)`：短窗口上一帧的量化误差远大于频差，
/// 长窗口（> 5 分钟）上频差才成为主项。两项都算出来取大，不拍脑袋选一个。
[[nodiscard]] std::int64_t wall_reference_bound_ns(std::int64_t frame_duration_ns,
                                                  std::int64_t span_ns) noexcept;

}  // namespace kivo::video::core::pace
