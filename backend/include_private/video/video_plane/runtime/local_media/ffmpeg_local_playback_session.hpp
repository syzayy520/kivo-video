#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"
#include "video/video_plane/runtime/local_media/ffmpeg_fd_callbacks.hpp"
#include "video/video_plane/runtime/local_media/ffmpeg_frame_sink.hpp"

namespace kivo::video::video_plane::runtime::local_media {

struct FfmpegSessionOutcome {
    enum class Code {
        kOk = 0,
        kOpenFailed = 1,
        kDemuxFailed = 2,
        kNoVideoStream = 3,
        kDecoderFailed = 4,
        kAlreadyRunning = 5,
        kNotStarted = 6,
    };
    Code code = Code::kOk;
    std::string message;
    [[nodiscard]] bool ok() const { return code == Code::kOk; }
};

struct FfSessionCounters {
    std::int64_t packets_read = 0;
    std::int64_t video_packets = 0;
    std::int64_t keyframes = 0;
    std::int64_t frames_decoded = 0;
    std::int64_t frames_rendered = 0;
    std::int64_t render_failures = 0;
    /// 为对齐音频时钟主动丢弃的帧数（落后超过阈值）。
    /// 这个数不是"故障"，是"为了对齐付出的代价"；但它必须可见 ——
    /// 持续增长说明视频跟不上声音，否则用户只会看到画面在跳而没人知道为什么。
    std::int64_t frames_dropped = 0;
    std::int64_t read_errors = 0;
    /// 读包耗时（ms 累计，含被丢弃的非视频包）。
    /// 与 `decode_ms_total` 分开记账才能回答"慢在读还是在解"——
    /// 之前只有一个 decode 数，读者只能靠猜（2026-11-10 真机 0.28x）。
    double read_ms_total = 0;
    double decode_ms_total = 0;
    std::int64_t wall_us = 0;
};

/// 软解旁路会话（211）：fd 回调 → demux → avcodec 软解 → sink 呈现。
///
/// **与硬解会话（`DecodeSession`）刻意分开**：生命周期几乎无公共面，
/// 硬抽基类只会得到一堆 unsupported。证据面与之**同构**（同名计数 +
/// end_reason + `decoder.backend`），差异如实出版（M1：无 seek/pause，
/// 节拍是 simple pts sleep，不是硬解的 FramePacer）。
///
/// 线程模型：`start()` 起一个 demux+decode 线程，`stop()` 置停 + join。
/// **fd 的所有权留在调用方**（JNI/Kotlin 护栏），会话只借用。
class FfmpegLocalPlaybackSession {
public:
    struct Config {
        int fd = -1;                 ///< 借用的文件描述符
        std::int64_t offset = 0;     ///< 数据起点
        std::int64_t length = -1;    ///< 数据长度；-1 = 量到 fd 末尾
        /// URL 输入（220）：非空时走 avformat URL 直开（http://127.0.0.1 本地
        /// Range 代理），fd 三项被忽略。与 fd 路径二选一。
        std::string url;
        std::string label;           ///< 证据里的媒体名（显示用）
        /// 路由事实（211 AC5）：Kotlin 侧路由判定把「走了哪条、为什么」传进来，
        /// 会话原样出版进证据（会话自己不做路由判定——判定在 Java 平台 API 上）。
        std::string routing_decision;  ///< "mediacodec" / "ffmpeg"
        std::string routing_reason;    ///< 非空：判据原文（容器不可解/无解码器/…）
    };

    FfmpegLocalPlaybackSession() = default;
    ~FfmpegLocalPlaybackSession();
    FfmpegLocalPlaybackSession(const FfmpegLocalPlaybackSession&) = delete;
    FfmpegLocalPlaybackSession& operator=(const FfmpegLocalPlaybackSession&) = delete;

    /// 打开 demux + 视频解码器（`sink` 借用，须活过 stop()）。
    [[nodiscard]] FfmpegSessionOutcome open(const Config& config, IFfFrameSink* sink);
    /// 起解线程。已运行时拒绝。
    [[nodiscard]] FfmpegSessionOutcome start();
    /// 停线程 + 收证据。证据 JSON 通过 `evidence_json` 带出（`kivo.p211.decode/1`）。
    [[nodiscard]] FfmpegSessionOutcome stop(std::string& evidence_json);

    [[nodiscard]] const FfSessionCounters& counters() const noexcept { return counters_; }
    [[nodiscard]] std::string backend_name() const noexcept { return "ffmpeg"; }
    [[nodiscard]] bool is_running() const noexcept {
        return running_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::int64_t source_width() const noexcept { return source_width_; }
    [[nodiscard]] std::int64_t source_height() const noexcept { return source_height_; }

    /// seek（220）：异步请求——run_loop 每轮检查（worker 线程内执行真正的
    /// avformat_seek_file + 解码器 flush + 节拍重锚）。fd/URL 两种输入都支持。
    /// **暂停状态下也可调用**：请求会把等待中的 worker 唤醒，定位完成后仍保持暂停。
    [[nodiscard]] FfmpegSessionOutcome seekTo(std::int64_t target_us);
    /// 暂停 / 恢复（软解补齐）。
    ///
    /// 语义是"停止取包与呈现"，不是"丢掉解码器"：worker 阻塞在条件变量上，
    /// 恢复时把节拍原点推到当前，避免把暂停期间攒下的时间一次性追回来。
    /// **必须在 open 之后、start 前后皆可**；重复调用同一状态是幂等的。
    [[nodiscard]] FfmpegSessionOutcome setPaused(bool paused);
    [[nodiscard]] bool isPaused() const noexcept {
        return paused_.load(std::memory_order_relaxed);
    }
    /// 当前媒体位置（µs；最后渲染帧的 media 时间；seek 后即目标值）。-1 = 未知。
    [[nodiscard]] std::int64_t positionUs() const noexcept {
        return position_us_.load(std::memory_order_relaxed);
    }

    /// 外部音频时钟：**正在出声**的媒体位置（µs）。传 -1 退回自身 pts 节拍。
    ///
    /// 为什么需要它：视频和音频是两条独立的会话，各自从 pts=0 起跑，但启动时刻
    /// 天然不同（音频要 demux + 打开 AAudio，在 20 GB REMUX 上晚 1 秒以上）。
    /// 两条 1.0x 的时间轴一旦错开一个固定量，就会**永远**错开 —— 各自守时的结果是
    /// 音画相差一秒多，正是用户说的"音画不同步"。
    ///
    /// 有了这个时钟，视频就不再守自己的表，而是贴着声音走：
    /// 超前就睡、落后就丢帧。代价只在起播的头一两秒（跳过音频尚未就绪的那一段），
    /// 换来的是全程对齐。
    void setAudioClockUs(std::int64_t us) noexcept {
        audio_clock_us_.store(us, std::memory_order_relaxed);
    }
    /// 容器时长（µs；open 时 probe_container 得出）。-1 = 未知。
    [[nodiscard]] std::int64_t durationUs() const noexcept {
        return duration_us_.load(std::memory_order_relaxed);
    }
    /// 结束原因（"completed"/"interrupted"/"error: ..."）——断连上报（AC4）用。
    [[nodiscard]] std::string end_reason() const noexcept { return end_reason_; }
    [[nodiscard]] bool completed() const noexcept { return completed_; }

private:
    void run_loop_();

    /// 当前音频主时钟（µs），已按墙钟外推；-1 = 不可用。
    /// 采样每 200 ms 才来一次，不外推的话每帧都会看到同一个值，
    /// 节拍会出现 200 ms 的台阶（观感是"一顿一顿"）。
    [[nodiscard]] std::int64_t effective_audio_clock_us_();

    bool opened_ = false;
    bool completed_ = false;
    Config config_{};
    IFfFrameSink* sink_ = nullptr;  // 借用
    FfFdSource fd_source_{};
    std::atomic<std::int64_t> seek_request_us_{-1};   // -1 = 无请求
    /// 暂停：true 时 worker 阻塞在 `pause_cv_` 上，不取包、不渲染、不睡节拍。
    /// 与 `seek_request_us_` 共用一个条件变量：暂停中来的 seek 必须先被处理，
    /// 否则用户暂停后拖进度条会毫无反应——那是"卡死"而不是"暂停"。
    std::atomic<bool> paused_{false};
    std::mutex pause_mutex_;
    std::condition_variable pause_cv_;
    /// 暂停中刚完成一次 seek：允许 worker 越过暂停继续读包，直到**解出一帧**为止
    /// （预算用完则回到暂停）。只给"一包"是不够的——VC-1 有 frame threading 延迟，
    /// seek 后 flush 重建参考帧，前若干包本来就不产帧。只在 worker 线程读写。
    int paused_decode_budget_ = 0;
    std::atomic<std::int64_t> position_us_{-1};
    std::atomic<std::int64_t> duration_us_{-1};

    /// 音频主时钟（µs）。-1 = 音频尚未出声或没有音轨，此时退回 pts 节拍。
    /// 只由调用线程写、worker 读，relaxed 足够（读到上一次采样值不影响正确性）。
    std::atomic<std::int64_t> audio_clock_us_{-1};
    /// 上次采样本会话位置时的墙钟（µs），用于在两帧之间外推音频时钟，
    /// 避免"每 200 ms 才更新一次"造成的阶梯感。只在 worker 线程读写。
    std::int64_t audio_extrapolate_from_us_ = -1;
    std::int64_t audio_extrapolate_wall_us_ = -1;
    /// 节拍曲线采样（220 诊断）：每 100 帧记录 (wall_ms, media_s)，stop 时随证据出版
    std::vector<std::pair<std::int64_t, double>> pace_samples_;
    kivo::cinema_engine::FfmpegFormatHandle format_{};
    int video_stream_ = -1;
    std::string codec_name_;
    std::string container_;
    int source_width_ = 0;
    int source_height_ = 0;
    double source_fps_ = 0;
    kivo::cinema_engine::RealSoftwareDecodeRuntime decoder_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    FfSessionCounters counters_{};
    /// 诊断（p211 软解进度恒 -1 的归因）：首个视频包的原始 pts / 换算后的 media_us，
    /// 以及"有帧但没有可用时间戳"的帧数。三个数一起看就能分清是 pts 缺失还是换算失败。
    std::int64_t first_packet_pts_ = -1;
    std::int64_t first_media_us_ = -1;
    std::int64_t frames_without_pts_ = 0;
    /// 渲染耗时（ms 累计）。与 `counters_.decode_ms_total` 对照即可判断
    /// 1080p VC-1 只有 ~5 fps 时到底是解码慢还是写屏慢。
    double render_ms_total_ = 0;
    std::string end_reason_{"not_started"};
    std::string first_error_;
};

}  // namespace kivo::video::video_plane::runtime::local_media
