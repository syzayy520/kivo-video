#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "video/video_plane/backends/aaudio/buffer/ring_buffer_sample_source.hpp"
#include "video/video_plane/backends/aaudio/session/aaudio_output_session.hpp"
#include "video/video_plane/backends/avsync/session/av_sync_stats.hpp"
#include "video/video_plane/backends/mediacodec/decode/mediacodec_audio_decoder.hpp"
#include "video/video_plane/backends/mediacodec/session/audio_decode_feeder.hpp"
#include "video/video_plane/backends/mediacodec/source/mediaextractor_source.hpp"
#include "video/video_plane/core/audio/spsc_audio_ring_buffer.hpp"

namespace kivo::video::avsync {

/// 音频播放侧：解封装 → 音频解码 → 环形缓冲 → AAudio → 音频主时钟。
///
/// 为什么需要它：C5 的节拍器早就可以接 `AudioClockSource`，但到今天为止
/// 从来没人往里塞过**真的**音频时钟 —— 纯视频轮一律用 `SystemClockSource`。
/// 于是 `av_drift` 这个字段一直没有数据源（见 `24-p12c8-design.md` §一）。
///
/// 与 `probe::AudioFileProbeReport` 的区别：探针是"一次性取证的入口"，
/// 这里是"可以嵌进播放会话的组件"。探针带 stdout 输出与退出码语义，这里没有。
///
/// 线程模型：`open()`/`start()`/`stop()` 在调用线程；解码在内部线程（`AudioDecodeFeeder`）；
/// 出声在 AAudio 的实时线程。三者之间只有环形缓冲与原子计数，不加锁。
///
/// 失败一律拒绝并写入 `stats().error`，**绝不静默降级**：
/// 音频开不起来却悄悄退回 `system` 时钟继续播，会产出一份"看起来有 av_drift、
/// 其实是节拍偏移"的假证据 —— 那比没有证据更坏。
class AudioPlayback {
public:
    AudioPlayback() = default;
    ~AudioPlayback();

    AudioPlayback(const AudioPlayback&) = delete;
    AudioPlayback& operator=(const AudioPlayback&) = delete;
    AudioPlayback(AudioPlayback&&) = delete;
    AudioPlayback& operator=(AudioPlayback&&) = delete;

    /// 打开音轨、配置解码器、开 AAudio 流、灌满预灌水位。
    ///
    /// @param fd 只读描述符，由调用方持有（与视频侧同一个文件的两个 fd 之一）
    /// @param length 文件字节数
    /// 成功返回 true；失败返回 false 并把原因写进 `stats().error`。
    [[nodiscard]] bool open(int fd, std::int64_t length, const std::string& label);

    /// 起流，并等到音频时钟拿到首个有效时间戳。
    ///
    /// **必须在视频起播之前调用。** NDK 明说 `requestStart` 是异步的，
    /// 起流瞬间读时间戳返回 `INVALID_STATE`。不等就开视频，前几十帧会读到
    /// 不推进的时钟 → `ClockHealth::kFreeRun` → 节拍器改用自生成时间轴，
    /// 整轮的 `av_drift` 都会变成噪声。
    [[nodiscard]] bool start();

    /// 一次音频 seek 的结果（P12-F2）。`landing_us` 是**实际落点**，不是请求值。
    struct SeekResult {
        bool ok = false;
        std::int64_t landing_us = -1;
        std::string reason;
    };

    /// 定位到指定时刻（P12-F2）。
    ///
    /// 顺序固定，改之前先想清楚每一步为什么在它前面：
    /// 1. `feeder.stop()`：先让环的写者停下（`ring.reset()` 要求两个线程都停）；
    /// 2. `output.stop()`：再停读者。**不 `close`** —— 重开一次流的启动方差是 24–146 ms（C1），
    ///    为一次 seek 白花一百毫秒不划算；
    /// 3. 定位 → `decoder.flush()` → `ring.reset()`；
    /// 4. 重新供给 + 预灌（与起播同一水位）+ 起流；
    /// 5. **`clock.rebase(landing)`**：停/启流之后帧位置归零，不重锚时钟会退回 0，
    ///    视频被节拍器拖回片头。这一步漏了，所有计数都正常但画面是错的。
    ///
    /// 返回 `landing_us < 0` 表示定位失败；此时**仍然会恢复播放**（从当前残留位置继续），
    /// 并把原因写进 `reason` —— 让调用方决定是中止还是继续，而不是让音频静默消失。
    [[nodiscard]] SeekResult seek_to(std::int64_t target_us);

    /// 停止。顺序固定：停供给 → 排空 → 取时钟 → 停流。
    /// 反过来（先停流）会让环里剩的数据永远没人取，被如实记成大量欠载。
    void stop();

    [[nodiscard]] bool is_open() const noexcept { return opened_; }
    [[nodiscard]] bool is_started() const noexcept { return started_; }

    /// 音频主时钟，交给 `PacingSetup::clock`。`open()` 成功前返回 nullptr。
    [[nodiscard]] core::audio::AudioClockSource* clock() noexcept;

    /// 事实快照。`feed` / `output` / `clock` 三组字段只有 `stop()` 之后才是终值。
    [[nodiscard]] const AudioPlaybackStats& stats() const noexcept { return stats_; }

private:
    mediacodec::MediaExtractorSource source_;
    mediacodec::MediaCodecAudioDecoder decoder_;

    /// 声明顺序即销毁顺序（逆序）：`feeder_` 引用前四个，
    /// `output_` 引用 `ring_source_`，`ring_source_` 引用 `ring_`。
    /// 因此 feeder → output → ring_source → ring 的销毁次序必须由声明顺序保证，
    /// 调换任意两行都会得到悬空引用（而且只在析构时才炸）。
    std::unique_ptr<core::audio::SpscAudioRingBuffer> ring_;
    std::unique_ptr<aaudio::RingBufferSampleSource> ring_source_;
    std::unique_ptr<aaudio::AaudioOutputSession> output_;
    std::unique_ptr<mediacodec::AudioDecodeFeeder> feeder_;

    AudioPlaybackStats stats_;
    bool opened_ = false;
    bool started_ = false;

    /// `stop()` 只做一次。第二次调用必须原样返回：
    /// 停流之后 `getTimestamp` 立刻退回 `INVALID_STATE`，再取一次时钟统计
    /// 会把 `clock_end_ns` 与 `clock` 覆盖成停流后的回退档数字 —— 好数据被坏数据覆盖，
    /// 而证据看上去依然完整。
    bool stopped_ = false;
};

}  // namespace kivo::video::avsync
