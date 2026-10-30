#include "video/video_plane/core/audio/spsc_audio_ring_buffer.hpp"

#include <bit>
#include <cstring>
#include <stdexcept>

namespace kivo::video::core::audio {
namespace {

/// 向上取整到 2 的幂（容量 0 → 1，由调用方校验，这里只管位运算）。
constexpr std::int32_t ceil_pow2(std::int32_t value) noexcept {
    const auto widened = static_cast<std::uint32_t>(value);
    return static_cast<std::int32_t>(std::bit_ceil(widened));
}

/// 把"可读/可写帧数"夹到 [0, capacity]。
/// 理论上索引差不会越界，但两个索引是两次独立读取的瞬时快照，
/// 若恰好读到消费者已推进、生产者未推进的中间态，差值可能短暂为负或超容。
/// 夹一次比让调用方拿到负数安全。
std::int32_t clamp_to_capacity(std::int64_t value, std::int32_t capacity) noexcept {
    if (value <= 0) {
        return 0;
    }
    if (value >= capacity) {
        return capacity;
    }
    return static_cast<std::int32_t>(value);
}

}  // namespace

SpscAudioRingBuffer::SpscAudioRingBuffer(std::int32_t channel_count, std::int32_t capacity_frames) {
    if (channel_count <= 0) {
        throw std::invalid_argument("SpscAudioRingBuffer: channel_count 必须为正");
    }
    if (capacity_frames <= 0) {
        throw std::invalid_argument("SpscAudioRingBuffer: capacity_frames 必须为正");
    }

    channel_count_ = channel_count;
    capacity_frames_ = ceil_pow2(capacity_frames);
    index_mask_ = capacity_frames_ - 1;

    const auto sample_count = static_cast<std::size_t>(capacity_frames_) * static_cast<std::size_t>(channel_count);
    samples_ = std::make_unique<float[]>(sample_count);
    std::memset(samples_.get(), 0, sample_count * sizeof(float));
}

SpscAudioRingBuffer::~SpscAudioRingBuffer() = default;

std::int32_t SpscAudioRingBuffer::write(const float* interleaved, std::int32_t frame_count) noexcept {
    if (interleaved == nullptr || frame_count <= 0) {
        return 0;
    }

    // 自己写的索引 relaxed 读即可（只有本线程会改它）；
    // 对方的索引必须 acquire，否则可能读到"空间已释放"却看不见新数据。
    const auto write_index = write_index_.load(std::memory_order_relaxed);
    const auto read_index = read_index_.load(std::memory_order_acquire);

    const auto free_frames = clamp_to_capacity(
        static_cast<std::int64_t>(capacity_frames_) - (write_index - read_index), capacity_frames_);
    if (free_frames == 0) {
        return 0;  // 短写：调用方据此记账，不要在这里等待
    }

    const std::int32_t frames_to_write = frame_count < free_frames ? frame_count : free_frames;

    const auto channel_stride = static_cast<std::size_t>(channel_count_);
    const auto head = static_cast<std::int32_t>(write_index & index_mask_);
    const std::int32_t until_end = capacity_frames_ - head;
    const std::int32_t first_chunk = frames_to_write < until_end ? frames_to_write : until_end;

    std::memcpy(samples_.get() + static_cast<std::size_t>(head) * channel_stride,
                interleaved,
                static_cast<std::size_t>(first_chunk) * channel_stride * sizeof(float));

    if (frames_to_write > first_chunk) {
        const std::int32_t second_chunk = frames_to_write - first_chunk;
        std::memcpy(samples_.get(),
                    interleaved + static_cast<std::size_t>(first_chunk) * channel_stride,
                    static_cast<std::size_t>(second_chunk) * channel_stride * sizeof(float));
    }

    // release：保证上面的 memcpy 对随后 acquire 到本索引的消费者可见。
    write_index_.store(write_index + frames_to_write, std::memory_order_release);
    return frames_to_write;
}

std::int32_t SpscAudioRingBuffer::read(float* interleaved, std::int32_t frame_count) noexcept {
    if (interleaved == nullptr || frame_count <= 0) {
        return 0;
    }

    const auto read_index = read_index_.load(std::memory_order_relaxed);
    const auto write_index = write_index_.load(std::memory_order_acquire);

    const auto available = clamp_to_capacity(write_index - read_index, capacity_frames_);
    if (available == 0) {
        return 0;  // 空读：不触碰输出缓冲，由会话层补静音并计数
    }

    const std::int32_t frames_to_read = frame_count < available ? frame_count : available;

    const auto channel_stride = static_cast<std::size_t>(channel_count_);
    const auto head = static_cast<std::int32_t>(read_index & index_mask_);
    const std::int32_t until_end = capacity_frames_ - head;
    const std::int32_t first_chunk = frames_to_read < until_end ? frames_to_read : until_end;

    std::memcpy(interleaved,
                samples_.get() + static_cast<std::size_t>(head) * channel_stride,
                static_cast<std::size_t>(first_chunk) * channel_stride * sizeof(float));

    if (frames_to_read > first_chunk) {
        const std::int32_t second_chunk = frames_to_read - first_chunk;
        std::memcpy(interleaved + static_cast<std::size_t>(first_chunk) * channel_stride,
                    samples_.get(),
                    static_cast<std::size_t>(second_chunk) * channel_stride * sizeof(float));
    }

    read_index_.store(read_index + frames_to_read, std::memory_order_release);
    return frames_to_read;
}

std::int32_t SpscAudioRingBuffer::available_frames() const noexcept {
    const auto write_index = write_index_.load(std::memory_order_acquire);
    const auto read_index = read_index_.load(std::memory_order_acquire);
    return clamp_to_capacity(write_index - read_index, capacity_frames_);
}

std::int32_t SpscAudioRingBuffer::free_frames() const noexcept {
    return capacity_frames_ - available_frames();
}

void SpscAudioRingBuffer::reset() noexcept {
    read_index_.store(0, std::memory_order_relaxed);
    write_index_.store(0, std::memory_order_relaxed);
}

}  // namespace kivo::video::core::audio
