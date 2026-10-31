#pragma once

namespace kivo::video::core::pace {

/// 判据 P11 用来抓"没量到却说不出为什么"的那句证词。
///
/// 它同时是 `MeasuredCounter::reason` 的**默认值**：任何一条链忘记填写
/// （新加一个派生量、忘了在 `summary()` 里赋值），出来的就是这个组合
/// `{false, "unmeasured without a recorded reason"}` —— 会被判据当场抓出来，
/// 而不是以 `{false, ""}` 或 `{true, ""}` 混过去。
inline constexpr const char* kUnspecifiedReason = "unmeasured without a recorded reason";

/// 一个**派生**计数「到底量到没有」。
///
/// 存在的理由是一整类字段：它们的值由另一个分布算出来，上游没样本时它们
/// 必然是 0 —— 而 0 看起来永远是好消息。
///
/// 真机证据（`docs/evidence/p14k/p14k-4k60.json`，`surface_at_time`）：
///
///   lateness_ns.samples   = 0      ← 一个样本都没有
///   lateness_ns.measured  = false  ← P14-K 才补上这一格
///   over_budget_frames    = 0      ← 从上面那个空直方图派生，读起来像"没有超预算帧"
///
/// G6 的判据就是 `over_budget_frames`，所以在默认排程档上 **G6 从未被验证过**，
/// 而它印出来的 0 让每一份证据都像是在通过。P14-K 修了 `lateness` 那一格，
/// 但 `over_budget_frames` 同一个病还在 —— 这张卡（P14-L）把它一次治完。
///
/// 为什么 `reason` 是 `const char*` 而不是 `std::string`：这些理由全是静态字面量，
/// 而 `PaceSummary` 每次取样都会被整块复制（每秒数次）。与
/// `DriftTrend::unusable_reason` 同一取舍。
struct MeasuredCounter {
    /// 量到了没有。`false` 时同名的那个整数字段**不可读作 0**。
    bool measured = false;

    /// 空串 = 量到了。非空 = 一句话说清为什么没量到。
    ///
    /// 只由 `measured_ok()` / `measured_missing()` 写入，所以 `measured == true`
    /// 时它必然是空的 —— 「量到了但附了一条理由」这种自相矛盾的组合在结构上不可能出现。
    const char* reason = kUnspecifiedReason;
};

/// 一个计数**量到了**。
[[nodiscard]] constexpr MeasuredCounter measured_ok() noexcept { return MeasuredCounter{true, ""}; }

/// 一个计数**没量到**，`reason` 说清为什么。
///
/// `reason` 为空时返回
/// `"unmeasured without a recorded reason"` —— 那不是一句解释，是一句**证词**：
/// 它会被判据 P11 专门抓出来。留一个空理由，比留一个 0 更容易骗过读者。
[[nodiscard]] MeasuredCounter measured_missing(const char* reason) noexcept;

/// `present_at` 时间戳一条都没有时的理由。
///
/// 三条派生链（`lateness` → `over_budget_frames`、`lateness` 的分位数）
/// 共用这一句，**不各写一遍**：两处字面量迟早会被改成两句略有出入的话，
/// 而那时读者会以为这是两个不同的原因。
inline constexpr const char* kNoPresentTimestampReason =
    "no present-timestamp samples: in surface_at_time the compositor decides when a "
    "frame is shown, so only the submit lead (schedule_lead_ns) is observable";

/// 一帧都没决策过（`frames_decided == 0`）。
///
/// `catch_up_events` 与 `catch_up_frames` 在这种情况下恒为 0，
/// 而那个 0 不说明"没有追赶"，只说明"节拍器一次都没跑"。
inline constexpr const char* kNoDecisionsReason =
    "no frame was ever decided, so no pace verdict was reached and no catch-up could occur";

/// 分母为 0：`drop_ratio = dropped / (presented + dropped)`。
///
/// 上一版在这种情况下直接报 `0.0`，而 0.0 是**最好**的丢帧率 ——
/// 一份什么都没播的证据因此拿到了满分。
inline constexpr const char* kNoDenominatorReason =
    "no frame was presented or dropped, so the drop ratio has no denominator";

}  // namespace kivo::video::core::pace
