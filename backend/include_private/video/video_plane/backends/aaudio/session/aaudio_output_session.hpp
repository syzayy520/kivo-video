#pragma once

#include <aaudio/AAudio.h>

#include <atomic>
#include <cstdint>
#include <memory>

#include "video/video_plane/backends/aaudio/clock/aaudio_clock_source.hpp"
#include "video/video_plane/backends/aaudio/clock/aaudio_timestamp_reader.hpp"
#include "video/video_plane/backends/aaudio/device/aaudio_open_outcome.hpp"
#include "video/video_plane/backends/aaudio/device/aaudio_stream_config.hpp"
#include "video/video_plane/backends/aaudio/session/audio_output_stats.hpp"

namespace kivo::video::aaudio {

/// 音频输出会话：开流、启停、统计。
///
/// 数据来源通过 SampleSource 注入 —— 会话不知道声音是什么，
/// 只知道"向 source 要 frames，要不到的补静音并计数"。
/// P12-C2 接入环形缓冲时，只需让缓冲实现这个接口，会话一行都不用改。
///
/// 回调线程约定（硬性）：
///   render() 在 AAudio 的实时音频线程上被调用，
///   其中**不许加锁、不许 new、不许打日志、不许构造字符串**。
///   违反会直接表现为爆音与 xrun，且极难归因。
class AaudioOutputSession {
public:
    class SampleSource {
    public:
        virtual ~SampleSource() = default;

        /// 填充交错样本。返回实际写入的帧数；
        /// 少于 frame_count 的部分由会话补静音并计入 frames_silenced。
        /// 返回值会被夹到 [0, frame_count]。
        virtual std::int32_t render(float* interleaved, std::int32_t frame_count) = 0;
    };

    AaudioOutputSession() = default;
    ~AaudioOutputSession();

    AaudioOutputSession(const AaudioOutputSession&) = delete;
    AaudioOutputSession& operator=(const AaudioOutputSession&) = delete;
    AaudioOutputSession(AaudioOutputSession&&) = delete;
    AaudioOutputSession& operator=(AaudioOutputSession&&) = delete;

    /// 开流并绑定数据源。成功后会话处于 open 状态（尚未出音）。
    [[nodiscard]] AaudioOpenOutcome open(const AaudioStreamConfig& config, SampleSource& source);

    /// 请求启动，并确认真的进入 STARTED（AAudio 的 start 是异步的）。
    [[nodiscard]] AaudioOpenOutcome start();

    /// 请求停止并等待状态落定。可重复调用。
    void stop();

    /// 停止并释放流。析构会自动调用。
    void close();

    [[nodiscard]] bool is_open() const { return stream_ != nullptr; }
    [[nodiscard]] const AaudioOpenOutcome& outcome() const { return outcome_; }
    [[nodiscard]] AudioOutputStats stats() const;

    /// 音频主时钟。`open()` 成功后可用，`close()` 之后为空。
    ///
    /// 时钟的生命周期刻意与 stream 绑定，而不是让调用方自己去造：
    /// 时间戳读的是一个具体的流，流一关，那个时钟就没有任何意义。
    /// 谁想拿它，就必须先拿到一个成功的 open。
    [[nodiscard]] AaudioClockSource* clock() { return clock_.get(); }

    /// `requestStart` 被调用的时刻（单调纳秒）。
    /// C4 用它和"首次拿到有效时间戳的时刻"相减，得到启动开销 ——
    /// 这个值必须由**设备侧读数**印证，不能靠墙钟猜（见 15-p12c4-design.md §三）。
    [[nodiscard]] std::int64_t start_mono_ns() const { return start_mono_ns_; }

private:
    static aaudio_data_callback_result_t on_data(AAudioStream* stream,
                                                 void* user_data,
                                                 void* audio_data,
                                                 std::int32_t num_frames);

    void render_into(float* target, std::int32_t frame_count);

    AAudioStream* stream_ = nullptr;
    SampleSource* source_ = nullptr;
    AaudioOpenOutcome outcome_;

    // 声明顺序即销毁顺序（逆序）：clock_ 引用 clock_reader_，
    // 所以 clock_reader_ 必须声明在前、销毁在后。
    std::unique_ptr<AaudioTimestampReader> clock_reader_;
    std::unique_ptr<AaudioClockSource> clock_;

    // 回调线程写、调用方读，因此全部 relaxed 原子。
    // 这些计数只用于事后统计，不参与任何"根据计数决定行为"的判断，
    // 不需要 acquire/release 语义。
    std::atomic<std::int64_t> callbacks_{0};
    std::atomic<std::int64_t> frames_rendered_{0};
    std::atomic<std::int64_t> frames_silenced_{0};
    std::atomic<std::int64_t> first_audible_frame_{-1};

    std::int32_t xrun_baseline_ = 0;
    std::int64_t started_at_millis_ = 0;
    std::int64_t start_mono_ns_ = 0;
    bool started_ = false;
};

}  // namespace kivo::video::aaudio
