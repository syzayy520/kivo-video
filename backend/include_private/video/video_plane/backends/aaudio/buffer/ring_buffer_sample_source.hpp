#pragma once

#include <atomic>
#include <cstdint>

#include "video/video_plane/backends/aaudio/session/aaudio_output_session.hpp"
#include "video/video_plane/core/audio/spsc_audio_ring_buffer.hpp"

namespace kivo::video::aaudio {

/// 把可移植的 SPSC 环形缓冲接到 AAudio 会话的数据源接口上。
///
/// 这层是刻意的薄适配器：`core/audio` 不知道 AAudio 存在，
/// 会话也不知道缓冲存在。两边只通过本类连起来，因此：
///   - iOS 端 AudioUnit 回调可复用同一个 `SpscAudioRingBuffer`，只换一个新的适配器；
///   - 单测可以在没有 AAudio 的桌面环境里直接测核心缓冲，不受平台牵制。
///
/// render() 运行在实时音频线程上：本类不分配、不加锁、不打日志。
class RingBufferSampleSource final : public AaudioOutputSession::SampleSource {
public:
    explicit RingBufferSampleSource(core::audio::SpscAudioRingBuffer& ring) noexcept : ring_(ring) {}

    RingBufferSampleSource(const RingBufferSampleSource&) = delete;
    RingBufferSampleSource& operator=(const RingBufferSampleSource&) = delete;

    /// 返回实际取到的帧数；返回少于 frame_count 时由会话补静音并计入欠载。
    ///
    /// **栅栏（P12-F2A）**：`muted` 为真时直接返回 0，**一次都不碰环**。
    /// 这让"清空环"这件事不必再要求"读者线程停掉"—— 读者被挡住即可。
    /// 语义上等价于"这次没有数据"，由会话补静音并计入 `frames_silenced`。
    ///
    /// 实现在实时线程上跑：只多一次 acquire 读，不加锁、不分配。
    std::int32_t render(float* interleaved, std::int32_t frame_count) override {
        if (muted_.load(std::memory_order_acquire)) {
            return 0;
        }
        return ring_.read(interleaved, frame_count);
    }

    /// 拉下/解除静音栅栏。**调用方负责在拉下之后等一个 burst 再动环**
    /// （在途回调可能已经过了判空那行）。见 `47-p12f2a-design.md` §二。
    void set_muted(bool muted) noexcept { muted_.store(muted, std::memory_order_release); }

    [[nodiscard]] bool muted() const noexcept {
        return muted_.load(std::memory_order_acquire);
    }

private:
    core::audio::SpscAudioRingBuffer& ring_;
    std::atomic<bool> muted_{false};
};

}  // namespace kivo::video::aaudio
