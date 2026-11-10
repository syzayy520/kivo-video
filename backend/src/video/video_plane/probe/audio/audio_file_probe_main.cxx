#include <fcntl.h>
#include <sys/stat.h>
#include <sys/system_properties.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include "video/video_plane/backends/aaudio/buffer/ring_buffer_sample_source.hpp"
#include "video/video_plane/backends/aaudio/clock/aaudio_monotonic_now.hpp"
#include "video/video_plane/backends/aaudio/session/aaudio_output_session.hpp"
#include "video/video_plane/backends/mediacodec/decode/mediacodec_audio_decoder.hpp"
#include "video/video_plane/backends/mediacodec/source/mediaextractor_source.hpp"
#include "video/video_plane/core/audio/pcm_format.hpp"
#include "video/video_plane/core/audio/spsc_audio_ring_buffer.hpp"
#include "video/video_plane/backends/mediacodec/session/audio_decode_feeder.hpp"
#include "video/video_plane/probe/audio/audio_file_probe_report.hpp"

namespace {

// 命名空间别名而不是 using namespace：
// 这个入口同时要碰 mediacodec / aaudio / core / probe 四棵子树，
// 展开整棵命名空间会让"这个名字到底属于哪一层"彻底看不见。
namespace probe = kivo::video::video_plane::probe;
namespace mediacodec = kivo::video::mediacodec;
namespace aaudio = kivo::video::aaudio;
namespace core = kivo::video::core;

/// 环形缓冲容量（毫秒）。300ms 足够吸收解码抖动与调度停顿，
/// 又不至于像 1 秒缓冲那样让"停止/切歌"明显拖尾。
constexpr std::int32_t kRingCapacityMillis = 600;  ///< 与播放器同值（252）

/// 起流前至少要灌进来的帧数，用 burst 的倍数表达：
/// 一个 burst 的量只能撑过一次回调，遇到调度抖动立刻欠载；
/// 3 个 burst 是"能扛一次卡顿"的最小值。
constexpr std::int32_t kPrefillBursts = 3;

/// 等预灌的上限。短音轨（< 300ms）永远到不了目标水位，因此这里只是上限不是要求。
constexpr int kPrefillTimeoutMillis = 500;

/// 播完尾部留一点时间让环形缓冲排空，避免把"我们自己停的"记成欠载。
constexpr int kDrainTimeoutMillis = 300;

std::string read_property(const char* name) {
    char value[PROP_VALUE_MAX] = {};
    const int length = __system_property_get(name, value);
    if (length <= 0) {
        return {};
    }
    return std::string(value, static_cast<std::size_t>(length));
}

/// 只保留文件名：证据 JSON 不该记录设备上的完整路径。
std::string display_name(const std::string& path) {
    const auto slash = path.find_last_of('/');
    return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

int parse_int(const char* text, int fallback) {
    if (text == nullptr || *text == '\0') {
        return fallback;
    }
    return static_cast<int>(std::strtol(text, nullptr, 10));
}

void emit(const probe::AudioFileProbeReport& report) {
    const std::string json = probe::render_audio_file_probe_json(report);
    std::fwrite(json.data(), 1, json.size(), stdout);
    std::fflush(stdout);
}

void fill_device(probe::AudioFileProbeReport& report) {
    report.device_model = read_property("ro.product.model");
    report.device_abi = read_property("ro.product.cpu.abi");
    report.device_sdk = parse_int(read_property("ro.build.version.sdk").c_str(), 0);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr,
                     "用法：%s <媒体文件路径> [最长播放毫秒]\n"
                     "  不带时长 = 播完整段；JSON 走 stdout，摘要走 stderr。\n",
                     argv[0]);
        return 2;
    }

    const std::string path = argv[1];
    const int max_millis = parse_int(argc > 2 ? argv[2] : nullptr, 0);

    probe::AudioFileProbeReport report;
    fill_device(report);
    report.label = display_name(path);

    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        report.failure = "cannot open " + path;
        emit(report);
        std::fprintf(stderr, "[kivo] open failed: %s\n", path.c_str());
        return 3;
    }

    struct stat file_info {};
    if (::fstat(fd, &file_info) != 0) {
        ::close(fd);
        report.failure = "fstat failed for " + path;
        emit(report);
        return 3;
    }
    report.file_bytes = static_cast<std::int64_t>(file_info.st_size);

    // 1) 解封装：选音频轨
    mediacodec::MediaExtractorSource source;
    const auto extracted =
        source.open_fd(fd, 0, file_info.st_size, report.label, mediacodec::TrackKind::kAudio);
    if (!extracted.ok()) {
        ::close(fd);
        report.failure = extracted.message;
        emit(report);
        std::fprintf(stderr, "[kivo] extract failed: %s\n", extracted.message.c_str());
        return 4;
    }

    const auto track = source.audio_track_info();
    report.mime = track.mime;
    report.declared_sample_rate = track.sample_rate;
    report.declared_channel_count = track.channel_count;
    report.container_duration_us = track.duration_us;

    // 2) 解码器。声明格式只用于"背离检测"，不用于转换。
    mediacodec::MediaCodecAudioDecoder decoder;

    core::audio::PcmFormat declared;
    declared.sample_rate = track.sample_rate;
    declared.channel_count = track.channel_count;
    // 容器里没有 pcm-encoding；这里填 16bit 只是为了让 declared 通过 is_valid()，
    // 真正的输出编码由解码器收到首块后回报。
    declared.encoding = core::audio::PcmEncoding::kInt16;
    decoder.set_declared_format(declared);

    auto configured = decoder.configure(track.mime, source.track_format());
    if (!configured.ok()) {
        ::close(fd);
        report.failure = configured.message;
        emit(report);
        return 5;
    }
    auto decoder_started = decoder.start();
    if (!decoder_started.ok()) {
        ::close(fd);
        report.failure = decoder_started.message;
        emit(report);
        return 5;
    }
    report.codec_name = decoder.codec_name();
    ::close(fd);  // fd 只在 open 期间需要；解封装器已复制所需信息

    // 3) 环形缓冲（可移植核心）
    core::audio::SpscAudioRingBuffer ring(
        track.channel_count, track.sample_rate * kRingCapacityMillis / 1000);
    report.ring_capacity_frames = ring.capacity_frames();

    // 4) AAudio 输出，按轨道声明的采样率/声道请求
    aaudio::AaudioStreamConfig config;
    config.sample_rate = track.sample_rate;
    config.channel_count = track.channel_count;
    config.format = aaudio::AaudioStreamConfig::SampleFormat::kFloat;
    config.prefer_exclusive = true;
    config.prefer_low_latency = true;
    report.config = config;

    aaudio::RingBufferSampleSource ring_source(ring);
    aaudio::AaudioOutputSession session;
    report.open = session.open(config, ring_source);
    if (!report.open.ok()) {
        report.failure = report.open.message;
        emit(report);
        std::fprintf(stderr, "[kivo] aaudio open failed: %s\n", report.open.message.c_str());
        return 6;
    }

    // 协商结果与片源不一致就停：本项目不做重采样（Direct Play First）。
    // 硬播只会得到音高错误的输出，而且听感上很难归因。
    if (report.open.sample_rate != track.sample_rate ||
        report.open.channel_count != track.channel_count) {
        report.failure = "aaudio negotiated " + std::to_string(report.open.sample_rate) + "Hz/" +
                         std::to_string(report.open.channel_count) + "ch but source is " +
                         std::to_string(track.sample_rate) + "Hz/" +
                         std::to_string(track.channel_count) +
                         "ch; resampling is intentionally not implemented";
        session.close();
        emit(report);
        std::fprintf(stderr, "[kivo] %s\n", report.failure.c_str());
        return 7;
    }

    // 5) 解码线程 + 预灌 + 起流
    mediacodec::AudioDecodeFeeder feeder(source, decoder, ring);
    feeder.start();

    const std::int32_t prefill_target = report.open.frames_per_burst * kPrefillBursts;
    report.prefill_reached = feeder.wait_filled(prefill_target, kPrefillTimeoutMillis);

    report.start = session.start();
    if (!report.start.ok()) {
        feeder.stop();
        session.stop();
        report.failure = report.start.message;
        emit(report);
        std::fprintf(stderr, "[kivo] aaudio start failed: %s\n", report.start.message.c_str());
        return 8;
    }

    // 起流后有一段"时间戳还不可用"的窗口 —— NDK 文档明说 requestStart 是异步的，
    // 这期间 `getTimestamp` 返回 INVALID_STATE 且**不算故障**。
    // 这段窗口的长度就是启动开销，必须以 1 ms 粒度探出来，而不是靠墙钟估。
    auto* clock = session.clock();
    if (clock != nullptr) {
        constexpr std::int64_t kStartupProbeDeadlineNs = 2'000'000'000;  // 2 s 上限
        const std::int64_t deadline = session.start_mono_ns() + kStartupProbeDeadlineNs;

        bool first_query = true;
        while (true) {
            const auto snapshot = clock->now();
            if (first_query) {
                // 启动期读到的位置必然是 0：设备还没播，不存在"已经走过去的时间"。
                report.clock_start_ns = snapshot.position_ns;
                first_query = false;
            }
            if (clock->stats().first_ok_mono_ns >= 0) {
                break;
            }
            if (aaudio::monotonic_now_ns() > deadline) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        const auto startup_stats = clock->stats();
        if (startup_stats.first_ok_mono_ns >= 0) {
            report.startup_millis =
                (startup_stats.first_ok_mono_ns - session.start_mono_ns()) / 1'000'000;
        }
    }

    // 6) 播放窗口：默认整段；给了毫秒数就按它截断
    const std::int64_t budget_millis =
        (max_millis > 0)
            ? max_millis
            : ((track.duration_us > 0) ? (track.duration_us / 1000 + 150) : 3000);

    const auto began = std::chrono::steady_clock::now();
    while (true) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - began)
                                 .count();
        if (elapsed >= budget_millis) {
            break;
        }
        // 解封装到底且环也空了，才算播完。只看其中一条会把尾巴截掉。
        if (feeder.finished() && ring.available_frames() == 0) {
            break;
        }
        // 每 5 ms 采一次时钟：让外推与单调守卫在真实查询节奏下被压一遍，
        // 而不是只在收尾时读一次数字。
        if (clock != nullptr) {
            (void)clock->now();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // 7) 收尾顺序有讲究：先停供给，再让环排空，最后停流。
    //    反过来（先停流）会让环里剩下的数据永远没人取，被记成大量欠载。
    const bool truncated = !feeder.finished();
    feeder.stop();
    for (int i = 0; i < kDrainTimeoutMillis && ring.available_frames() > 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // 时钟必须在 stop 之前取：流一停，getTimestamp 立刻退回 INVALID_STATE，
    // 那时再读只会拿到回退档的数字，证据就不再是"播放期间的真实时钟"了。
    if (clock != nullptr) {
        report.clock_end_ns = clock->now().position_ns;
        report.clock = clock->stats();
    }

    report.stats = session.stats();
    session.stop();

    report.feed = feeder.stats();
    report.output_format = decoder.output_format();
    report.output_format_diverged = decoder.output_format_diverged();
    report.wall_millis = static_cast<std::int64_t>(report.stats.wall_millis);
    if (report.failure.empty() && !report.feed.error.empty()) {
        report.failure = report.feed.error;
    }

    emit(report);

    const std::int64_t clock_span_ns = report.clock_end_ns - report.clock_start_ns;
    const std::int64_t rendered_span_ns =
        (report.open.sample_rate > 0)
            ? report.stats.frames_rendered * 1'000'000'000LL / report.open.sample_rate
            : 0;
    const double span_ratio =
        (rendered_span_ns > 0) ? static_cast<double>(clock_span_ns) / rendered_span_ns : 0.0;

    std::fprintf(stderr,
                 "[kivo] clock: source=%s queries=%lld startup=%lldms clock_span=%.3fs "
                 "rendered_span=%.3fs ratio=%.4f | ok=%lld not_started=%lld fail=%lld "
                 "fallback(read/written/frozen)=%lld/%lld/%lld rejects=%lld clamped=%lld "
                 "latency=%lldframes first_ok_frame=%lld\n",
                 kivo::video::core::audio::audio_clock_source_name(report.clock.dominant_source()),
                 static_cast<long long>(report.clock.queries),
                 static_cast<long long>(report.startup_millis),
                 static_cast<double>(clock_span_ns) / 1e9,
                 static_cast<double>(rendered_span_ns) / 1e9, span_ratio,
                 static_cast<long long>(report.clock.timestamp_ok),
                 static_cast<long long>(report.clock.timestamp_not_started),
                 static_cast<long long>(report.clock.timestamp_fail),
                 static_cast<long long>(report.clock.fallback_frames_read),
                 static_cast<long long>(report.clock.fallback_frames_written),
                 static_cast<long long>(report.clock.stale_freezes),
                 static_cast<long long>(report.clock.monotonic_rejects),
                 static_cast<long long>(report.clock.jumps_clamped),
                 static_cast<long long>(report.clock.latency_frames),
                 static_cast<long long>(report.clock.first_ok_frame_position));

    std::fprintf(stderr,
                 "[kivo] %s: %lldHz/%dch %s | decoded=%lld rendered=%lld silenced=%lld "
                 "dropped=%lld xrun=%d eos=%d%s\n",
                 report.label.c_str(), static_cast<long long>(report.output_format.sample_rate),
                 report.output_format.channel_count,
                 std::string(core::audio::pcm_encoding_name(report.output_format.encoding)).c_str(),
                 static_cast<long long>(report.feed.frames_decoded),
                 static_cast<long long>(report.stats.frames_rendered),
                 static_cast<long long>(report.stats.frames_silenced),
                 static_cast<long long>(report.feed.frames_dropped), report.stats.xrun_count,
                 report.feed.reached_eos ? 1 : 0, truncated ? " (truncated by time budget)" : "");

    return report.failure.empty() ? 0 : 1;
}
