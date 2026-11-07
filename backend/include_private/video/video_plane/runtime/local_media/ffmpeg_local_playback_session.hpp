#pragma once

#include <atomic>
#include <cstdint>
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
    std::int64_t read_errors = 0;
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
    [[nodiscard]] FfmpegSessionOutcome seekTo(std::int64_t target_us);
    /// 当前媒体位置（µs；最后渲染帧的 media 时间；seek 后即目标值）。-1 = 未知。
    [[nodiscard]] std::int64_t positionUs() const noexcept {
        return position_us_.load(std::memory_order_relaxed);
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

    bool opened_ = false;
    bool completed_ = false;
    Config config_{};
    IFfFrameSink* sink_ = nullptr;  // 借用
    FfFdSource fd_source_{};
    std::atomic<std::int64_t> seek_request_us_{-1};   // -1 = 无请求
    std::atomic<std::int64_t> position_us_{-1};
    std::atomic<std::int64_t> duration_us_{-1};
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
    std::string end_reason_{"not_started"};
    std::string first_error_;
};

}  // namespace kivo::video::video_plane::runtime::local_media
