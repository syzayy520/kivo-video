#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

namespace kivo::video::core::audio {

/// 单生产者单消费者（SPSC）无锁音频环形缓冲。
///
/// 存在理由：解码线程产出 PCM 的速度与音频硬件取走的速度永远不相等，
/// 中间必须有一段缓冲吸收抖动。AAudio 回调线程（iOS 上是 AudioUnit 回调）
/// 属于实时线程，**不能加锁**——一旦被解码线程持有的锁挡住，
/// 表现就是爆音与 xrun，而且现场无法复现。
///
/// 使用契约（违反即未定义行为，不加运行时检查以免污染热路径）：
///   - **只有一个线程可以调用 write()**（生产者，通常是解码线程）；
///   - **只有一个线程可以调用 read()**（消费者，通常是音频回调线程）；
///   - capacity_frames()/available_frames()/free_frames() 可在任意线程调用，
///     但返回值只是瞬时快照，不构成任何"读了就一定有"的保证；
///   - reset() 只能在两个线程都已停止时调用。
///
/// 线程安全的实现依据：生产者与消费者各自独占一个单调递增索引，
/// 只读取对方的索引（acquire）来判断可用空间，写完数据后用 release 发布自己的索引。
/// 因为索引永不回绕（int64，48kHz 下可用约 600 万年），
/// 不存在 ABA，也不需要序号回绕处理。
#if defined(_MSC_VER)
// C4324「由于对齐说明符，结构被填充」在本类是**故意结果**：
// 填充就是为了让两个索引落在不同缓存行。关掉它而不是改回 alignas(1)，
// 否则每次构建都会有人提议"修掉这个填充"。
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
class SpscAudioRingBuffer {
public:
    /// @param channel_count  交错声道数，必须 > 0
    /// @param capacity_frames 期望容量（帧）。内部向上取整到 2 的幂以便用位与取模；
    ///                        实际容量以 capacity_frames() 为准，不小于本参数。
    ///
    /// 构造期一次性分配样本区，之后 write()/read() 全程零分配。
    SpscAudioRingBuffer(std::int32_t channel_count, std::int32_t capacity_frames);

    ~SpscAudioRingBuffer();

    SpscAudioRingBuffer(const SpscAudioRingBuffer&) = delete;
    SpscAudioRingBuffer& operator=(const SpscAudioRingBuffer&) = delete;
    SpscAudioRingBuffer(SpscAudioRingBuffer&&) = delete;
    SpscAudioRingBuffer& operator=(SpscAudioRingBuffer&&) = delete;

    /// 生产线程调用。写入不超过 frame_count 帧，返回**实际写入**的帧数。
    ///
    /// 空间不足时返回"短写"（可能为 0），**绝不阻塞、绝不覆盖未读数据**。
    /// 短写是正常信号而非错误：调用方应当记录欠载并把余量留给下一轮，
    /// 而不是扩容或等待——在音频路径上等待就等于制造 xrun。
    std::int32_t write(const float* interleaved, std::int32_t frame_count) noexcept;

    /// 消费线程调用（实时）。读取不超过 frame_count 帧，返回实际读到的帧数。
    ///
    /// 缓冲空时返回 0，且**不修改输出缓冲**——由调用方决定补静音还是保留上一帧。
    /// 会话层据此统计 frames_silenced，这就是欠载的度量口径。
    std::int32_t read(float* interleaved, std::int32_t frame_count) noexcept;

    /// 实际容量（帧），已按 2 的幂向上取整。
    [[nodiscard]] std::int32_t capacity_frames() const noexcept { return capacity_frames_; }

    [[nodiscard]] std::int32_t channel_count() const noexcept { return channel_count_; }

    /// 当前可读帧数（瞬时快照）。
    [[nodiscard]] std::int32_t available_frames() const noexcept;

    /// 当前可写帧数（瞬时快照）。
    [[nodiscard]] std::int32_t free_frames() const noexcept;

    /// 丢弃所有未读数据。仅在两个线程都停止时调用。
    void reset() noexcept;

private:
    std::int32_t channel_count_ = 0;
    std::int32_t capacity_frames_ = 0;
    std::int32_t index_mask_ = 0;  ///< capacity_frames_ - 1，用于位与取模

    std::unique_ptr<float[]> samples_;  ///< 容量 × 声道，构造期分配，之后不再改变

    // 两个索引分处不同缓存行：生产者写 write_index_、消费者写 read_index_，
    // 放在同一行会导致两个核反复使对方缓存行失效（伪共享），
    // 在 4K 这种高帧率场景下足以吃掉几个百分点的吞吐。
    alignas(64) std::atomic<std::int64_t> write_index_{0};
    alignas(64) std::atomic<std::int64_t> read_index_{0};
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}  // namespace kivo::video::core::audio
