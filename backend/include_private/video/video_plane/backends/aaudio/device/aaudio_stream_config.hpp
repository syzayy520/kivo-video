#pragma once

#include <cstdint>

namespace kivo::video::aaudio {

/// 我们希望拿到的音频输出参数。
///
/// 刻意不含任何 `<aaudio/AAudio.h>` 类型：配置是纯数据，单测可以直接构造。
/// 平台枚举的翻译集中在 `aaudio_stream_opener` 一处完成，别的地方不许再碰。
struct AaudioStreamConfig {
    std::int32_t sample_rate = 48000;
    std::int32_t channel_count = 2;

    /// 只保留两种。float 与音频平面的内部表示一致；
    /// int16 留给「独占模式被拒、设备只接受定点」的极少数情况。
    enum class SampleFormat : std::int32_t {
        kFloat = 0,
        kInt16 = 1,
    };
    SampleFormat format = SampleFormat::kFloat;

    /// 是否优先申请独占模式。被拒会回退共享，并在 outcome 里留下 downgrade_reason。
    bool prefer_exclusive = true;

    /// 是否申请低延迟性能模式。G7 要求的同步精度依赖它。
    bool prefer_low_latency = true;

    /// 0 表示交给 AAudio 决定；非 0 时作为 buffer 容量下限。
    std::int32_t buffer_capacity_frames = 0;

    /// 一帧（所有声道）占多少字节。
    [[nodiscard]] std::int32_t bytes_per_frame() const {
        const std::int32_t sample_bytes = (format == SampleFormat::kFloat) ? 4 : 2;
        return sample_bytes * channel_count;
    }

    [[nodiscard]] bool plausible() const {
        return sample_rate > 0 && channel_count > 0 && buffer_capacity_frames >= 0;
    }
};

}  // namespace kivo::video::aaudio
