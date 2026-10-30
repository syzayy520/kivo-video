#pragma once

#include <cstdint>

namespace kivo::video::core::pace {

/// 一段播放里，那条斜率**到底累积成了多少**。
///
/// 存在的理由是本项目说错过一句话：`slope_detectable = true` 被读成"漂移要紧"。
/// 它不是。它只说"在 95% 置信带之内，这个斜率与 0 分得开"——
/// P13-K 的 r3（35 s / 700 点）就报过 `detectable = true`，而那个斜率
/// 0.48 ppm 在 35 秒里只累积 **17 µs**，比一次 vsync 抖动还小。
/// 分得开 ≠ 要紧：**看得见的东西可以小到无关，看不见的东西也可能大到出事**。
///
/// 所以这个单元只做一件事：把 `ppm` 积回纳秒，再对着一个写明出处的额度比一次。
/// 它不改斜率、不改 SE、不改 CI —— 那些是 `pace_drift_trend` 的事。
///
/// 三态而不是布尔，是因为 `false` 在布尔里承载两种相反的含义：
/// "低于预算"和"根本估不出来"。前者是结论，后者是没有结论，
/// 印成同一个 `false` 等于让一次失败伪装成一次通过 —— 这个项目为此栽过多次。
enum class DriftVerdict : std::int8_t {
    /// 序列不可用（样本 / 跨度不够），或输入不是有限数。
    /// **不是**"漂移很小"—— 那个读法会把"量不出来"包装成"没问题"。
    kUnknown = 0,

    /// 保守上界仍低于额度：这段播放的漂移大概率听不出来。
    kWithinBudget = 1,

    /// 保守上界越过额度：不必等到真听见，这份证据已经该被拦下。
    kOverBudget = 2,
};

/// 线格式名字。三态在测试里逐字钉死 —— 改一个字节就是换了一份 schema。
[[nodiscard]] const char* drift_verdict_name(DriftVerdict verdict) noexcept;

/// 额度：**45 ms**，取 ITU-R BT.1359 的"音频超前可检测阈值"。
///
/// 为什么不是 125 ms（该标准里音频滞后方向的那个数）：
/// 两个方向取**较严**的一侧，是故意的保守 —— 宁可把一份"刚好听得出来"的
/// 证据判成超额度，也不要漏掉一份真的会被人耳抓到的。
///
/// 为什么不用"40 ms 体验上限"这种业内口径：那类数字没有可引的文档出处，
/// 写进证据就成了一句无法核对的断言。有标准就用标准，哪怕它偏严。
///
/// 还有一条必须说清：45 ms 是**检测阈值**（刚能察觉），不是"可接受"。
/// 低于它只代表"大概率听不出偏移"，不代表体验合格。
inline constexpr std::int64_t kAudibleThresholdNs = 45'000'000;

/// `slope_ppm ± 1.96 · SE` 的那个系数。与 `pace_drift_trend.cxx`、校验器
/// `CI_Z` 必须是同一个数 —— 三处不一致时，"报告说 CI 是 A、校验器算出 B"
/// 会让两边都自称正确。
inline constexpr double kConfidenceZ = 1.96;

/// 累积漂移与它的判读。**事实与结论分成两组字段**，谁也不替谁说话。
struct DriftAccumulation {
    /// 累积量算出来了没有。`false` ⇒ 其余数值字段无意义（不是零）。
    ///
    /// 与 `verdict == kUnknown` 不冗余：序列不可用时这里仍可为 `true`
    /// （数算得出来，只是没有资格下结论）。
    bool estimated = false;

    /// 点估计：`slope_ppm × 跨度_s`，单位 ns，**带符号**（正 = 越来越晚）。
    ///
    /// 它是"这段窗口内漂了多少"，**不是整场**：斜率只在 `trend.span_ns`
    /// 那段成立，拿它去乘整场时长是外推，而外推出来的数没有证据支撑。
    /// 一份 600 秒的片源若趋势只覆盖最后 102 秒，这里说的也只是那 102 秒。
    std::int64_t accumulated_ns = 0;

    /// 保守上界：用置信带里**离 0 更远**的那一端积出来的绝对值，非负。
    ///
    /// 为什么要用远端而不是点估计：读者问的是"最坏能坏到哪"。
    /// 为什么取绝对值最大而不是 `ci_high`：早 45 ms 与晚 45 ms 一样糟，
    /// 方向不该影响"会不会被听出来"这个判定。
    ///
    /// ⚠️ 它只覆盖**斜率的抽样误差**，不覆盖模型失配：序列若不是直线
    /// （中途跳一下、换向），这条斜率本身就描述错了对象。
    /// 那种情况由 `slope_halves_agree` 报警，不是靠把上界放大能救的。
    std::int64_t accumulated_upper_ns = 0;

    /// 判定用的额度，恒等于 `kAudibleThresholdNs`。
    ///
    /// 放进结构体而不是让读者去翻常数：判定和它的尺子必须在一起。
    /// 一个 `over_budget` 旁边没有额度，等于一句没有刻度的断言。
    std::int64_t threshold_ns = kAudibleThresholdNs;

    DriftVerdict verdict = DriftVerdict::kUnknown;
};

/// 把 `ppm` 积回纳秒，并对额度判一次。
///
/// 输入全部来自 `DriftTrend`：`slope_ppm`、`slope_ppm_std_error`（HAC 值）、
/// `span_ns`（**窗口**跨度）、`usable`。不读时钟、不分配、不改任何入参。
///
/// `usable == false` 时照样把数算出来（纯事实），但 `verdict` 强制 `kUnknown` ——
/// 一份样本不足的趋势可以进档案，但不能用来宣称"漂移没问题"。
[[nodiscard]] DriftAccumulation accumulate_drift(double slope_ppm,
                                                 double slope_std_error_ppm,
                                                 std::int64_t span_ns,
                                                 bool usable) noexcept;

}  // namespace kivo::video::core::pace
