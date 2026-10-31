#pragma once

#include <cstdint>

#include "video/video_plane/core/pace/pace_measurability.hpp"

namespace kivo::video::core::pace {

/// 领先量走过的形状（P14-U）。
///
/// **四态不是三态**：`kUnknown` 必须与 `kSteady` 分开。两者在报告里都表现为
/// "领先量很小"，但一个是"量到了、很稳"，另一个是"根本没量"——
/// 而 P14-L 整整一张卡就是为这一件事立的（`MeasuredCounter` 那一族）。
/// 把"没量到"折进"很稳"，是把假观测写成好消息的最短路径。
enum class LeadVerdict {
    /// 帧数不足两帧，或调用方没给帧长 —— 这一块一个数都不许读。
    kUnknown,
    /// 领先量全程在一帧以内。此时 `av_drift` 可以按"音画偏差"读。
    kSteady,
    /// 没有单次跳变，但领先量匀速长到了一帧以上 —— 钟在跑偏，`rate_ppm` 是那个速率。
    kRamped,
    /// 相邻两帧之间领先量跳了不止一帧 —— 钟**跳**过，`max_step_ns` 是台阶大小。
    kStepped,
};

/// 一次节拍会话里"钟相对墙多走了多少"的轨迹摘要（P14-U）。
///
/// 定义：`lead_k = elapsed_k − (mono_k − mono_0)`，`mono_0` / `elapsed_0` 取
/// **第一个带钟读数的呈现帧**（自动立基准）。正数 = 钟比墙快。
///
/// **为什么需要它，而不是继续用 `PaceSummary::clock_lead_ns`**：
/// 那一格是整场两个跨度相减，只有首末两点。它能答"多走了多少"，
/// 答不出"是一次跳的还是一路走偏的" —— 而这两件事的修法完全不同
/// （跳 = 设备报了个位移过的位置；跑偏 = 采样率换算错了）。
/// `max_step_ns` 与 `step_frames` 就是那个分辨力。
struct LeadTrace {
    /// 参与统计的呈现帧数。**< 2 时整块无意义**（`verdict` 必为 `kUnknown`）。
    std::int64_t samples = 0;

    /// 判"这是不是一步"的尺子。0 = 调用方没给，此时分不出阶跃与渐变。
    ///
    /// 用一帧而不是某个绝对毫秒：一帧是"人能分辨的最小错位"，
    /// 而 24/30/60 fps 的一帧差 2.5 倍 —— 写死 20 ms 会让 60 fps 上的
    /// 一次真跳变（16.7 ms）落进容差里，正是 P14-T 变异 V3 栽的那个坑。
    std::int64_t frame_duration_ns = 0;

    /// 基准帧的两侧读数。出版它们是为了让 `last_ns` 能被**复算**：
    /// `lead_last = elapsed_last − elapsed_0 − (mono_last − mono_0)`。
    /// 不给基准而只给结果，读者就只能选择相信。
    std::int64_t baseline_elapsed_ns = 0;
    std::int64_t baseline_mono_ns = 0;

    /// 末帧的领先量。与 `clock_lead_ns` 同量不同口径（那个用 arrive 首末、这个用 present 首末），
    /// 两者之差应当在一帧以内 —— 判据 U1 守这条。
    std::int64_t last_ns = 0;
    std::int64_t max_ns = 0;
    std::int64_t min_ns = 0;

    /// **相邻两帧**领先量的最大绝对变化。阶跃的直接证据。
    ///
    /// 与 `max_ns − min_ns` 不是一回事：那是全场的摆幅，累积 300 帧、
    /// 每帧涨 1 ms 也能到 300 ms，而每一步都只有 1 ms。
    /// 要判"跳"必须看**步**，不能看**幅**。
    std::int64_t max_step_ns = 0;
    /// `|Δlead| > frame_duration_ns` 的帧数。0 = 没有任何一步超过一帧。
    std::int64_t step_frames = 0;

    /// 基准帧到末帧的**墙钟**跨度。`rate_ppm` 的分母。
    std::int64_t span_ns = 0;

    /// `last_ns / span_ns` 换成 ppm。`span_ns == 0` 时为 0，且 `verdict` 必为 `kUnknown`。
    ///
    /// 这一格是"频差解释"能不能成立的唯一入口：消费级晶振在 ±50 ppm 量级，
    /// 超过 2000 ppm 的偏移**不可能**是速率（判据 U4）。
    double rate_ppm = 0.0;

    LeadVerdict verdict = LeadVerdict::kUnknown;

    /// 非空，且是**静态字面量**（这些理由整块复制，见 `MeasuredCounter` 的取舍）。
    /// 数字不写进这句话，放在 `max_step_ns` / `rate_ppm` 里 —— 两处都印数字，
    /// 迟早一处改了一处没改。
    const char* verdict_reason = kUnspecifiedReason;

    /// 与 `verdict` 同源：`kUnknown` 即未量到。**不另行判断**——
    /// 两处各判一次，迟早在某个边界上分家（`over_budget_measured` 的注释记着同一件事）。
    MeasuredCounter measured{};
};

/// 领先量轨迹的累加器。**非线程安全**，只该在呈现线程里用（与 `PaceStats` 一致）。
///
/// 只有一个写入口 `note()`：立基准、算当前值、更新极值与步长全在同一次调用里完成。
/// 拆成 `begin()` + `add()` 两步的版本被否掉了 —— 那会让"忘了立基准"成为一种可编译的状态。
class LeadTraceRecorder {
public:
    /// 喂一帧的钟读数，返回**这一帧**的领先量。
    ///
    /// @param elapsed_ns 时钟自称走过的媒体时间（`DueStep::clock_elapsed_ns`）
    /// @param mono_ns    同一时刻的墙钟读数（`DueStep::clock_mono_ns`）
    ///
    /// 首帧无条件立基准并返回 0。**调用方必须只喂带钟读数的帧**
    /// （`clock_mono_ns != 0`）：`surface_at_time` 档不带读数，
    /// 喂进来 480 个 0 会让 `max_step_ns = 0`、`verdict = kSteady` —— 一个漂亮的假观测。
    [[nodiscard]] std::int64_t note(std::int64_t elapsed_ns, std::int64_t mono_ns) noexcept;

    /// 帧长（判"是不是一步"的尺子）。可在任意时刻设置，`summary()` 时才用到。
    void set_frame_duration(std::int64_t frame_duration_ns) noexcept {
        frame_duration_ns_ = frame_duration_ns;
    }

    /// 最近一帧的领先量。没喂过帧时为 0。
    [[nodiscard]] std::int64_t current_ns() const noexcept { return has_baseline_ ? last_lead_ns_ : 0; }

    [[nodiscard]] LeadTrace summary() const noexcept;

private:
    bool has_baseline_ = false;
    bool has_previous_ = false;
    std::int64_t baseline_elapsed_ns_ = 0;
    std::int64_t baseline_mono_ns_ = 0;
    std::int64_t last_mono_ns_ = 0;
    std::int64_t previous_lead_ns_ = 0;
    std::int64_t last_lead_ns_ = 0;
    std::int64_t max_lead_ns_ = 0;
    std::int64_t min_lead_ns_ = 0;
    std::int64_t max_step_ns_ = 0;
    std::int64_t step_frames_ = 0;
    std::int64_t samples_ = 0;
    std::int64_t frame_duration_ns_ = 0;
};

}  // namespace kivo::video::core::pace
