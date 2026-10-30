#pragma once

#include <cstdint>

namespace kivo::video::core::pace {

/// 一份分位数是**按什么节奏采出来的**。
///
/// 存在的理由只有一个：`p99 = 1.6 ms` 这句话本身没有对错。
/// 逐帧采时 60fps 的样本密度是 30fps 的两倍，尾巴自然更容易被摸到，
/// 于是同一台机器、同一段 8 秒能读出"60fps 更差"——差的是看得更清楚，不是播得更差。
/// 所以一个分位数**必须**和它的采样口径绑在一起才有意义（见 `63-p13d-status.md` §九）。
///
/// 名字是**线格式**（写进 JSON 的字符串），不是给人看的注释。
/// 改一个字节就等于换了一份 schema，因此 `p13e_sampling_mode_tests` 把三个名字
/// 逐字钉死 —— 谁想改，先改测试，再谈兼容。
enum class SamplingMode : std::int8_t {
    /// 按媒体时间抽稀到 20 Hz。**唯一**允许放进"30fps vs 60fps"那张表的口径。
    kMediaTimeDecimated = 0,

    /// 每一帧都进分布。能回答"实际呈现稳不稳"，但**不能**跨帧率比。
    kPerFrame = 1,

    /// 这一轮根本量不出来（没音轨 / 时钟是 system / 走了 surface_at_time）。
    ///
    /// 单独列成一个模式，而不是"没有模式"：缺席也要有一个名字，
    /// 否则调用方分不清"没采"和"采了但是零"。
    kNotMeasured = 2,
};

/// 线格式名字。三个返回值在测试里逐字比对，不许"顺手改得更好读"。
[[nodiscard]] const char* sampling_mode_name(SamplingMode mode) noexcept;

/// 这个口径的分位数能不能跨帧率比较。
///
/// 只有 `kMediaTimeDecimated` 为真。写成函数而不是让调用方写
/// `mode == kMediaTimeDecimated`，是为了让"能不能比"这件事只有一个答案 ——
/// 散落在各处的等价判断，迟早会有一处写反。
[[nodiscard]] bool comparable_across_frame_rates(SamplingMode mode) noexcept;

/// 抽稀块成立所需的**最少**样本数（P13-G）。
///
/// 为什么是 2 而不是 30：
/// - 2 是**结构门** —— 至少要有两点才能证明"媒体时间在推进"；
/// - 30 是**统计门**（`DriftTrend::kMinSamples`）—— 管斜率可不可信。
///
/// 两件事用一个阈值管，会让"1.2 秒片源的 p99 有意义、斜率没有"这个
/// 真实情况被一刀切掉。1 个样本连"推进"都证明不了：它的 p50/p95/p99/max
/// 四个数完全相同，看起来完全合法（见 `68-p13g-design.md` §二）。
///
/// **这个是线格式契约的一部分。** 改它等于改判据 G1，不是调参。
inline constexpr std::int32_t kMinDecimatedSamples = 2;

/// 抽稀块是否成立。
///
/// `samples` 与 `span_ns` 必须来自**同一份**统计（`PaceSummary` 的
/// `sampled_samples` 与 `drift.span_ns`），不许一处取本轮、一处取上轮。
[[nodiscard]] bool decimated_block_is_usable(std::int32_t samples,
                                             std::int64_t span_ns) noexcept;

/// 抽稀不成立时的**真实**原因，给 `sampling.reason` 用。
///
/// 存在的理由是本项目栽过的一次：`kPerFrame` 分支曾写
/// `"no media pts was supplied to note_presented()"`，
/// 而那条路径**结构上不可达**（缺 pts 会静默产出 1 个样本，不会降级）。
/// 一个编造的 reason 会把排查引到反方向。
///
/// 把"为什么"收成一个函数，编造就没有位置 —— 想写第二个解释，
/// 就得先改这里，而这里被单测逐字钉着。
///
/// 返回静态字面量，不碰 `std::string`：它会被写进每次出报告的路径。
[[nodiscard]] const char* decimated_unusable_reason(std::int32_t samples,
                                                    std::int64_t span_ns) noexcept;

}  // namespace kivo::video::core::pace
