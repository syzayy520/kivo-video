#pragma once

#include <cstdint>

namespace kivo::video::core::audio {

/// 从 `(墙钟, 媒体位置)` 样本对估计"媒体钟相对墙钟的速率偏差"。
///
/// 为什么单独一个类：`ClockPositionTracker` 现在只有**相位**控制
/// （越过 45 ms 就一次复位到 40 ms），没有**速率**估计。
/// 而"钟持续快 1.5%"这种误差，靠相位门只能周期性砍一刀（`184` 的 6/23 轮门沉默、
/// P13-AB 的 2/5/6/10 次复位都是这个形状）。要判断该不该换成速率伺服，
/// 先得有一个逐帧可用的 `r = Δposition / Δwall`。
///
/// **纯数学**：不读时钟、不分配、不加锁、不含任何判定。
/// 放在 `core/audio/clock/` 而不是 `pace/`，因为它不知道帧、不知道门、不知道会话。
///
/// 为什么 header-only：它只有一个类、每帧调用一次、实现会被内联；
/// 而新增一个 `.cxx` 就要同时改两份 CMake 清单（`backend/CMakeLists.txt` 正被他人主线占用），
/// 否则会留下"未注册源文件"的技术债。纯数学不值得为此欠账。
///
/// 与 `ClockPositionTracker` 的关系（刻意的）：
///   `phase_error_ns = position − 首样本位置 − 墙钟增量`
/// 与那边的派生 lead（`clock_position_tracker.cxx:165-168`）同一个式子，
/// 这样将来接线时两边的"领先量"不会各算一套。
class ClockRateEstimator {
public:
    struct Sample {
        std::int64_t mono_ns = 0;      ///< 墙钟（与时间戳同源）
        std::int64_t position_ns = 0;  ///< 媒体位置
    };

    struct Estimate {
        /// 一阶低通后的速率偏差，单位 ppm。正数 = 媒体钟比墙钟快。
        double rate_ppm = 0.0;
        /// 本次样本的瞬时值（未滤波），便于看抖动。
        double raw_ppm = 0.0;
        /// 累计相位误差：位置 − 首样本位置 − 墙钟增量。正数 = 位置跑在前面。
        std::int64_t phase_error_ns = 0;
        /// 是否够两个样本、且墙钟增量 > 0。`false` 时 `reason` 必非空。
        bool valid = false;
        const char* reason = "";
    };

    /// @param smoothing 低通系数，取值 (0, 1]；越小越平滑、收敛越慢。
    ///                  默认 0.05：约 20 个样本达到 63% 阶跃响应。
    explicit ClockRateEstimator(double smoothing = 0.05) noexcept
        : smoothing_(smoothing > 0.0 && smoothing <= 1.0 ? smoothing : 0.05) {}

    /// 喂一个样本。调用方保证 `mono_ns` 单调不减。
    [[nodiscard]] Estimate note(const Sample& sample) noexcept {
        Estimate out;

        if (!has_origin_) {
            has_origin_ = true;
            origin_mono_ns_ = sample.mono_ns;
            origin_position_ns_ = sample.position_ns;
        }

        if (has_prev_) {
            const std::int64_t wall_delta_ns = sample.mono_ns - prev_mono_ns_;
            if (wall_delta_ns <= 0) {
                // 墙钟没前进：算不出速率。不是 0 ppm，是"不可判"（G9）。
                out.reason = "wall clock did not advance; rate cannot be estimated";
                out.phase_error_ns = sample.position_ns -
                                     (origin_position_ns_ + (sample.mono_ns - origin_mono_ns_));
                return out;
            }

            const std::int64_t position_delta_ns = sample.position_ns - prev_position_ns_;
            out.raw_ppm = (static_cast<double>(position_delta_ns) /
                               static_cast<double>(wall_delta_ns) -
                           1.0) *
                          1'000'000.0;

            // 第一个有效样本直接作为初值：否则从 0 起步，前几十个样本都在爬坡，
            // 会让人误以为"钟在加速"。
            if (samples_ == 0) {
                filtered_ppm_ = out.raw_ppm;
            } else {
                filtered_ppm_ += smoothing_ * (out.raw_ppm - filtered_ppm_);
            }
            ++samples_;
            out.valid = true;
        } else {
            out.reason = "first sample: no delta yet";
        }

        out.rate_ppm = filtered_ppm_;
        out.phase_error_ns =
            sample.position_ns - (origin_position_ns_ + (sample.mono_ns - origin_mono_ns_));

        prev_mono_ns_ = sample.mono_ns;
        prev_position_ns_ = sample.position_ns;
        has_prev_ = true;
        return out;
    }

    /// 清空全部历史。seek / 换片源 / 设备重建之后必须调用 ——
    /// 否则首样本基准还停在上一段，相位误差会凭空多出"seek 距离"。
    void reset() noexcept {
        has_origin_ = false;
        has_prev_ = false;
        origin_mono_ns_ = 0;
        origin_position_ns_ = 0;
        prev_mono_ns_ = 0;
        prev_position_ns_ = 0;
        filtered_ppm_ = 0.0;
        samples_ = 0;
    }

    /// 已接受的有效样本数（不含首个无增量的样本）。
    [[nodiscard]] std::int32_t samples() const noexcept { return samples_; }

private:
    double smoothing_;
    bool has_origin_ = false;
    bool has_prev_ = false;
    std::int64_t origin_mono_ns_ = 0;
    std::int64_t origin_position_ns_ = 0;
    std::int64_t prev_mono_ns_ = 0;
    std::int64_t prev_position_ns_ = 0;
    double filtered_ppm_ = 0.0;
    std::int32_t samples_ = 0;
};

}  // namespace kivo::video::core::audio
