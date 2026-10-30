#pragma once

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

#include "video/video_plane/core/audio/spsc_audio_ring_buffer.hpp"
#include "video/video_plane/probe/audio/aaudio_tone_source.hpp"

namespace kivo::video::video_plane::probe {

/// 生产侧计数。**只有在 stop() 返回之后读取才有效**（线程已被 join）。
struct ToneRingFeedStats {
    std::int64_t frames_pushed = 0;    ///< 成功写进环形缓冲的帧数
    std::int64_t short_writes = 0;     ///< 缓冲满导致的短写次数（供大于求）
    std::int64_t push_rounds = 0;      ///< 供给轮次
};

/// 模拟解码线程：把正弦样本按指定倍速推进环形缓冲。
///
/// 存在理由：环形缓冲单测只能证明"两个线程之间不丢不乱"，
/// 但它证明不了"接到真实音频会话上，欠载到底会不会被如实记下来"。
/// 这个供给器就是用来做这件事的：
///   - speed = 1.0 → 正常供给，`frames_silenced` 应当接近 0；
///   - speed = 0.5 → 故意供不上，`frames_silenced` **必须**明显大于 0。
///
/// 第二条是本探针的真正价值：一个计数器如果从来没有非零过，
/// 就分不清它是"真的没欠载"还是"根本没在计"。
class ToneRingProducer {
public:
    /// @param sample_rate 名义采样率，用于决定每次供给的粒度（10 ms）
    /// @param speed       供给倍速，必须 > 0
    ToneRingProducer(core::audio::SpscAudioRingBuffer& ring,
                     AaudioToneSource& tone,
                     std::int32_t sample_rate,
                     double speed);

    ~ToneRingProducer();

    ToneRingProducer(const ToneRingProducer&) = delete;
    ToneRingProducer& operator=(const ToneRingProducer&) = delete;
    ToneRingProducer(ToneRingProducer&&) = delete;
    ToneRingProducer& operator=(ToneRingProducer&&) = delete;

    /// 启动前把缓冲灌满。不预灌的话，开流瞬间必然是空的，
    /// 头几十毫秒的 `frames_silenced` 会掩盖后面真正想观察的现象。
    void prefill();

    void start();
    void stop();

    [[nodiscard]] ToneRingFeedStats stats() const { return stats_; }

private:
    void loop();
    void push_chunk();

    core::audio::SpscAudioRingBuffer& ring_;
    AaudioToneSource& tone_;
    std::int32_t chunk_frames_ = 0;
    double speed_ = 1.0;

    std::vector<float> chunk_;  ///< 构造期分配，线程内不再分配
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};

    ToneRingFeedStats stats_;
};

}  // namespace kivo::video::video_plane::probe
