#include "video/video_plane/probe/audio/tone_ring_producer.hpp"

#include <chrono>
#include <stdexcept>

namespace kivo::video::video_plane::probe {
namespace {

constexpr std::int32_t kChunkMillis = 10;

}  // namespace

ToneRingProducer::ToneRingProducer(core::audio::SpscAudioRingBuffer& ring,
                                   AaudioToneSource& tone,
                                   std::int32_t sample_rate,
                                   double speed)
    : ring_(ring), tone_(tone), speed_(speed) {
    if (sample_rate <= 0) {
        throw std::invalid_argument("ToneRingProducer: sample_rate 必须为正");
    }
    if (!(speed > 0.0)) {
        throw std::invalid_argument("ToneRingProducer: speed 必须为正");
    }

    chunk_frames_ = sample_rate / 1000 * kChunkMillis;
    if (chunk_frames_ <= 0) {
        throw std::invalid_argument("ToneRingProducer: 采样率过低，无法形成 10ms 供给粒度");
    }
    chunk_.assign(static_cast<std::size_t>(chunk_frames_) * static_cast<std::size_t>(ring_.channel_count()),
                  0.0f);
}

ToneRingProducer::~ToneRingProducer() { stop(); }

void ToneRingProducer::prefill() {
    // 一直灌到写不下为止：此时缓冲是满的，消费者一开始就有余量。
    while (true) {
        const std::int32_t written = [&] {
            tone_.render(chunk_.data(), chunk_frames_);
            return ring_.write(chunk_.data(), chunk_frames_);
        }();
        if (written <= 0) {
            return;
        }
        stats_.frames_pushed += written;
        if (written < chunk_frames_) {
            return;
        }
    }
}

void ToneRingProducer::start() {
    if (running_.exchange(true)) {
        return;
    }
    stop_requested_.store(false, std::memory_order_relaxed);
    worker_ = std::thread([this] { loop(); });
}

void ToneRingProducer::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    stop_requested_.store(true, std::memory_order_relaxed);
    if (worker_.joinable()) {
        worker_.join();
    }
}

void ToneRingProducer::loop() {
    // 供给周期 = 10ms / speed：半速（0.5）时每 20ms 供 10ms 的量，
    // 供给速率就是实时的 0.5 倍，欠载必然发生——这正是我们要观察的现象。
    //
    // 用**绝对截止时刻**而不是 sleep_for 相对睡眠：
    // sleep_for(10ms) 实际总比 10ms 略长（唤醒开销），误差会一轮轮累加，
    // 实测 5 秒就累积出 40ms 的供给缺口，被误记成"环形缓冲欠载"。
    // sleep_until 每轮对齐同一时间轴，单轮超时不会累积到下一轮。
    const auto period = std::chrono::microseconds(
        static_cast<std::int64_t>(static_cast<double>(kChunkMillis) * 1000.0 / speed_));

    auto next_deadline = std::chrono::steady_clock::now();
    while (!stop_requested_.load(std::memory_order_relaxed)) {
        push_chunk();
        next_deadline += period;
        std::this_thread::sleep_until(next_deadline);
    }
}

void ToneRingProducer::push_chunk() {
    tone_.render(chunk_.data(), chunk_frames_);
    const std::int32_t written = ring_.write(chunk_.data(), chunk_frames_);
    stats_.frames_pushed += written;
    ++stats_.push_rounds;
    if (written < chunk_frames_) {
        // 缓冲满：说明我们供得比设备取得快，这不算错误，
        // 但如果长期为 0 就说明缓冲容量过大，会白增延迟。
        ++stats_.short_writes;
    }
}

}  // namespace kivo::video::video_plane::probe
