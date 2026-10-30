#include "video/video_plane/core/pace/pace_hac.hpp"

#include <cmath>

namespace kivo::video::core::pace {
namespace {

/// 残差 `e_i`：用已拟合的 `(截距, 斜率)` 预测该点，再看差多少。
///
/// 单独抽成一个函数而不是在循环里内联：HAC 要按 lag 反复扫序列，
/// 若把残差公式抄在三处，改一处忘两处就会让 γ_l 与 ssr 用不同的残差 ——
/// 而那种错不会让任何一个数变成 NaN，只会让膨胀比悄悄偏离 1。
struct Residuals {
    double intercept_ns = 0.0;
    double slope_per_ns = 0.0;
    std::int64_t origin_ns = 0;

    [[nodiscard]] double at(const DriftPoint& point) const noexcept {
        const double t = static_cast<double>(point.media_ns - origin_ns);
        return static_cast<double>(point.lateness_ns) - (intercept_ns + slope_per_ns * t);
    }
};

}  // namespace

std::int32_t bartlett_lags(std::int32_t n) noexcept {
    if (n <= 0) {
        return 0;
    }
    const double scaled = std::pow(static_cast<double>(n) / 100.0, 2.0 / 9.0);
    const double raw = 4.0 * scaled;
    if (!(raw >= 0.0)) {
        return 0;  // NaN 防御：宁可给 0 阶（退化成 IID），也不给一个未定义的阶数
    }
    return static_cast<std::int32_t>(raw);
}

HacSlopeError estimate_hac_slope_error(const DriftSeries& series,
                                       double slope_per_ns) noexcept {
    // 转发的意义不是省一行，而是让「整条序列」与「半程」走**同一条**代码：
    // 两份实现里的 `L` 钳制、`n/(n−2)` 修正、核权重迟早会分家，
    // 而分家之后两份 SE 就不能相除（`pooled = sqrt(se1² + se2²)` 直接失效）。
    return estimate_hac_slope_error_range(series, 0, series.size(), slope_per_ns);
}

HacSlopeError estimate_hac_slope_error_range(const DriftSeries& series,
                                             std::int32_t begin,
                                             std::int32_t end,
                                             double slope_per_ns) noexcept {
    HacSlopeError out;
    // 钳进合法范围而不是直接拒绝：调用方从 `n/2` 切出来的下标本来就该
    // 落在里面，落在外面一定是算错了 —— 但那种错用一个能算的数报出来
    // 比返回 `estimated = false` 更容易被发现（`false` 会被读成"样本太少"）。
    const std::int32_t size = series.size();
    if (begin < 0) {
        begin = 0;
    }
    if (end > size) {
        end = size;
    }
    const std::int32_t n = end - begin;
    if (n < 3) {
        return out;  // 自由度 ≤ 0，残差方差无从估计
    }

    // 原点取**这一段**的首点，不是整条序列的首点。
    // 取整条序列的首点也不会错（中心化矩不变），但 `t` 会多带一个常数，
    // 而在 `t ≤ 1e11` 的量级上那个常数的平方会吃掉 double 的低位。
    const std::int64_t origin = series.at(begin).media_ns;
    double sum_t = 0.0;
    double sum_y = 0.0;
    double sum_tt = 0.0;
    for (std::int32_t i = begin; i < end; ++i) {
        const DriftPoint point = series.at(i);
        const double t = static_cast<double>(point.media_ns - origin);
        sum_t += t;
        sum_y += static_cast<double>(point.lateness_ns);
        sum_tt += t * t;
    }
    const double dn = static_cast<double>(n);
    const double mean_t = sum_t / dn;
    const double mean_y = sum_y / dn;
    const double sxx = sum_tt - sum_t * sum_t / dn;
    if (!(sxx > 0.0)) {
        return out;  // 时间不前进
    }
    // 截距由序列自己算，不从调用方要：多一个可推导的参数就多一处能传错。
    const Residuals residuals{mean_y - slope_per_ns * mean_t, slope_per_ns, origin};

    // 一趟同时攒 `ssr`（IID 版要用）与 `γ0`（HAC 的核心）与 `ρ1` 的分子。
    double ssr = 0.0;
    double gamma0 = 0.0;
    double rho1_numerator = 0.0;
    double previous_e = 0.0;
    for (std::int32_t i = begin; i < end; ++i) {
        const DriftPoint point = series.at(i);
        const double t = static_cast<double>(point.media_ns - origin);
        const double x = t - mean_t;
        const double e = residuals.at(point);
        ssr += e * e;
        const double u = x * e;
        gamma0 += u * u;
        if (i > begin) {
            rho1_numerator += previous_e * e;
        }
        previous_e = e;
    }

    const double dof = dn / (dn - 2.0);
    const double var_iid = (ssr / (dn - 2.0)) / sxx;

    std::int32_t lags = bartlett_lags(n);
    // 自由度保护：`L ≥ n − 2` 时 γ_L 只剩一两个乘积项，纯噪声。
    if (lags > n - 3) {
        lags = n - 3;
    }
    if (lags < 0) {
        lags = 0;
    }

    // Bartlett 核：`w_l = 1 − l/(L+1)`，权重线性衰减到 0，保证核半正定。
    double kernel = gamma0;
    const double denom = static_cast<double>(lags) + 1.0;
    for (std::int32_t lag = 1; lag <= lags; ++lag) {
        double acc = 0.0;
        for (std::int32_t i = begin; i + lag < end; ++i) {
            const DriftPoint left = series.at(i);
            const DriftPoint right = series.at(i + lag);
            const double xl = static_cast<double>(left.media_ns - origin) - mean_t;
            const double xr = static_cast<double>(right.media_ns - origin) - mean_t;
            acc += (xl * residuals.at(left)) * (xr * residuals.at(right));
        }
        kernel += 2.0 * (1.0 - static_cast<double>(lag) / denom) * acc;
    }

    // `n/(n−2)`：让 L=0 时 HAC 与 IID 落在同一把秤上（见头文件）。
    double var_hac = dof * kernel / (sxx * sxx);
    if (!(var_hac >= 0.0)) {
        // 有限样本下核估计能算出负方差（已知的退化情形，真机上未出现）。
        // 钳到 0 而不是取绝对值：绝对值会造出一个"看起来正常"的 SE，
        // 而这里唯一诚实的说法是"这次没量出正的方差"。
        var_hac = 0.0;
    }

    out.estimated = true;
    out.lags = lags;
    out.std_error_per_ns = std::sqrt(var_hac);
    out.lag1_autocorrelation = (ssr > 0.0) ? (rho1_numerator / ssr) : 0.0;
    out.variance_inflation = (var_iid > 0.0) ? (var_hac / var_iid) : 1.0;
    return out;
}

}  // namespace kivo::video::core::pace
