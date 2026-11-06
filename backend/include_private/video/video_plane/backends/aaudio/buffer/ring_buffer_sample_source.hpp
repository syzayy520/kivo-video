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
    /// 实现在实时线程上跑：不加锁、不分配、不打日志。
    ///
    /// P16-P1B6：计数**必须早于判空**。若放在 `muted_.load()` 之后，
    /// 那些"已经过了判空那行、还没读环"的回调不计入 —— 而 `set_muted()` 的注释
    /// 点名的正是这个窗口。两条 relaxed/release atomic 与 SPSC 环的索引同类，
    /// 不改变判空/读环逻辑，只让"在途"变得可观测。
    std::int32_t render(float* interleaved, std::int32_t frame_count) override {
        in_flight_.fetch_add(1, std::memory_order_relaxed);
        if (muted_.load(std::memory_order_acquire)) {
            in_flight_.fetch_sub(1, std::memory_order_release);
            return 0;
        }
        const std::int32_t read = ring_.read(interleaved, frame_count);
        in_flight_.fetch_sub(1, std::memory_order_release);
        return read;
    }

    /// 只读观测：当前有多少回调处在 `render()` 内（含"已过判空、还没读环"的窗口）。
    /// 本卡只用它测量，不拿它做同步 —— 若将来用做同步，需按 SPSC 环的惯例
    /// 给 `in_flight_` 加 `alignas(64)` 防止与 `muted_` 伪共享。
    [[nodiscard]] std::int32_t in_flight() const noexcept {
        return in_flight_.load(std::memory_order_acquire);
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
    std::atomic<std::int32_t> in_flight_{0};
};

}  // namespace kivo::video::aaudio
