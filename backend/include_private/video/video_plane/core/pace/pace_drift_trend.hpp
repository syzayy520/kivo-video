#pragma once

#include <array>
#include <cstdint>

#include "video/video_plane/core/pace/pace_drift_budget.hpp"
#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_halves_agreement.hpp"

namespace kivo::video::core::pace {

/// 一次会话的漂移趋势。**纯事实**：窗口中位数、两道斜率、线性度。
///
/// 为什么不放在 `PaceSummary` 里就地算：`PaceSummary` 是"计数与分布"的容器，
/// 加一段回归逻辑进去，它就不再是数据了。这里是纯函数 `summarize(series)`。
///
/// 尺寸刻意压到 ~400 字节：`PaceSummary` 会被整块复制（stop 之后发布一次），
/// 8 个窗口的数组是这个类里唯一"按窗口数量线性增长"的东西，窗口数写死 8。
struct DriftTrend {
    /// 8 个等宽时间窗。60 秒片源 → 每窗 7.5 s / 150 个点，中位数足够稳；
    /// 8 个点画出来人眼就能看出趋势。
    static constexpr std::int32_t kWindowCount = 8;

    /// `usable` 的两个门。为什么是这两条：
    /// - 样本 < 30：分位数和回归都在描述噪声，`r2` 自己也不可信；
    /// - 跨度 < 30 s：0.1% 的漂移在 8 秒里只累积 8 ms，与抖动同量级，**量不出来**。
    ///   短片段硬报一个斜率，等于把噪声包装成结论。
    static constexpr std::int32_t kMinSamples = 30;
    static constexpr std::int64_t kMinSpanNs = 30'000'000'000;

    bool usable = false;

    /// `usable == false` 时非空。静态字面量，不碰 `std::string`。
    const char* unusable_reason = "";

    std::int32_t window_count = 0;      ///< 实际有样本的窗口数（≤ kWindowCount）
    std::int32_t samples_used = 0;      ///< 序列里剩下的点数（≤ 容量）
    std::int64_t span_ns = 0;           ///< 首末样本的媒体时间跨度
    bool truncated = false;             ///< 发生过环形覆盖（只剩最近 102.4 s）
    std::int32_t segments = 1;          ///< 时间轴段数（锚点重建次数 +1，本趋势只覆盖最后一段）
    /// 本段内最宽的空洞（格点，50 ms，P13-M）。0 = 段内没有整格丢失。
    ///
    /// 为什么趋势里要带这个：`span_ns` 是"末 − 首"，**空洞落在里面**。
    /// 一个 10 s 的空洞会让 `span_ns` 多算 10 s，于是 `slope_ppm × span` 的累积量、
    /// 以及 `observed_hz` 都偏。这一格让读者一眼看到那个洞有多宽，
    /// 而不是拿着一个偏了的跨度去乘。
    std::int32_t segment_largest_gap_cells = 0;
    /// 本段空洞没有超过 `DriftSeries::kMaxContiguousGapCells`（20 格 = 1 s）。
    ///
    /// false 时上面那些按跨度算出来的数都要打折看 —— 这条限制写在字段上，
    /// 不写在文档里。
    bool segment_contiguous = true;

    std::int64_t first_window_median_ns = 0;
    std::int64_t last_window_median_ns = 0;
    std::int64_t delta_median_ns = 0;   ///< `last − first`。抗尾部的"粗漂移"读数。

    /// 8 个窗口中位数的中位数（P13-N）。**整批点的一个精确代表值。**
    ///
    /// 为什么需要它：`quantiles.<口径>.p50` 是直方图桶中心，真机上恒为 125 000；
    /// 而窗口精确中位数是 74 835 —— 同一份证据两个 p50 差 66%，JSON 里
    /// 却没有任何字段能阻止读者把它读成"两份测量打架"。这份值给出
    /// **同一批点的精确参照**，让"桶中心偏了多少"变成一个可计算的数
    /// （`bucket_center_bias_ns` = 直方图 p50 − 本字段），而不是一句解释。
    ///
    /// ⚠️ 它不是全体样本的中位数：中位数不可合并，8 个等样本窗才近似成立。
    /// 字段名里带 `of_window_medians` 就是为了不让人把它当成 `p50` 的精确版。
    /// 所以它只在 `window_median_exact == true` 时才有意义 —— 有窗退回桶中心时
    /// 这里混着脏值，报告层那个时候**不出版**它。
    std::int64_t median_of_window_medians_ns = 0;

    /// 逐点最小二乘斜率，单位 **ppm**（1 ppm = 每秒偏 1 µs）。
    ///
    /// 正值 = `lateness` 越跑越大 = 视频越来越晚于音频时钟。
    /// 1000 ppm 就是"每秒偏 1 ms"，10 分钟累积 600 ms —— 人耳能听出来。
    ///
    /// **这个数单看没有意义**，必须与下面的 `slope_ppm_std_error` 一起读：
    /// `r2 = 5.5e-05` 的那份证据照样写着 `-0.022183`，六位小数全是噪声，
    /// 只是没画刻度盘所以看起来像结论（见 `74-p13j-design.md` §一）。
    double slope_ppm = 0.0;

    /// 逐点回归斜率的标准误（ppm）：`sqrt( SSres / (n−2) / Sxx )`。
    ///
    /// 为什么它是本卡的主产出，而不是"顺手加个字段"：
    /// 之前一直在说"漂移量准卡在 0.25 ms 桶宽 ⇒ 7 ppm 上限"，那是**误判**。
    /// 真机上三轮的 8 个窗口中位数全是 125000（桶中心），`slope_from_windows_ppm`
    /// 恒为 0 —— 抗噪的那根尺子根本没有刻度。而真正有分辨力的逐点回归
    /// 只需要一个误差棒，就能说出"漂移在 ±0.15 ppm 之内"，
    /// 比 7 ppm 好一个半数量级。尺子不粗，是缺刻度盘。
    ///
    /// 自 P13-K（`sync/9`）起，这里是 **HAC（Newey–West）修正后**的标准误。
    ///
    /// 上一版这里是 `sqrt(σ̂²/Sxx)`，假定残差独立同分布。`lateness` 不满足这条：
    /// 相邻抽稀样本的残差正相关，有效样本数远小于 `n`，SE 被系统性低估 ——
    /// 后果是 P13-J 的 r3（35 s / 700 点）报出 `detectable = true` 而 `r2 = 0.007`。
    /// 那次误报不是"阈值太松"，是尺子用错了模型（见 `76-p13k-design.md` §一）。
    ///
    /// **字段名没变，语义变了** —— 这是刻意的，也是本卡唯一一个语义变更：
    /// 保留旧名是因为它才是"该用的那个数"，读者不该在 `_iid` 与 `_hac` 之间
    /// 二选一；旧值原地改名成 `slope_ppm_std_error_iid`，不会被谁顺手引用。
    /// 名字不变而语义变，正是这类项目最容易骗过自己的地方，所以它必须
    /// 出现在状态卡的诚实清单第一条。
    double slope_ppm_std_error = 0.0;

    /// 旧口径：`sqrt( SSres/(n−2)/Sxx )`，假设残差独立同分布。
    ///
    /// 留着不是为了兼容，是为了**能对照**：`slope_ppm_std_error_inflation`
    /// 就是这两个数的比值，一个 1.0 的膨胀比意味"自相关没有影响"，
    /// 而 4.0 意味"上一版的 CI 窄了两倍"。没有旧值，读者就无从判断
    /// 这次修正到底动了多少 —— 那会让"我们修好了"变成一句无法检验的声明。
    double slope_ppm_std_error_iid = 0.0;

    /// `Var_HAC / Var_IID`。> 1 表示上一版低估了不确定度。
    double slope_ppm_std_error_inflation = 1.0;

    /// HAC 用的 Bartlett 截断阶数。写进证据是为了可复算（见 `pace_hac.hpp`）。
    std::int32_t hac_lags = 0;

    /// 残差 1 阶自相关，取值 [−1, 1]。这是"为什么要修正"的一句话解释。
    double hac_lag1_autocorrelation = 0.0;

    /// HAC 是否真的估出来了。`false` 时 `slope_ppm_std_error` 沿用 IID 值 ——
    /// 那种情况下证据里必须能看出来（判据 K0），否则"修正过"与"没修正"
    /// 在 JSON 上长得一模一样。
    bool hac_estimated = false;

    /// 名义 95% 置信带：`slope_ppm ± 1.96 × slope_ppm_std_error`。
    ///
    /// 存两端而不是只存半宽：读者要回答的问题从来是"0 在不在里面"，
    /// 一个半宽还要他自己减一遍，多一步就多一次算错的机会。
    double slope_ci_low_ppm = 0.0;
    double slope_ci_high_ppm = 0.0;

    /// 置信带不含 0 ⇒ 在这把尺子的分辨力之内，"有漂移"这个说法成立。
    ///
    /// ⚠️ `true` 只说明**看得见**，不说明看得见的那个数是多少 ——
    /// 量级仍受上面那条自相关低估的约束。`false` 则明确得多：
    /// 它把"0.000 ppm"翻译成"小于 ±CI 半宽"，这才是这个字段存在的意义。
    bool slope_detectable = false;

    /// 用 8 个窗口中位数再回归一遍。**可复算**：只要 8 个 `(中心时刻, 中位数)`。
    ///
    /// 与 `slope_ppm` 不一致 = 序列不是线性的，这时候 `slope_ppm` 不能当漂移用。
    ///
    /// 注意它的分辨力取决于每窗中位数是否精确：中位数若被量化到 0.25 ms 格点，
    /// 这个斜率就只能取 0 或 ±(250 µs / 跨度)。所以它旁边必须跟
    /// `window_median_exact` —— 说一个 0 而不说尺子有几个刻度，是这份证据
    /// 从 P13-D 到 P13-I 一直犯的错。
    double slope_from_windows_ppm = 0.0;

    /// 8 窗中位数是否**全部**由原始 `lateness_ns` 选出（`false` ⇒ 至少一窗退回桶中心）。
    bool window_median_exact = true;

    /// 退回桶中心的窗数。`window_median_exact == false` 时 > 0。
    /// 单独给个数而不是只给 bool：一窗退回和八窗退回，可信度差得远。
    std::int32_t window_median_fallback_windows = 0;

    /// 线性度 `1 − SSres/SStot`。低值说明"斜率"只是在描述噪声。
    /// 光有斜率没有 r2，噪声会被读成漂移 —— 这是本卡最容易犯的错。
    double r2 = 0.0;

    /// 前后半程各拟合一次（各 `n/2` 个点），单位 ppm。
    ///
    /// 为什么要在 HAC 之外再加一对：HAC 回答"这个斜率有多准"，
    /// 但答不了"它是一条持续的漂移，还是中途跳了一下"。两种情形给出同样的
    /// 全序列斜率，而含义完全不同 —— 前者是时钟速率差（会一直累积），
    /// 后者是一次性错位（不会继续变大）。
    ///
    /// 判读：两个半程**同号**才说明序列在整段上朝一个方向走；
    /// 反号则整个斜率只由中途那一跳决定，不能外推。
    double slope_first_half_ppm = 0.0;
    double slope_second_half_ppm = 0.0;

    /// 两个半程斜率是否同号（都 > 0 或都 < 0）。
    ///
    /// ⚠️ 它只说方向，不说大小，也没有阈值：斜率本身接近 0 时符号由噪声决定，
    /// 这时 `true` 与 `false` 都是掷硬币。所以它是**诊断**不是判据 ——
    /// 校验器只验它与两个半程斜率自洽（K6），不拿它下结论。
    bool slope_halves_agree = false;

    /// 两个半程**是不是同一条线**（P13-O）。
    ///
    /// `slope_halves_agree` 只比符号，实测 13 份证据里 8 份报 `true`，
    /// 其中 3 份两半程量级差 5 倍以上 —— 最极端的一份两半程都是正的、
    /// 全序列斜率却是负的，而它照样报 `true`。
    ///
    /// 这一组字段回答的是同一件事的正确问法：**差值相对于差值自身的误差有多大**。
    /// 三个结论（`consistent` / `inconsistent` / `unknown`）里，
    /// 只有 `inconsistent` 会反证掉 `drift_verdict`（降为 `unknown`）。
    HalvesAgreement halves;

    /// ── 累积量：把上面那个 `ppm` 积回纳秒（P13-L）──
    ///
    /// `slope_detectable` 回答的是"与 0 分得开吗"，不是"要紧吗"。
    /// 这两件事被混为一谈是有代价的：P13-K 的 r3 报 `detectable = true`，
    /// 对应的斜率 0.48 ppm 在 35 秒里只累积 17 µs —— 一次 vsync 抖动那么大。
    /// 反过来的情形同样真实：CI 宽到含 0（`detectable = false`），
    /// 但 CI 远端积起来可能已经越过人耳阈值。
    ///
    /// 所以这里给三个数 + 一个三态判定，算法全在 `pace_drift_budget`。
    ///
    /// 点估计：`slope_ppm × span_ns / 1e6`，单位 ns，带符号。
    /// **只覆盖 `span_ns` 这一段**，不是整场 —— 斜率不能拿去做外推。
    std::int64_t drift_accumulated_ns = 0;

    /// 保守上界：置信带里离 0 更远的那一端积出的绝对值。非负。
    /// 只覆盖斜率的抽样误差，**不覆盖模型失配**（半程不一致时这条斜率本就错了对象）。
    std::int64_t drift_accumulated_upper_ns = 0;

    /// 判定额度，恒等于 `kAudibleThresholdNs`（45 ms，ITU-R BT.1359 检测阈值）。
    /// 放进结构体是为了让判定与它的尺子出现在同一处。
    std::int64_t drift_budget_ns = kAudibleThresholdNs;

    /// 三态：`unknown` / `within_budget` / `over_budget`。
    /// `unknown` 不是"没问题"——它明确表示这份趋势没有资格下这个结论。
    DriftVerdict drift_verdict = DriftVerdict::kUnknown;

    std::array<std::int64_t, kWindowCount> window_start_ns{};
    std::array<std::int64_t, kWindowCount> window_median_ns{};
    std::array<std::int32_t, kWindowCount> window_samples{};
};

/// 从序列算趋势。**纯函数**：不改序列、不读时钟、不分配。
///
/// 序列为空或只有 1 个点时返回 `usable = false` 且 `unusable_reason` 非空 ——
/// 不许返回一个"斜率 0"让读者误以为"没有漂移"。
[[nodiscard]] DriftTrend summarize_drift(const DriftSeries& series) noexcept;

}  // namespace kivo::video::core::pace
