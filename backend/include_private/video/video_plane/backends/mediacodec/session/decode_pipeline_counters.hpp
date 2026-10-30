#pragma once

#include <atomic>
#include <cstdint>

namespace kivo::video::mediacodec {

/// 流水线的热路径计数器：两段各自写，`snapshot()` 从任意线程读。
///
/// 全部是原子量而不是「加锁改整个结构」：热路径每帧都要更新好几项，
/// 若每帧都锁一次并复制字符串，等于自己给解码器加了一道枷锁（P12-B 的教训）。
///
/// 分两组而不是一个：
/// - **输入组**只由送包段写（`samples_*`）；
/// - **输出组**只由收帧段写（`frames_*`）。
/// 这样"谁写的"是确定的，出现不一致时不用猜是哪一段的问题。
struct DecodePipelineCounters {
    // ---- 输入段（送包线程单写）----
    std::atomic<std::int64_t> samples_submitted{0};
    std::atomic<std::int64_t> bytes_submitted{0};
    std::atomic<std::int64_t> samples_rejected{0};  ///< 被显式拒绝的包（如加密样本）

    // ---- 输出段（收帧线程单写）----
    /// 从解码器取走的缓冲数，**含末尾那个 EOS 缓冲**（它占一个输出槽，不是画面）。
    /// 因此 `frames_received == frames_rendered + frames_dropped_late + 1`。
    /// 与 `frames_released` 必须严格相等 —— 不等就是有输出槽没归还，
    /// 那是会让解码器彻底停住的一类 bug。
    std::atomic<std::int64_t> frames_received{0};
    std::atomic<std::int64_t> frames_released{0};
    std::atomic<std::int64_t> frames_rendered{0};
    std::atomic<std::int64_t> frames_dropped_late{0};

    /// 因为"没有面可上"而丢弃的帧（P12-D1）。与 `frames_dropped_late` 分开记：
    /// 那个是节拍判的"太晚了不要"，这个是"想显示但没地方显示"。
    /// 两者混在一起，一换面就会看起来像"节拍崩了"。
    std::atomic<std::int64_t> frames_discarded_no_surface{0};
    std::atomic<std::int64_t> first_frame_latency_us{-1};  ///< -1 表示还没出过帧
    /// seek 后首帧的媒体 pts（P12-F2）。与上面那个是两件事：
    /// 那个是"多久出帧"，这个是"出的是哪一刻的帧"。错序/跳错位置会先体现在这里。
    std::atomic<std::int64_t> first_frame_pts_us{-1};

    // ---- 终态 ----
    /// 收帧段结束时落一次（不是 `stop()` 时）：用它当分母算 fps，
    /// 若在 stop() 时才落，播放结束到销毁之间的空闲会被算进解码耗时。
    std::atomic<std::int64_t> wall_micros{0};
    std::atomic<bool> completed{false};

    /// 复用同一个会话对象再跑一轮时清账。只允许在没有任何 worker 运行时调用。
    void reset() noexcept {
        samples_submitted.store(0, std::memory_order_relaxed);
        bytes_submitted.store(0, std::memory_order_relaxed);
        samples_rejected.store(0, std::memory_order_relaxed);
        frames_received.store(0, std::memory_order_relaxed);
        frames_released.store(0, std::memory_order_relaxed);
        frames_rendered.store(0, std::memory_order_relaxed);
        frames_dropped_late.store(0, std::memory_order_relaxed);
        first_frame_latency_us.store(-1, std::memory_order_relaxed);
        first_frame_pts_us.store(-1, std::memory_order_relaxed);
        wall_micros.store(0, std::memory_order_relaxed);
        completed.store(false, std::memory_order_relaxed);
    }
};

}  // namespace kivo::video::mediacodec
