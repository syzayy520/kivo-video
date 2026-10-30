#pragma once

#include <cstdint>

#include "video/video_plane/core/pace/pace_drift_series.hpp"

namespace kivo::video::core::pace {

/// 斜率的 HAC（Newey–West / Bartlett 核）标准误，外加"为什么要修正"的三个读数。
///
/// 存在的理由是一条被真机推翻的假设，不是理论洁癖：
/// P13-J 的 SE 用的是 `sqrt(σ̂² / Sxx)`，它假定残差**独立同分布**。
/// 而 `lateness` 不是：同一段播放里相邻两帧的 lateness 会连着好几帧偏同一个方向
/// （vsync 相位、CPU 调频、抽稀序列本身把慢变信号抹平）。正自相关会让
/// 有效样本数远小于 `n`，于是 SE 被系统性低估，置信带过窄 ——
/// 直接后果就是 P13-J 三轮里 r3（35 s、700 点）报出 `detectable = true`
/// 而 `r2 = 0.007`：一条只解释 0.7% 方差的线被判成"看得见"。
///
/// 所以这个单元只做一件事：**在同一个斜率上，换一把承认自相关的尺子**。
/// 它不改斜率、不改 r2、不改分窗 —— 那些是 `pace_drift_trend` 的事。
///
/// 公式（带截距 OLS 的斜率，Bartlett 核）：
///
///   u_i = x_i · e_i            （x 为去均值的时刻，e 为残差）
///   γ_l = Σ_{i=0}^{n-1-l} u_i · u_{i+l}
///   Var_HAC(b) = [γ_0 + 2 Σ_{l=1}^{L} (1 − l/(L+1)) γ_l] / Sxx² · n/(n−2)
///
/// `n/(n−2)` 是自由度修正，**必须带**：不带的话 L=0 时 HAC 与 IID 两个数
/// 差着一个 `n/(n−2)` 的常数因子，`variance_inflation` 就永远不等于 1，
/// 读者会以为是自相关而不是自由度。带上之后两条路在同一把秤上。
struct HacSlopeError {
    /// false ⇒ 样本太少（`n < 3`，或带宽吃掉全部自由度），其余字段不可读。
    /// **不是**"修正量为零"—— 那个读法会让一条算不出来的证据看起来像"没问题"。
    bool estimated = false;

    /// HAC 修正后的斜率标准误（ns/ns，量纲与斜率相同）。
    /// 只有在 `estimated` 为真时才有意义。
    double std_error_per_ns = 0.0;

    /// 实际使用的 Bartlett 截断阶数 `L`。写进证据是为了**可复算** ——
    /// 读者拿着原始序列和三行公式就能自己验一遍，不必相信这里的实现。
    std::int32_t lags = 0;

    /// 残差的 1 阶自相关 `ρ1 = Σ e_i e_{i+1} / Σ e_i²`，取值 [−1, 1]。
    ///
    /// 这是"为什么要修正"的一句话解释：ρ1 接近 0 时 HAC ≈ IID，
    /// ρ1 明显为正时 IID 的 SE 一定偏小。把它摆在 inflation 旁边，
    /// 读者不必去猜那个膨胀比是怎么来的。
    double lag1_autocorrelation = 0.0;

    /// `Var_HAC / Var_IID`。1.0 表示自相关没有影响；> 1 表示 IID 低估了不确定度。
    ///
    /// ⚠️ 它不是纯自相关因子 —— L=0 时它仍然会因为异方差偏离 1
    /// （IID 版把 `Σe²/(n−2)` 当常数，HAC 版是 `Σ x²e²` 的加权和）。
    /// 所以读法是"两把尺子差几倍"，不是"自相关系数是几"。
    double variance_inflation = 1.0;
};

/// Newey–West 的经验带宽：`L = floor(4 · (n/100)^(2/9))`。
///
/// 为什么用经验式而不是拍一个常数：这个式子在 n=100 时给 4、n=2048 时给 7，
/// 增长极慢（2/9 次幂），正好对应"窗口越长，能容纳的相关结构越多，但收益递减"。
/// 拍常数的话 700 点和 2048 点会用同一个 L，而这两者的相关尺度并不一样。
///
/// 单独暴露出来是为了让单测能钉住规则本身，也让证据里的 `hac_lags`
/// 有一个不依赖实现的复算路径（校验器 K1 就是拿它复算的）。
[[nodiscard]] std::int32_t bartlett_lags(std::int32_t n) noexcept;

/// 给一条**已经拟合好**的直线算 HAC 斜率标准误。
///
/// 只收 `series` 与 `slope_per_ns`：截距由序列内部重算（`ȳ − b·t̄`），
/// 因为多传一个可以由前两者推出的参数，就多一个能传错的地方。
///
/// 时间复杂度 `O((L+1)·n)`，`n ≤ 2048`、`L ≤ 7` ⇒ 约 1.6 万次乘加，
/// 只在停机出报告时调用一次（与 `summarize_drift` 同一时机），不进实时路径。
/// **不占栈上的大数组**：每个 lag 现算残差，用时间换 16 KB 栈。
///
/// `series.size() < 3` 或 `n ≤ L + 2` 时返回 `estimated = false`。
[[nodiscard]] HacSlopeError estimate_hac_slope_error(const DriftSeries& series,
                                                     double slope_per_ns) noexcept;

/// 同一把尺子，作用在 `[begin, end)` 这一段上（P13-O）。
///
/// 存在的理由是本项目的一个盲区：P13-K 给**全序列**斜率配了 HAC 标准误，
/// 却把两个**半程**斜率留在六位小数的裸数状态 —— 而「两半程是不是同一条线」
/// 这个判断，恰恰只能靠差值相对于各自误差的大小来回答。
///
/// 为什么不另立一个单元：职责仍是「一条直线的 HAC 斜率标准误」，
/// 只是把定义域从整条序列收窄到一段。另立单元会让同一套 Bartlett 核出现
/// 两份实现，而两份核迟早会在 `L` 的钳制上分家（那里的边界条件
/// `lags > n − 3` 依赖的正是这一段自己的 `n`）。
///
/// `slope_per_ns` 必须是**这一段自己拟合出来的**斜率，不是全序列的：
/// 残差 `e_i = y_i − (a + b·t_i)` 用的是它，传错斜率会让 `γ0` 与真实的
/// 残差方差无关，`std_error` 变成另一个量而不报错。
///
/// 越界的 `begin` / `end` 会被钳进 `[0, size()]`；钳完不足 3 点时
/// 返回 `estimated = false` —— 与整条序列那条路同一判据。
[[nodiscard]] HacSlopeError estimate_hac_slope_error_range(const DriftSeries& series,
                                                           std::int32_t begin,
                                                           std::int32_t end,
                                                           double slope_per_ns) noexcept;

}  // namespace kivo::video::core::pace
