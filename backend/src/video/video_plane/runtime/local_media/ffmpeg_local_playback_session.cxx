#include "video/video_plane/runtime/local_media/ffmpeg_local_playback_session.hpp"

#include <chrono>
#include <cstdio>
#include <limits>
#include <utility>

#ifdef __ANDROID__
#include <android/log.h>
#define KIVO_SOFT_LOG(...) \
    __android_log_print(ANDROID_LOG_INFO, "KivoSoftDecode", __VA_ARGS__)
#else
#define KIVO_SOFT_LOG(...) ((void)0)
#endif

namespace kivo::video::video_plane::runtime::local_media {
namespace {

std::string escape_json(const std::string& in) {
    std::string out;
    for (char c : in) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

std::int64_t steady_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

/// 画面落后音频超过这个量就丢帧追赶。
///
/// 人耳对"声音早于画面"的容忍只有约 40 ms，但丢帧本身是可感知的，
/// 所以不能一落后就丢：150 ms 是"再等一帧也追不回来"的量级，
/// 低于它靠下一次不睡眠自然就追上了。
constexpr std::int64_t kDropBehindUs = 150'000;
/// 画面超前音频这么多就开始睡。留 50 ms 是因为渲染到真正上屏还有几毫秒，
/// 且每帧渲染耗时本身有抖动；贴着 0 睡会让画面频繁赶在声音前面。
constexpr std::int64_t kSleepAheadUs = 50'000;
/// 音频时钟最多外推这么远。Kotlin 侧每 200 ms 推一次，超时未更新说明
/// 音频侧卡住或已停，此时必须停止外推，否则视频会一路跑到前面去。
constexpr std::int64_t kAudioClockExtrapolateLimitUs = 250'000;
/// 音频时钟多久没更新就判定为"不再可用"，视频退回自身节拍。
///
/// 必须存在：片尾音频先结束、或音频被暂停时，`playedUs` 会定格。
/// 若继续以它为准，视频每帧都判"画面超前"于是越睡越久 —— 画面就此冻在最后一帧，
/// 观众看到的是"片子播到一半卡死"。800 ms 是"推了四次都没变"的量级。
constexpr std::int64_t kAudioClockStaleUs = 800'000;

}  // namespace

std::int64_t FfmpegLocalPlaybackSession::effective_audio_clock_us_() {
    const std::int64_t raw = audio_clock_us_.load(std::memory_order_relaxed);
    if (raw < 0) {
        audio_extrapolate_from_us_ = -1;
        audio_extrapolate_wall_us_ = -1;
        return -1;
    }
    const std::int64_t now = steady_us();
    if (raw != audio_extrapolate_from_us_) {
        // 新的采样到：以它为基准，从现在开始外推。
        audio_extrapolate_from_us_ = raw;
        audio_extrapolate_wall_us_ = now;
        return raw;
    }
    if (audio_extrapolate_wall_us_ < 0) {
        return raw;
    }
    const std::int64_t elapsed = now - audio_extrapolate_wall_us_;
    if (elapsed > kAudioClockStaleUs) {
        // 音频已经不再前进（片尾结束 / 被暂停 / 线程死了）。
        // 必须交还主导权，否则视频会一直"等一个永远不会来的时间点"。
        audio_extrapolate_from_us_ = -1;
        audio_extrapolate_wall_us_ = -1;
        return -1;
    }
    if (elapsed > kAudioClockExtrapolateLimitUs) {
        return raw;
    }
    return raw + elapsed;
}

FfmpegLocalPlaybackSession::~FfmpegLocalPlaybackSession() {
    std::string evidence;
    // 析构里证据没人接：显式弃值（nodiscard 是给正常 stop 路径的）。
    (void)stop(evidence);
}

FfmpegSessionOutcome FfmpegLocalPlaybackSession::open(const Config& config,
                                                      IFfFrameSink* sink) {
    if (running_.load(std::memory_order_relaxed)) {
        return {FfmpegSessionOutcome::Code::kAlreadyRunning, "session is running"};
    }
    // fd 校验仅适用于 fd 输入路径（211 本地文件）；URL 输入（220 SMB/本地代理）
    // 无 fd——url 非空即视为合法输入。
    if (config.fd < 0 && config.url.empty()) {
        return {FfmpegSessionOutcome::Code::kOpenFailed, "fd is negative"};
    }
    if (sink == nullptr) {
        return {FfmpegSessionOutcome::Code::kOpenFailed, "sink is null"};
    }
    config_ = config;
    sink_ = sink;
    opened_ = true;
    return {};
}

FfmpegSessionOutcome FfmpegLocalPlaybackSession::seekTo(std::int64_t target_us) {
    if (!opened_) {
        return {FfmpegSessionOutcome::Code::kNotStarted, "open() first"};
    }
    seek_request_us_.store(target_us, std::memory_order_relaxed);
    // 暂停中也要能定位：worker 正阻塞在条件变量上，不叫醒它就永远看不到这个请求。
    pause_cv_.notify_all();
    return {};
}

FfmpegSessionOutcome FfmpegLocalPlaybackSession::setPaused(bool paused) {
    if (!opened_) {
        return {FfmpegSessionOutcome::Code::kNotStarted, "open() first"};
    }
    paused_.store(paused, std::memory_order_relaxed);
    // 两种方向都要唤醒：暂停时 worker 要立刻停下来，恢复时要立刻继续。
    pause_cv_.notify_all();
    return {};
}

FfmpegSessionOutcome FfmpegLocalPlaybackSession::start() {
    if (!opened_) {
        return {FfmpegSessionOutcome::Code::kNotStarted, "open() first"};
    }
    if (running_.load(std::memory_order_relaxed)) {
        return {FfmpegSessionOutcome::Code::kAlreadyRunning, "session is running"};
    }
    running_.store(true, std::memory_order_relaxed);
    // 解码拆分计时是全局累加的，每次起播先清零，否则读到的是上一部片子的均值。
    kivo::cinema_engine::FfmpegAdapter::reset_decode_timings();
    worker_ = std::thread([this] { run_loop_(); });
    return {};
}

FfmpegSessionOutcome FfmpegLocalPlaybackSession::stop(std::string& evidence_json) {
    if (!opened_) {
        return {FfmpegSessionOutcome::Code::kNotStarted, "nothing to stop"};
    }
    if (worker_.joinable()) {
        running_.store(false, std::memory_order_relaxed);
        // 暂停中的 worker 正阻塞在条件变量上：不叫醒它，join 会一直等下去。
        paused_.store(false, std::memory_order_relaxed);
        pause_cv_.notify_all();
        worker_.join();
    }
    opened_ = false;

    // ---- 证据（kivo.p211.decode/1）：与硬解证据面同构的计数 + 差异如实出版 ----
    std::string json = "{\n";
    json += "  \"schema\": \"kivo.p211.decode/1\",\n";
    json += "  \"decoder\": { \"backend\": \"ffmpeg\", \"codec\": \"" +
            escape_json(codec_name_) + "\" },\n";
    json += "  \"media\": \"" + escape_json(config_.label) + "\",\n";
    json += "  \"container\": \"" + escape_json(container_) + "\",\n";
    json += "  \"source\": { \"width\": " + std::to_string(source_width_) +
            ", \"height\": " + std::to_string(source_height_) + " },\n";
    json += "  \"counters\": {\n";
    json += "    \"packets_read\": " + std::to_string(counters_.packets_read) + ",\n";
    json += "    \"video_packets\": " + std::to_string(counters_.video_packets) + ",\n";
    json += "    \"keyframes\": " + std::to_string(counters_.keyframes) + ",\n";
    json += "    \"frames_decoded\": " + std::to_string(counters_.frames_decoded) + ",\n";
    json += "    \"frames_rendered\": " + std::to_string(counters_.frames_rendered) + ",\n";
    json += "    \"render_failures\": " + std::to_string(counters_.render_failures) + ",\n";
    json += "    \"frames_dropped\": " + std::to_string(counters_.frames_dropped) + ",\n";
    json += "    \"read_errors\": " + std::to_string(counters_.read_errors) + "\n";
    json += "  },\n";
    json += "  \"timing\": {\n";
    json += "    \"wall_us\": " + std::to_string(counters_.wall_us) + ",\n";
    json += "    \"read_ms_total\": " + std::to_string(counters_.read_ms_total) + ",\n";
    json += "    \"decode_ms_total\": " + std::to_string(counters_.decode_ms_total) + ",\n";
    json += "    \"render_ms_total\": " + std::to_string(render_ms_total_) + "\n";
    json += "  },\n";
    // 24x：把 decode_ms_total 再拆一层。`avcodec_ms_per_frame` 是解码器本身的成本，
    // `sws_ms_per_frame` 是 1080p YUV→RGBA 的成本，两者之和才等于单帧解码耗时。
    // 没有这两个数就只能凭感觉"优化解码"，而真正的大头可能在色彩空间转换上。
    {
        const auto timings = kivo::cinema_engine::FfmpegAdapter::take_decode_timings();
        const double frames = timings.frames > 0 ? static_cast<double>(timings.frames) : 0.0;
        const double avcodec_ms = frames > 0 ? timings.avcodec_ns / 1e6 / frames : 0.0;
        const double sws_ms = frames > 0 ? timings.sws_ns / 1e6 / frames : 0.0;
        json += "  \"decode_breakdown\": { \"frames\": " + std::to_string(timings.frames) +
                ", \"send_ms_per_call\": " +
                std::to_string(timings.send_calls > 0
                                   ? timings.send_ns / 1e6 / static_cast<double>(timings.send_calls)
                                   : 0.0) +
                ", \"send_calls\": " + std::to_string(timings.send_calls) +
                ", \"avcodec_ms_per_frame\": " + std::to_string(avcodec_ms) +
                ", \"sws_ms_per_frame\": " + std::to_string(sws_ms) + " },\n";
    }
    json += "  \"render\": { \"sink\": \"" + std::string(sink_ ? sink_->name() : "none") +
            "\" },\n";
    json += "  \"routing\": { \"backend_decision\": \"" +
            escape_json(config_.routing_decision) + "\", \"reason\": \"" +
            escape_json(config_.routing_reason) + "\" },\n";
    json += "  \"pacing\": \"pts_sleep_simple\",\n";
    // 进度诊断：首包原始 pts、首个可用 media_us、无时间戳帧数。
    // 三个数一起读：frames_without_pts 等于 frames_decoded 就是"完全没时间戳"。
    json += "  \"pts_diag\": { \"first_packet_pts\": " + std::to_string(first_packet_pts_) +
            ", \"first_media_us\": " + std::to_string(first_media_us_) +
            ", \"frames_without_pts\": " + std::to_string(frames_without_pts_) + " },\n";
    // 节拍曲线（220 诊断）：[wall_ms, media_s] 采样对
    json += "  \"pace_curve\": [";
    {
        bool first_sample = true;
        for (const auto& sample : pace_samples_) {
            if (!first_sample) json += ",";
            first_sample = false;
            json += "[" + std::to_string(sample.first) + "," +
                    std::to_string(sample.second) + "]";
        }
    }
    json += "],\n";
    json += "  \"end_reason\": \"" + escape_json(end_reason_) + "\",\n";
    json += "  \"first_error\": " +
            std::string(first_error_.empty() ? "null" : "\"" + escape_json(first_error_) + "\"") +
            "\n";
    json += "}\n";
    evidence_json = std::move(json);
    return {};
}

void FfmpegLocalPlaybackSession::run_loop_() {
    // **FFmpeg 全生命周期都在本线程**：建（demux/解码器）、解、毁一律不跨线程。
    // 跨线程使用 FFmpeg 句柄（主线程建、工作线程用）会在 Windows 的
    // MSVC-Debug + FFmpeg-DLL 组合上把退出期炸成 abort（exit=3）——
    // 二分实验记录在本测试文件（--trio-only/--decode-inline/--thread-only/
    // --thread-ffmpeg-use 四段）。单线程持有一切后，桌面测试干净退出。
    const std::int64_t started_us = steady_us();
    std::int64_t first_pts_us = -1;
    std::int64_t last_media_us = -1;
    std::int64_t pace_origin_us = 0;
    bool completed = false;
    bool error_reported = false;

    // 1. 输入源二选一：URL 直开（220 SMB 本地代理）或 fd 回调（211 本地文件）。
    if (!config_.url.empty()) {
        format_ = kivo::cinema_engine::FfmpegAdapter::open_url(config_.url);
    } else {
        fd_source_.fd = config_.fd;
        fd_source_.offset = config_.offset;
        fd_source_.length = config_.length;
        fd_source_.cursor = 0;
        auto callbacks = make_ff_fd_callbacks(&fd_source_);
        kivo::cinema_engine::FfmpegInputConfig io_config;
        io_config.callbacks = callbacks;
        format_ = kivo::cinema_engine::FfmpegAdapter::open_custom_io(io_config);
    }
    double duration = 0;
    std::int64_t bitrate = 0;
    const bool probe_ok = format_.native != nullptr &&
        kivo::cinema_engine::FfmpegAdapter::probe_container(format_, container_,
                                                            duration, bitrate);
    if (probe_ok && duration > 0) {
        duration_us_.store(static_cast<std::int64_t>(duration * 1e6),
                           std::memory_order_relaxed);
    }
    if (format_.native == nullptr || !probe_ok) {
        counters_.wall_us = steady_us() - started_us;
        end_reason_ = "error: demux open/probe failed";
        if (format_.native != nullptr) {
            kivo::cinema_engine::FfmpegAdapter::free_format_context(format_);
        }
        running_.store(false, std::memory_order_relaxed);
        return;
    }
    std::vector<int> video_indices;
    std::vector<int> audio_indices;
    std::vector<int> subtitle_indices;
    if (!kivo::cinema_engine::FfmpegAdapter::enumerate_streams(
            format_, video_indices, audio_indices, subtitle_indices) ||
        video_indices.empty()) {
        counters_.wall_us = steady_us() - started_us;
        end_reason_ = "error: no video stream";
        kivo::cinema_engine::FfmpegAdapter::free_format_context(format_);
        running_.store(false, std::memory_order_relaxed);
        return;
    }
    video_stream_ = video_indices.front();
    // 只留视频这一路：其余轨道标丢弃，demuxer 就不再为它们付载荷代价。
    // 没有这一步时每个视频帧要额外读 ~50 个包（实测 ~35 ms/帧，几乎等于
    // 一个 24 fps 的帧间隔），播放速率被压到 0.3x。
    kivo::cinema_engine::FfmpegAdapter::discard_streams_except(format_, video_stream_);
    kivo::cinema_engine::FfmpegAdapter::get_stream_codec_name(format_, video_stream_,
                                                              codec_name_);
    std::string source_pixel_format;
    kivo::cinema_engine::FfmpegAdapter::get_stream_video_info(
        format_, video_stream_, source_width_, source_height_, source_fps_,
        source_pixel_format);

    auto codec_handle =
        kivo::cinema_engine::FfmpegAdapter::create_codec_context(format_, video_stream_);
    if (codec_handle.native == nullptr ||
        !decoder_.adopt_codec_context(codec_handle.native, codec_name_, "p211_session")) {
        counters_.wall_us = steady_us() - started_us;
        end_reason_ = "error: decoder open failed (" + decoder_.error_code() + ")";
        kivo::cinema_engine::FfmpegAdapter::free_format_context(format_);
        running_.store(false, std::memory_order_relaxed);
        return;
    }

    // 2. 解码循环（本线程）。
    int consecutive_read_errors = 0;
    while (running_.load(std::memory_order_relaxed)) {
        // seek 请求（220）：worker 线程内执行——avformat_seek_file（选最近关键帧）
        // + 解码器 flush（丢弃 seek 前残留帧）+ 节拍重锚到目标位置。
        if (const auto req = seek_request_us_.exchange(-1, std::memory_order_relaxed);
            req >= 0) {
            if (kivo::cinema_engine::FfmpegAdapter::seek_file_us(
                    format_, video_stream_, req)) {
                for (int i = 0; i < 16; ++i) {
                    auto fr = decoder_.flush("p220_seek");
                    if (!fr.success || fr.frame.frame_data.empty()) break;
                }
                // **不把请求值写进 position**：请求不是落点。真实落点由下一帧的 pts 给出，
                // 提前写等于"进度条到了、画面没动"——那是最难被发现的假成功。
                first_pts_us = -1;
                last_media_us = -1;
                pace_origin_us = steady_us();
                // 定位后音频位置会跳变，旧的外推基准必须作废，否则视频会按
                // 定位前的时间线继续睡/丢，跳转后头一两秒全是错位。
                audio_extrapolate_from_us_ = -1;
                audio_extrapolate_wall_us_ = -1;
                // 暂停中定位：越过暂停继续读，直到解出一帧为止（预算见下）。
                if (paused_.load(std::memory_order_relaxed)) {
                    paused_decode_budget_ = 240;
                }
            } else {
                // 定位被拒：**不重试**。同一个目标再试一次还是同样结果，而重试会把
                // 请求存回去，让暂停中的 wait 条件永远成立 —— 那是忙等烧 CPU、
                // 画面又不动。留在原位继续播，进度条不动是诚实的反馈。
                if (first_error_.empty()) {
                    first_error_ = "seek rejected by demuxer";
                }
            }
        }
        // 暂停（软解补齐）：阻塞在这里，而不是"关掉解码器再重建"——恢复后接着
        // 同一个码流位置往下走，不丢帧、不重开 SMB 连接。
        //
        // 暂停期间**不取包**：既省 CPU，也不会把网络读的一侧拖进超时重连。
        // 谓词里带上 seek：暂停时拖进度条必须仍能定位，否则用户看到的是"卡死"。
        if (paused_.load(std::memory_order_relaxed) && paused_decode_budget_ <= 0) {
            std::unique_lock<std::mutex> lock(pause_mutex_);
            pause_cv_.wait(lock, [this] {
                return !paused_.load(std::memory_order_relaxed) ||
                       !running_.load(std::memory_order_relaxed) ||
                       seek_request_us_.load(std::memory_order_relaxed) >= 0;
            });
            if (!running_.load(std::memory_order_relaxed)) {
                break;
            }
            if (!paused_.load(std::memory_order_relaxed)) {
                // 真恢复：把节拍原点推到"现在"。
                // 不重锚的话 due 会落后 now 整个暂停时长，恢复瞬间连续补帧——那是快进不是继续。
                if (first_pts_us >= 0 && last_media_us >= 0) {
                    pace_origin_us = steady_us() - (last_media_us - first_pts_us);
                }
            }
            // 两种情形都回循环顶部：真恢复要让 seek 处理（若有）先跑；
            // 仍暂停则一定是 seek 唤醒的，必须在**定位之后**才解那一帧。
            continue;
        }
        if (paused_decode_budget_ > 0) {
            --paused_decode_budget_;
        }
        const auto read_started = std::chrono::steady_clock::now();
        auto read_result = kivo::cinema_engine::FfmpegAdapter::read_packet(format_);
        counters_.read_ms_total +=
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() -
                                                      read_started)
                .count();
        if (read_result.eof) {
            completed = true;
            break;
        }
        if (!read_result.success) {
            ++counters_.read_errors;
            // 断连上报（AC4）：连续读失败超阈值 → 终止循环并出版 reason
            if (++consecutive_read_errors > 200) {
                end_reason_ = "error: read failed repeatedly (" +
                              (read_result.error_message.empty() ? "no msg"
                                                                 : read_result.error_message) +
                              ")";
                break;
            }
            if (!error_reported && !read_result.error_message.empty()) {
                first_error_ = read_result.error_message;
                error_reported = true;
            }
            continue;
        }
        consecutive_read_errors = 0;
        if (read_result.eof) {
            completed = true;
            break;
        }
        ++counters_.packets_read;
        if (!read_result.success) {
            ++counters_.read_errors;
            if (!error_reported && !read_result.error_message.empty()) {
                first_error_ = read_result.error_message;
                error_reported = true;
            }
            continue;
        }
        if (read_result.packet.stream_index != video_stream_) {
            continue;
        }
        ++counters_.video_packets;
        if (first_packet_pts_ < 0) {
            first_packet_pts_ = read_result.packet.pts;
        }
        if (read_result.packet.is_key_frame) {
            ++counters_.keyframes;
        }
        // 桌面 pump 的转换逐字段照抄（local_video_runtime_pump.cpp:201-211）。
        kivo::cinema_engine::KivoPacket packet;
        packet.stream_index = read_result.packet.stream_index;
        packet.pts = read_result.packet.pts;
        packet.dts = read_result.packet.dts;
        packet.duration = read_result.packet.duration;
        packet.is_key_frame = read_result.packet.is_key_frame;
        packet.data = read_result.packet_data;
        for (auto& side : read_result.side_data) {
            packet.side_data.push_back({side.type, std::move(side.data)});
        }

        const auto decode_started = std::chrono::steady_clock::now();
        auto result = decoder_.decode_video_rgba(packet, "p211_session");
        counters_.decode_ms_total +=
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() -
                                                      decode_started)
                .count();
        if (!result.success || result.needs_more_input || result.eof) {
            continue;
        }
        if (result.frame.width <= 0 || result.frame.height <= 0 ||
            result.frame.frame_data.empty()) {
            continue;
        }
        ++counters_.frames_decoded;
        // 24x：分段记账。只报"解码多少毫秒"会把结论带偏 —— 实测 send 只有 7.7 ms，
        // 但每帧实际走了 139 ms，差的那 130 ms 究竟在等 SMB 读、等 Surface 出缓冲，
        // 还是在 sleep，只有把每帧总时长与各段并列才看得出来。
        if (counters_.frames_decoded % 60 == 0) {
            const auto t = kivo::cinema_engine::FfmpegAdapter::take_decode_timings();
            const double frames = static_cast<double>(counters_.frames_decoded);
            const double wall_ms = (steady_us() - started_us) / 1000.0;
            const double per_frame_ms = wall_ms / frames;
            const double send_ms =
                t.send_calls > 0 ? t.send_ns / 1e6 / static_cast<double>(t.send_calls) : 0.0;
            const double recv_ms = t.frames > 0 ? t.avcodec_ns / 1e6 / static_cast<double>(t.frames) : 0.0;
            const double sws_ms = t.frames > 0 ? t.sws_ns / 1e6 / static_cast<double>(t.frames) : 0.0;
            KIVO_SOFT_LOG(
                "frame_budget frames=%lld fps=%.1f per=%.1fms | send=%.1f recv=%.1f sws=%.1f "
                "resize=%.1f read=%.1f render=%.1f | decode_total=%.1f",
                static_cast<long long>(counters_.frames_decoded),
                wall_ms > 0 ? frames * 1000.0 / wall_ms : 0.0,
                per_frame_ms, send_ms, recv_ms, sws_ms,
                t.frames > 0 ? t.resize_ns / 1e6 / static_cast<double>(t.frames) : 0.0,
                counters_.read_ms_total / frames,
                render_ms_total_ / frames,
                counters_.decode_ms_total / frames);
        }

        // 节拍（simple pts sleep）：首帧起锚，之后每帧睡到"起点 + 媒体时间差"。
        // 差异如实出版（pacing 字段）；比硬解的 FramePacer 粗，M1 不追求等价。
        std::int64_t media_us = 0;
        // VC-1 等部分软解码器**不**回填 `AVFrame::pts`，它保持 `AV_NOPTS_VALUE`
        // （= INT64_MIN，非 0）。直接拿去 rescale 会溢出成负数，于是 `media_us > 0`
        // 永远不成立 —— 真机表现是"画面在动、进度条停在 0:00:00"（2026-11-10 实测）。
        // 这里显式把它当作"没有帧时间戳"，退回包时间戳。
        constexpr std::int64_t kNopts = std::numeric_limits<std::int64_t>::min();
        const std::int64_t frame_pts = result.frame.pts;
        const std::int64_t packet_pts = read_result.packet.pts;
        const std::int64_t pts =
            (frame_pts != 0 && frame_pts != kNopts) ? frame_pts
            : (packet_pts != kNopts ? packet_pts : 0);
        if (kivo::cinema_engine::FfmpegAdapter::rescale_stream_timestamp_us(
                format_, video_stream_, pts, media_us) &&
            media_us > 0) {
            position_us_.store(media_us, std::memory_order_relaxed);
            last_media_us = media_us;
            if (first_media_us_ < 0) {
                first_media_us_ = media_us;
            }
            // 节拍曲线采样（220 诊断）：每 100 帧记 (wall_ms, media_s)
            if (counters_.frames_decoded % 100 == 0) {
                pace_samples_.emplace_back((steady_us() - started_us) / 1000,
                                           media_us / 1e6);
            }
            if (first_pts_us < 0) {
                first_pts_us = media_us;
                pace_origin_us = steady_us();
            }

            // 节拍分两条路：
            //   有音频时钟 → **贴着声音走**（音频是主时钟）；
            //   没有 → 退回自身 pts 节拍（无音轨 / 音频还没出声 / 暂停中）。
            //
            // 为什么必须让音频做主：视频和音频是两条独立会话，各自从 pts=0 起跑，
            // 但启动时刻天然不同（20 GB REMUX 上音频要 demux + 探针 + 打开 AAudio，
            // 实测晚 1.4 s）。两条 1.0x 的时间轴错开一个固定量就会**永远**错开 ——
            // 各自守时，音画就差一秒多。这不是精度问题，是"谁说了算"的问题。
            const std::int64_t clock_us = effective_audio_clock_us_();
            if (clock_us > 0) {
                const std::int64_t ahead_us = media_us - clock_us;
                if (ahead_us < -kDropBehindUs) {
                    // 画面落后声音太多：直接丢这一帧去追。
                    // **丢帧必须归还缓冲**，否则每丢一帧漏掉一块 8.3 MB 的 RGBA，
                    // 几十帧就能把进程撑爆。
                    ++counters_.frames_dropped;
                    kivo::cinema_engine::FfmpegAdapter::recycle_frame_buffer(
                        std::move(result.frame.frame_data));
                    continue;
                }
                if (ahead_us > kSleepAheadUs) {
                    // 分段睡：起播时音频没就绪、视频可能一次要等一秒多，一口气睡死
                    // 会让这段时间里的 seek / 暂停毫无响应。每 100 ms 出来看一眼。
                    std::int64_t remain = ahead_us - kSleepAheadUs;
                    while (remain > 0 && running_.load(std::memory_order_relaxed)) {
                        const std::int64_t chunk =
                            remain > 100'000 ? 100'000 : remain;
                        std::this_thread::sleep_for(std::chrono::microseconds(chunk));
                        remain -= chunk;
                        if (seek_request_us_.load(std::memory_order_relaxed) >= 0 ||
                            paused_.load(std::memory_order_relaxed)) {
                            break;
                        }
                    }
                }
                // 落在 [−150 ms, +50 ms] 里：立刻上屏，不睡。
                // 这个窗口就是"已经在正确位置"的定义，睡反而是错的。
                if (counters_.frames_decoded % 60 == 0) {
                    KIVO_SOFT_LOG("avsync media=%lld clock=%lld ahead=%lld dropped=%lld",
                                  static_cast<long long>(media_us),
                                  static_cast<long long>(clock_us),
                                  static_cast<long long>(ahead_us),
                                  static_cast<long long>(counters_.frames_dropped));
                }
            } else {
                const std::int64_t due = pace_origin_us + (media_us - first_pts_us);
                std::int64_t now = steady_us();
                if (due > now) {
                    std::this_thread::sleep_for(std::chrono::microseconds(due - now));
                }
            }
        } else {
            // 有帧但拿不到可用时间戳：进度条停住就是这个分支被走穿了。
            // 计数进证据，下一次真机就不用再猜。
            if (first_media_us_ < 0) {
                ++frames_without_pts_;
            }
        }

        std::string render_error;
        const auto render_started = std::chrono::steady_clock::now();
        const bool rendered = sink_ != nullptr && sink_->render(result.frame, render_error);
        render_ms_total_ +=
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() -
                                                      render_started)
                .count();
        if (rendered) {
            ++counters_.frames_rendered;
            // 暂停中 seek 的目的已经达到：新位置的一帧解出并上屏，立刻回到暂停。
            paused_decode_budget_ = 0;
        } else {
            ++counters_.render_failures;
            if (!error_reported && !render_error.empty()) {
                first_error_ = render_error;
                error_reported = true;
            }
        }
        // 上屏之后这块 RGBA 就没用了，还给适配器复用 —— 不还会让下一帧重新
        // 分配 8.3 MB（实测 69 ms/帧，是解码成本的 8 倍）。
        kivo::cinema_engine::FfmpegAdapter::recycle_frame_buffer(
            std::move(result.frame.frame_data));
    }

    // 3. 清理（本线程）。
    decoder_.close();
    if (format_.native != nullptr) {
        kivo::cinema_engine::FfmpegAdapter::free_format_context(format_);
    }
    counters_.wall_us = steady_us() - started_us;
    completed_ = completed;
    if (completed) {
        end_reason_ = "completed";
    } else if (!end_reason_.empty() && end_reason_[0] == 'e') {
        // 已带 error 前缀（如断连上报）——保留
    } else {
        end_reason_ = "interrupted";
    }
    // 自然 EOS 也算停了：is_running() 的语义是"线程还活着"，
    // 不清标志会让调用方在 EOS 后白等到 stop()（桌面测试当场暴露）。
    running_.store(false, std::memory_order_relaxed);
}

}  // namespace kivo::video::video_plane::runtime::local_media
