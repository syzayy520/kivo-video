#pragma once

#include <cstdint>

#include "video/video_plane/core/pace/pace_measurability.hpp"

namespace kivo::video::core::pace {

/// 门限在**这一次会话里**的换算（P14-V）。
///
/// `drift_limit_ns` 是 45 ms，这个数在每一份证据里都一样 —— 于是它读起来像
/// "容差恒定"。它不是：门守的是**累积漂移**，而累积漂移 = 速率误差 × 时长。
/// 所以同一个 45 ms 在 8 s 的会话里等价于 5 648 ppm，在 1 800 s 的会话里等价于
/// 25 ppm，摆 226 倍（`157-p14v-design.md` §2 的全仓普查）。
///
/// 25 ppm 已经落进普通石英晶振的精度区间：**播 30 分钟，一个完全正常的时钟就会撞门**。
/// 这句话从来没有人算过，因为报告里 `limit_ns` 与 `elapsed_span_ns` 虽然并排印着，
/// 却从不互相比。
///
/// 本结构只做换算，**不改任何阈值**。改 `drift_limit_ns` 是调参，
/// `pace_policy.hpp` 第 11 行写明"调参必须另开卡并给出对照数据"。
struct GateScale {
    /// 门限等价速率（ppm）：`limit_ns × 1e6 / span_ns`。
    ///
    /// **`ppm.measured == false` 时这一格不可读作 0。** 0 ppm 意味着
    /// "门严到一点点速率误差都不许有"，正好是最容易被误读的那种假观测 ——
    /// 与 `MeasuredCounter` 存在的理由同一个病。
    double limit_ppm_equivalent = 0.0;
    MeasuredCounter ppm{};

    /// 以 `media_rate` 的实测偏差，多久会撞到门（秒）。
    ///
    /// 与 `limit_ppm_equivalent` 回答**不同**的问题，所以分成两格：
    /// 前者是"门限对应多严"，后者是"这次会话实际会用多久撞上"。
    /// 8 s 窗口上 ppm = 5 648（看着很宽松），但 `media_rate ≈ 1`，
    /// 于是 `reach` 不可算 —— 整段都撞不到。两格一起读才不会被任一个误导。
    double limit_reach_s = 0.0;
    MeasuredCounter reach{};
};

/// 门限换算。**纯函数**，零状态，不依赖 `PaceStats` / `PacePolicy`。
///
/// 参数全部是现成数据：`limit_ns` 与 `span_ns` 来自 `PaceSummary`，
/// `media_rate` 来自 P14-S 的 `timeline.media_rate`（分子是实测 pts）。
/// 本卡不新增任何测量。
/// @param drift_rebases 本轮门复位过几次（`PaceSummary::drift_rebases`）。
///        非 0 ⇒ 观测速率已被修正污染，`limit_reach_s` 一律判"不可算"并给理由。
[[nodiscard]] GateScale gate_scale(std::int64_t limit_ns,
                                   std::int64_t span_ns,
                                   double media_rate,
                                   bool media_rate_measured,
                                   std::int64_t drift_rebases = 0) noexcept;

/// 速率偏差小于这个值就不算"撞门时间"。
///
/// 45 ms / 1 ppm = 45 000 s = 12.5 小时。比任何一次真机会话都长两个数量级，
/// 算出来也是一个没有信息量的巨大数字，不如说清"这个量程回答不了"。
inline constexpr double kMinReachDeviation = 1e-6;

/// `limit_ns <= 0`：门没配置，没有东西可换算。
inline constexpr const char* kNoGateLimitReason =
    "drift gate limit is unset (0), so there is no tolerance to convert";

/// `span_ns <= 0`：**这一句就是本卡的全部论点**。
///
/// 45 ms 离开时长就没有意义 —— 它是"多长时间内允许攒下 45 ms"，
/// 而时长没记录时，那句话读不出任何东西。
inline constexpr const char* kNoSpanReason =
    "no wall-clock span recorded (elapsed_span_ns == 0): the same 45 ms means a "
    "different rate tolerance in every session length, so without a span there is "
    "nothing to convert";

/// `media_rate_measured == false`：算不出撞门时间。
inline constexpr const char* kRateUnmeasuredReason =
    "media rate was not measured for this session, so the time to reach the gate "
    "cannot be derived";

/// 门已经动作过（P14-W）：观测到的是**修正后**的净速率。
///
/// 这一句是 P14-W 撞出来的：60 s 片源上跑 5 000 ppm，门在第 9 秒就动手，
/// 之后 `media_rate` 被压回 1.0007 —— 于是 `limit_reach_s` 算出 63.6 s，
/// 而**实际上门已经动作了 49 次**。这个数描述的是"残余速率还要多久才撞门"，
/// 与"注入的速率多久会撞门"是两个问题；并排印着就会被读成后者。
inline constexpr const char* kGateActedReason =
    "the gate already corrected this session, so the observed media rate is the "
    "post-correction rate: the time it would take to reach the limit describes the "
    "residual, not the disturbance that caused it";

/// 速率偏差小于 `kMinReachDeviation`：撞门要 12 小时以上。
inline constexpr const char* kRateTooCloseReason =
    "media rate is within 1 ppm of nominal, so reaching the gate would take more "
    "than twelve hours";

}  // namespace kivo::video::core::pace
