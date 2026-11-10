#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace kivo::video::avsync {

struct SoftAudioStats {
    std::int64_t packets_read = 0;
    std::int64_t packets_sent = 0;
    std::int64_t frames_decoded = 0;
    std::int64_t frames_written = 0;
    std::int64_t ring_full_waits = 0;
    std::int64_t read_errors = 0;
    double decode_ms_total = 0;
    std::int64_t wall_us = 0;
    std::string codec;
    std::string container;
    int source_sample_rate = 0;
    int source_channels = 0;
    int output_sample_rate = 0;
    int output_channels = 0;
    std::string error;
    bool reached_eos = false;
};

/// 软解音频通路：fd → FFmpeg 解封装 → FFmpeg 解码 → swresample → 环形缓冲 → AAudio。
///
/// 为什么必须自成一条：设备只提供 AAC / MP3 / FLAC / Opus / Vorbis 的硬解，
/// TrueHD / AC3 / EAC3 / DTS 一个都没有（真机 media_codecs 清单为证），
/// 而这些正是 REMUX 片源的常客。平台的 `MediaExtractor` 也认不全这些轨道，
/// 所以连解封装都得自己做——**整条链路都不经过 MediaCodec**。
///
/// 与视频软解（`FfmpegLocalPlaybackSession`）刻意分开：那是画面上屏，
/// 这里是声音出声，两者的失败语义、线程模型、时钟来源都不同。
/// 唯一的共同点是输入都是同一个片源的另一个 fd。
///
/// 线程模型：`start()` 起一个"解封装+解码+供数"线程，AAudio 在自己的实时线程
/// 取数。两者只通过 SPSC 环和原子量交互，不加锁。
class SoftAudioSession {
public:
    SoftAudioSession();
    ~SoftAudioSession();

    SoftAudioSession(const SoftAudioSession&) = delete;
    SoftAudioSession& operator=(const SoftAudioSession&) = delete;

    /// 记录输入并为输出参数做准备。**不做任何 IO**：解封装在 worker 线程里做，
    /// 因为 SMB 上一次 open 就是一次网络往返，不能占调用线程。
    [[nodiscard]] bool open(int fd, std::int64_t length, const std::string& label);
    [[nodiscard]] bool start();

    /// 暂停 / 恢复。暂停会**停掉 AAudio 流**——只置标志的话环里那 500 ms
    /// 还会继续响，用户按下暂停后声音仍在走。环里的数据原样保留，
    /// 恢复时从断点接着放，既不丢也不会重复。
    void setPaused(bool paused);
    [[nodiscard]] bool isPaused() const noexcept {
        return paused_.load(std::memory_order_relaxed);
    }

    /// 定位。异步：worker 在下一轮自行执行 seek + flush + 环重置。
    void seekTo(std::int64_t target_us);

    /// 当前媒体位置（µs）。-1 = 还没播出第一帧。
    ///
    /// **注意语义**：这是"最后一帧解码并写进环"的位置，它领先**实际出声**整整一个
    /// 环的深度（最多 500 ms + 预灌 200 ms）。想做音画对齐必须用 [playedUs]，
    /// 拿这个去和画面比会系统性偏早几百毫秒 —— 那正是"看起来差一点"却怎么调都不对的来源。
    [[nodiscard]] std::int64_t positionUs() const noexcept {
        return position_us_.load(std::memory_order_relaxed);
    }

    /// 实际正在出声的媒体位置（µs）。-1 = 还没开始出声。
    ///
    /// 由"已写入位置 − 环里尚未被取走的时长"得到，直接对应听众此刻听到的那一帧。
    /// 视频侧要以它为准做对齐。
    [[nodiscard]] std::int64_t playedUs() const noexcept {
        return played_us_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::int64_t durationUs() const noexcept {
        return duration_us_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] bool isRunning() const noexcept {
        return running_.load(std::memory_order_relaxed);
    }
    /// 是否真的开出了 AAudio 流（false = 片源没有可解音轨，或设备打不开）。
    [[nodiscard]] bool hasOutput() const noexcept {
        return output_open_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] const SoftAudioStats& stats() const noexcept { return stats_; }

    /// 停线程、停流、收统计。可重复调用。
    void stop();

private:
    void run_loop_();

    bool opened_ = false;
    int fd_ = -1;
    std::int64_t length_ = -1;
    std::string label_;

    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> output_open_{false};
    std::atomic<std::int64_t> seek_request_us_{-1};
    std::atomic<std::int64_t> position_us_{-1};
    std::atomic<std::int64_t> played_us_{-1};
    std::atomic<std::int64_t> duration_us_{-1};
    std::mutex pause_mutex_;
    std::condition_variable pause_cv_;

    SoftAudioStats stats_;
    std::thread worker_;
};

}  // namespace kivo::video::avsync
