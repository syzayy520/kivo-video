#pragma once

#include <cstdint>

#include "video/video_plane/core/pace/pace_drift_budget.hpp"

namespace kivo::video::core::pace {

/// 两个半程斜率**是不是同一条线**。
///
/// 存在的理由是本项目的一个布尔说过三次谎。`slope_halves_agree` 的定义只有一句
/// 「同号」，而真机 13 份证据里 8 份报 `true`，其中 3 份的两个半程量级差 5 倍以上：
///
///   p13k-130s-seek-r1   +1.4417 / +0.1870   差 7.7 倍   agree = true
///   p13l-35s-r1         −2.9145 / −0.2173   差 13.4 倍  agree = true
///   p13n-fwd-r1         +0.5975 / +0.0276   差 21.7 倍  agree = true
///
/// 最后一份最刺眼：两个半程都是正的，全序列斜率却是 **−0.0015**。
/// 真实情形是「前 65 秒在漂、后 65 秒停了」，中间那一跳把整条回归线的符号翻了过来。
/// 而 `agree = true` 在告诉读者「整段朝一个方向走，可以外推」。
///
/// 同号是最弱的相似性：它连「这两个数差多少」都不问。要问的那个问题是
/// **差值相对于差值自身的抽样误差有多大** —— 也就是下面的 z 检验。
enum class HalvesRelation : std::int8_t {    /// 至少一侧的标准误估不出来（半程不足 3 点、时间不前进、输入非有限）。
    /// **不是**「一致」—— 把它和 `kConsistent` 印成同一个值，
    /// 就是让一次失败伪装成一次通过（P13-L 的 `drift_verdict` 为这个错付过代价）。
    kUnknown = 0,

    /// 两个斜率的差落在噪声之内：可以读成「整段在同一个方向上以同一速率走」。
    ///
    /// ⚠️ 它的**强度**取决于 `resolution_ppm`：SE 很大时任何差异都会被吞掉，
    /// 那时「一致」这个词没有信息量。所以每个 `kConsistent` 都必须
    /// 和 `resolution_sufficient` 一起读。
    kConsistent = 1,

    /// 两个斜率的差超出噪声：全序列那条斜率**描述错了对象**。
    ///
    /// 后果不是「斜率不准」，是那条 `slope × span` 的乘法没有定义域 ——
    /// 它可能严重低估（前半 0.6 ppm 在 65 s 里积了 38.8 µs，
    /// 而全序列 −0.0015 ppm 乘 130 s 给出 −0.2 µs）。
    /// 所以报告层在 `kInconsistent` 时把 `drift_verdict` 降为 `unknown`。
    kInconsistent = 2,
};

/// 线格式名字。三态在测试里逐字钉死 —— 改一个字节就是换了一份 schema。
[[nodiscard]] const char* halves_relation_name(HalvesRelation relation) noexcept;

/// 「差得够不够多」的绝对下限：累积差不到 1 µs 时，`kInconsistent` 不成立（P13-P 补）。
///
/// 为什么 z 检验一个人不够：z 是**相对**的。两侧都是完美直线时，残差只剩浮点噪声，
/// 标准误落到 1e-15 ppm 量级，于是 1e-14 ppm 的差也是 6 σ —— 判成「两条线」，
/// `drift_verdict` 随之降成 `unknown`。那件事在 `p13l` 的预算单测里被撞到两次：
/// 一份完美线性的 20 ppm 序列报不出 `within_budget`，而它恰恰是最干净的那种证据。
///
/// 1 µs 这个数的出处：它是额度的 1/45000（`kAudibleThresholdNs = 45 ms`），
/// 也是直方图桶宽（0.25 ms）的 1/250。积不到这个量，两条线还是同一条线，
/// 对「整场漂了多少」的判读没有后果 —— 反证不成立，就不该推翻结论。
///
/// `z` 字段照旧出版：它仍是那个统计量，只是 `relation` 多了一道**材料性**门槛。
/// 两个数并排摆着，读者能看到「6.5 σ 但只差 2.5e-14 ppm」这种情形，而不是被它骗。
inline constexpr std::int64_t kMinCumulativeGapNs = 1'000;

/// 一个半程的拟合结果。三个数就够，多一个都是能传错的地方。
struct HalvesFit {
    /// HAC 标准误估出来了没有。false 时 `std_error_ppm` 不可读。
    ///
    /// 与 `slope_ppm` 无关：斜率总是能拟合的（只要有 2 个点），
    /// 估不出来的是**误差**。把这两件事混成一个 flag，
    /// 会让"斜率算出来了但不知道准不准"这种情形无法表达。
    bool estimated = false;

    /// 半程斜率，ppm（正 = `lateness` 越跑越大）。
    double slope_ppm = 0.0;

    /// 半程斜率的 HAC 标准误，ppm。只在 `estimated` 为真时有效。
    double std_error_ppm = 0.0;

    /// 这个半程**实际**覆盖的媒体时间跨度。
    ///
    /// 不按「半程时长」算而是取真实首末差：两半程的长度由 `n/2` 的整数除法
    /// 决定，末尾那一个点的归属会让两者差一格。用真实跨度就不必假设它们等长。
    std::int64_t span_ns = 0;
};

/// 判定结果：**数值是一组，结论是一个词**，谁也不替谁说话。
struct HalvesAgreement {
    /// 两侧 SE 都估出来了，因此 `std_error` / CI / `z` / `resolution` / `relation`
    /// 这一组字段可读。
    ///
    /// ⚠️ `cumulative_gap_ns` 与 `gap_ppm` **不受它约束** ——
    /// 那两个只依赖斜率，而斜率总是拟合得出来的。把这两组绑在同一个 flag 上，
    /// 会让「知道差多少但不知道准不准」这种真实情形无法表达。
    bool estimated = false;

    HalvesRelation relation = HalvesRelation::kUnknown;

    double first_std_error_ppm = 0.0;
    double second_std_error_ppm = 0.0;
    double first_ci_low_ppm = 0.0;
    double first_ci_high_ppm = 0.0;
    double second_ci_low_ppm = 0.0;
    double second_ci_high_ppm = 0.0;

    /// `|first − second|`。**必须和 `pooled_error_ppm` 一起读** ——
    /// 单独一个 0.57 ppm 的差，读者会拿它去和一个 0.18 ppm 的尺子比，
    /// 而那个比较根本不成立。
    double gap_ppm = 0.0;

    /// 差值自身的抽样误差：`sqrt(se1² + se2²)`（两侧独立）。
    double pooled_error_ppm = 0.0;

    /// `gap / pooled`。t 统计量的双样本形式。`pooled == 0` 时为 0 或 +inf。
    double z = 0.0;

    /// 两个半程各自把斜率积在自己跨度上，两个累积量之差（ns，非负）。
    ///
    /// 这是「混合斜率可能错多少」的**直接读数**，不是外推：
    /// `p13n-fwd-r1` 的前半积出 38.8 µs、后半积出 1.8 µs，
    /// 而这个字段会说 37 µs —— 全序列那条线说 −0.2 µs。
    std::int64_t cumulative_gap_ns = 0;

    /// `1.96 × pooled`：这套证据能分辨的最小速率差（ppm）。
    double resolution_ppm = 0.0;

    /// 要分辨出「整段积满额度」所需的速率差：`额度 / 总跨度` 换算成 ppm。
    double budget_equivalent_ppm = 0.0;

    /// `resolution_ppm <= budget_equivalent_ppm`。
    ///
    /// 这个布尔不进三态：它不改变「差在不在噪声内」这个事实，只说明
    /// 那个事实够不够细。校验器判 `consistent ⇒ resolution_sufficient`，
    /// 让「空口说一致」在结构上不可能 —— 否则一份半程各 15 个点的证据
    /// 照样能报 `consistent`，而那时 `resolution` 可能到 40 ppm。
    bool resolution_sufficient = false;

    /// 两半程各自**真实**覆盖的媒体时间跨度（P13-O）。
    ///
    /// 出版它们不是为了好看，是为了让 `cumulative_gap_ns` 可复算：
    /// 没有这两个数，校验器只能验"它是个正整数"，而
    /// `|first×span1 − second×span2|` 这个式子里有两个未知量。
    std::int64_t first_span_ns = 0;
    std::int64_t second_span_ns = 0;
};

/// 比两个半程。
///
/// 纯函数：不读序列、不读时钟、不分配、不改入参。全部输入是七个标量级别的量，
/// 所以它能在桌面上把每种判据边界都钉死 —— 真机只能给出"这一轮是什么形状"。
///
/// `total_span_ns` 是整条序列的跨度，用来把额度换算成 ppm；
/// `budget_ns` 通常传 `kAudibleThresholdNs`，独立成参数是为了让单测能换尺子。
[[nodiscard]] HalvesAgreement compare_halves(const HalvesFit& first,
                                             const HalvesFit& second,
                                             std::int64_t total_span_ns,
                                             std::int64_t budget_ns) noexcept;

}  // namespace kivo::video::core::pace
