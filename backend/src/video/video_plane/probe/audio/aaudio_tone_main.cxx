#include <sys/system_properties.h>

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

#include "video/video_plane/backends/aaudio/buffer/ring_buffer_sample_source.hpp"
#include "video/video_plane/backends/aaudio/session/aaudio_output_session.hpp"
#include "video/video_plane/core/audio/spsc_audio_ring_buffer.hpp"
#include "video/video_plane/probe/audio/aaudio_probe_report.hpp"
#include "video/video_plane/probe/audio/aaudio_tone_source.hpp"
#include "video/video_plane/probe/audio/tone_ring_producer.hpp"

namespace {

constexpr int kDefaultDurationSeconds = 3;
constexpr int kMaxDurationSeconds = 30;
constexpr double kDefaultFrequencyHz = 440.0;
constexpr double kToneAmplitude = 0.20;  ///< 0.2 满幅：听得清又不至于炸耳朵
constexpr std::int32_t kRingCapacityFrames = 16384;  ///< ≈341 ms @48kHz：够吸收抖动，又不白增延迟

std::string read_property(const char* name) {
    char value[PROP_VALUE_MAX] = {};
    const int length = __system_property_get(name, value);
    if (length <= 0) {
        return {};
    }
    return std::string(value, static_cast<std::size_t>(length));
}

int parse_int(const char* text, int fallback) {
    if (text == nullptr || *text == '\0') {
        return fallback;
    }
    return static_cast<int>(std::strtol(text, nullptr, 10));
}

double parse_double(const char* text, double fallback) {
    if (text == nullptr || *text == '\0') {
        return fallback;
    }
    return std::strtod(text, nullptr);
}

}  // namespace

int main(int argc, char** argv) {
    int duration_seconds = parse_int(argc > 1 ? argv[1] : nullptr, kDefaultDurationSeconds);
    if (duration_seconds <= 0) {
        duration_seconds = kDefaultDurationSeconds;
    }
    if (duration_seconds > kMaxDurationSeconds) {
        duration_seconds = kMaxDurationSeconds;
    }
    const double frequency_hz = parse_double(argc > 2 ? argv[2] : nullptr, kDefaultFrequencyHz);

    // 供给方式：direct = 正弦源直连回调（C1 基线）；ring = 经 SPSC 环形缓冲（C2 通路）。
    // 放在同一入口而不是再开一个可执行文件，是因为两者除"谁供数"之外完全一致，
    // 分开就得复制一份报告与退出码逻辑，两份真相迟早会漂移。
    const std::string feed_mode = (argc > 3 && argv[3] != nullptr) ? argv[3] : "direct";
    const double producer_speed = parse_double(argc > 4 ? argv[4] : nullptr, 1.0);
    if (feed_mode != "direct" && feed_mode != "ring") {
        std::fprintf(stderr, "用法：%s [时长秒] [频率Hz] [direct|ring] [供给倍速]\n", argv[0]);
        return 4;
    }
    if (!(producer_speed > 0.0)) {
        std::fprintf(stderr, "[kivo-audio-probe] 供给倍速必须为正\n");
        return 4;
    }

    kivo::video::aaudio::AaudioStreamConfig config;
    config.sample_rate = 48000;
    config.channel_count = 2;
    config.format = kivo::video::aaudio::AaudioStreamConfig::SampleFormat::kFloat;
    config.prefer_exclusive = true;
    config.prefer_low_latency = true;

    kivo::video::video_plane::probe::AaudioToneSource tone(
        config.sample_rate, config.channel_count, frequency_hz, kToneAmplitude);

    // 只有 ring 模式才创建缓冲与供给器：direct 模式保持 C1 的基线路径不变，
    // 这样两组数据的差异只来自"有没有缓冲层"，不受其他改动污染。
    std::unique_ptr<kivo::video::core::audio::SpscAudioRingBuffer> ring;
    std::unique_ptr<kivo::video::aaudio::RingBufferSampleSource> ring_source;
    std::unique_ptr<kivo::video::video_plane::probe::ToneRingProducer> producer;
    kivo::video::aaudio::AaudioOutputSession::SampleSource* source = &tone;

    if (feed_mode == "ring") {
        ring = std::make_unique<kivo::video::core::audio::SpscAudioRingBuffer>(
            config.channel_count, kRingCapacityFrames);
        ring_source = std::make_unique<kivo::video::aaudio::RingBufferSampleSource>(*ring);
        producer = std::make_unique<kivo::video::video_plane::probe::ToneRingProducer>(
            *ring, tone, config.sample_rate, producer_speed);
        source = ring_source.get();
    }

    kivo::video::aaudio::AaudioOutputSession session;
    const auto open_outcome = session.open(config, *source);

    const auto emit = [&](const kivo::video::aaudio::AaudioOpenOutcome& start_outcome,
                          const kivo::video::aaudio::AudioOutputStats& stats_snapshot) {
        kivo::video::video_plane::probe::AaudioProbeReport report;
        report.device_model = read_property("ro.product.model");
        report.device_abi = read_property("ro.product.cpu.abi");
        report.device_sdk = parse_int(read_property("ro.build.version.sdk").c_str(), 0);
        report.config = config;
        report.open = open_outcome;
        report.start = start_outcome;
        report.stats = stats_snapshot;
        report.requested_millis = static_cast<std::int64_t>(duration_seconds) * 1000;
        report.tone_frequency_hz = frequency_hz;
        report.feed_mode = feed_mode;
        if (producer) {
            const auto feed = producer->stats();
            report.producer_speed = producer_speed;
            report.ring_capacity_frames = ring->capacity_frames();
            report.frames_pushed = feed.frames_pushed;
            report.producer_short_writes = feed.short_writes;
        }

        const std::string json =
            kivo::video::video_plane::probe::render_aaudio_probe_json(report);
        std::fwrite(json.data(), 1, json.size(), stdout);
        std::fflush(stdout);
    };

    if (!open_outcome.ok()) {
        session.stop();
        emit(open_outcome, session.stats());
        std::fprintf(stderr, "[kivo-audio-probe] open failed: %s\n", open_outcome.message.c_str());
        return 1;
    }

    // 协商结果先打到 stderr：stdout 只留 JSON，方便直接重定向成证据文件。
    std::fprintf(stderr,
                 "[kivo-audio-probe] negotiated %d Hz / %d ch / burst %d / capacity %d / %s\n",
                 open_outcome.sample_rate,
                 open_outcome.channel_count,
                 open_outcome.frames_per_burst,
                 open_outcome.buffer_capacity_frames,
                 open_outcome.downgraded() ? open_outcome.downgrade_reason.c_str() : "no downgrade");

    // 预灌必须发生在**起流之前**。第一版放在了 start() 之后，
    // 音频流一起来就空读，前 13 个 burst（2496 帧 = 52 ms）被如实记成欠载，
    // 看上去像环形缓冲有问题，实际是启动顺序问题。
    // 这也侧面证明 frames_silenced 不是摆设——它确实抓到了我的错。
    if (producer) {
        producer->prefill();
        producer->start();
    }

    const auto start_outcome = session.start();
    if (!start_outcome.ok()) {
        if (producer) {
            producer->stop();
        }
        session.stop();
        emit(start_outcome, session.stats());
        std::fprintf(stderr, "[kivo-audio-probe] start failed: %s\n", start_outcome.message.c_str());
        return 2;
    }

    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));

    // 先停消费者再停生产者：反过来的话，从"生产者停止"到"音频流停止"之间
    // 设备仍在取数而无人供给，那段空隙会被记成 frames_silenced，
    // 看起来像环形缓冲欠载，实际是探针自己的收尾顺序造成的假象。
    // 第一版就踩了这个坑（5 秒多出 52ms 假欠载）。
    session.stop();
    if (producer) {
        producer->stop();
    }
    const auto stats_snapshot = session.stats();

    emit(start_outcome, stats_snapshot);

    const std::int64_t negotiated_rate =
        (open_outcome.sample_rate > 0) ? open_outcome.sample_rate : config.sample_rate;
    const std::int64_t expected_frames =
        negotiated_rate * static_cast<std::int64_t>(duration_seconds);
    const double render_ratio =
        (expected_frames > 0)
            ? static_cast<double>(stats_snapshot.frames_rendered) /
                  static_cast<double>(expected_frames)
            : 0.0;

    std::fprintf(stderr,
                 "[kivo-audio-probe] frames=%lld expected=%lld ratio=%.3f silenced=%lld xrun=%d\n",
                 static_cast<long long>(stats_snapshot.frames_rendered),
                 static_cast<long long>(expected_frames),
                 render_ratio,
                 static_cast<long long>(stats_snapshot.frames_silenced),
                 stats_snapshot.xrun_count);

    if (producer) {
        const auto feed = producer->stats();
        std::fprintf(stderr,
                     "[kivo-audio-probe] feed=ring speed=%.2f pushed=%lld short=%lld rounds=%lld\n",
                     producer_speed,
                     static_cast<long long>(feed.frames_pushed),
                     static_cast<long long>(feed.short_writes),
                     static_cast<long long>(feed.push_rounds));
    } else {
        std::fprintf(stderr, "[kivo-audio-probe] feed=direct speed=1.00\n");
    }

    return stats_snapshot.frames_rendered > 0 ? 0 : 3;
}
