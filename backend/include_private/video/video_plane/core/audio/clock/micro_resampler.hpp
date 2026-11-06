#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace kivo::video::core::audio {

/// 极小比率的采样率微调器（零阶保持）。
///
/// 用途：`199` 的 B 方案 —— 把"音频钟比墙钟快 0.2%"这件事在**输出侧**吃掉，
/// 而不是周期性复位位置。比率只在 ±几千 ppm 量级，不需要高质量重采样：
/// 每 ~500 帧重复或丢 1 帧，0.2% 的失真在听感上可忽略（但仍需真机听测，本卡不宣称）。
///
/// **纯逻辑、零分配、零平台依赖**。header-only 的理由与 `ClockRateEstimator` 相同：
/// 新增 `.cxx` 要同时改两份 CMake 清单，而 `backend/CMakeLists.txt` 被他人主线占用。
///
/// 本卡**不接线**：`RingBufferSampleSource::render()` 一行未改。
/// 接线需要跨线程把 `rate_ppm` 从视频线程传到音频回调线程（atomic），
/// 并在实时路径上保证 `ratio == 1.0` 时逐位透传 —— 那是独立一张卡。
class MicroResampler {
public:
    /// @param ratio 输出帧数 / 输入帧数。`1.0` = 透传。
    ///              钟快 2000 ppm 时应设 `1.0 - 2000e-6 = 0.998`（消费变慢）。
    void set_ratio(double ratio) noexcept {
        // 只接受"接近 1"的比率：这不是通用重采样器，超过 ±5% 就不是微调了。
        if (ratio > 0.95 && ratio < 1.05) {
            ratio_ = ratio;
        }
    }

    [[nodiscard]] double ratio() const noexcept { return ratio_; }

    /// 喂 `in_frames` 帧，写出最多 `max_out` 帧，返回实际输出帧数。
    ///
    /// 相位跨调用保留 —— 每次 AAudio 回调只给 192 帧，比率的小数部分必须在
    /// 调用之间累积，否则 0.2% 会被每帧四舍五入掉。
    ///
    /// `ratio == 1.0` 时走快路径：逐帧拷贝，输出帧数恒等于输入帧数。
    std::int32_t process(const float* in, std::int32_t in_frames, float* out,
                         std::int32_t max_out, std::int32_t channels) noexcept {
        if (in == nullptr || out == nullptr || channels <= 0 || in_frames <= 0) {
            return 0;
        }
        if (ratio_ == 1.0) {
            const std::int32_t frames = in_frames < max_out ? in_frames : max_out;
            const std::int32_t samples = frames * channels;
            for (std::int32_t i = 0; i < samples; ++i) {
                out[i] = in[i];
            }
            return frames;
        }

        std::int32_t written = 0;
        for (std::int32_t frame = 0; frame < in_frames && written < max_out; ++frame) {
            accumulator_ += ratio_;
            // ratio < 1：偶尔一次都不输出（丢帧）；ratio > 1：偶尔输出两次（重复帧）。
            while (accumulator_ >= 1.0 && written < max_out) {
                const float* src = in + static_cast<std::ptrdiff_t>(frame) * channels;
                float* dst = out + static_cast<std::ptrdiff_t>(written) * channels;
                for (std::int32_t c = 0; c < channels; ++c) {
                    dst[c] = src[c];
                }
                ++written;
                accumulator_ -= 1.0;
            }
        }
        return written;
    }

    /// **实时回调要用的接口**：输出**恰好** `out_frames` 帧。
    ///
    /// 为什么不能直接用 `process()`：AAudio 回调必须填满 `num_frames`，
    /// 而 `process()` 输出的是 `输入 × ratio`。`ratio < 1` 时会少几帧，
    /// 会话只能补静音 —— 那不是"音频变慢"，是"每 500 帧断一下"。
    ///
    /// 语义：`ratio` 仍是"输出/输入"，但调用方先按 `ceil(out_frames / ratio) + 1`
    /// 读够输入，这里再按 `step = 1/ratio` 在输入流上走，输出固定帧数。
    /// 钟快（`ratio > 1`）时表现为**偶尔重复一帧**，媒体时间因此变慢 —— 这正是要的。
    ///
    /// @return 实际写出帧数；正常应等于 `out_frames`。小于它说明调用方输入读少了。
    std::int32_t fill(const float* in, std::int32_t in_frames, float* out,
                      std::int32_t out_frames, std::int32_t channels) noexcept {
        if (in == nullptr || out == nullptr || channels <= 0 || out_frames <= 0 ||
            in_frames <= 0) {
            return 0;
        }
        const double step = 1.0 / ratio_;
        std::int32_t written = 0;
        double pos = phase_;
        while (written < out_frames) {
            const auto index = static_cast<std::int32_t>(pos);
            if (index >= in_frames) {
                break;  // 输入不够：调用方读少了，别在这里造静音
            }
            const float* src = in + static_cast<std::ptrdiff_t>(index) * channels;
            float* dst = out + static_cast<std::ptrdiff_t>(written) * channels;
            for (std::int32_t c = 0; c < channels; ++c) {
                dst[c] = src[c];
            }
            ++written;
            pos += step;
        }
        // 只保留**小数**相位：输入缓冲每次都是新的，整数偏移没有意义；
        // 若写成 `pos - in_frames`，读多输入时会变负、被 clamp 成 0，相位就丢了。
        phase_ = pos - std::floor(pos);
        return written;
    }

    /// 清空相位。seek / 换片源后调用，否则上一段的余量会带进下一段。
    void reset() noexcept {
        accumulator_ = 0.0;
        phase_ = 0.0;
    }

    [[nodiscard]] double pending_phase() const noexcept { return accumulator_; }

private:
    double ratio_ = 1.0;
    double accumulator_ = 0.0;
    /// `fill()` 的相位：当前在输入流中的位置（跨调用保留）。
    double phase_ = 0.0;
};

}  // namespace kivo::video::core::audio
