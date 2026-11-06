#include "video/video_plane/backends/avsync/session/audio_playback.hpp"

#include <chrono>
#include <thread>

#include "video/video_plane/backends/aaudio/clock/aaudio_monotonic_now.hpp"
#include "video/video_plane/backends/aaudio/device/aaudio_format_names.hpp"
#include "video/video_plane/core/audio/pcm_format.hpp"

namespace kivo::video::avsync {
namespace {

/// 环形缓冲容量（毫秒）。与 `kivo_audio_file_probe` 取同一值：
/// 300ms 足够吸收解码抖动与调度停顿，又不至于让"停止"明显拖尾。
/// **两边必须一致** —— 否则同一片源在探针和播放器里得到不同的欠载数，
/// 而"哪一份是对照基线"就说不清了。
constexpr std::int32_t kRingCapacityMillis = 300;

/// 起流前的预灌水位，用 burst 的倍数表达：
/// 一个 burst 只能撑过一次回调，3 个才扛得住一次调度抖动。
constexpr std::int32_t kPrefillBursts = 3;

/// 等预灌的上限（不是要求）：短音轨永远到不了目标水位。
constexpr int kPrefillTimeoutMillis = 500;

/// 等时钟首个有效时间戳的上限。C4 实测启动开销 24–146 ms，2 s 是宽容的兜底。
constexpr std::int64_t kClockReadyTimeoutNs = 2'000'000'000;

/// 停止时的排空上限，避免把"我们自己停的"记成欠载。
constexpr int kDrainTimeoutMillis = 300;

}  // namespace

AudioPlayback::~AudioPlayback() {
    stop();
}

bool AudioPlayback::open(int fd, std::int64_t length, const std::string& label) {
    if (opened_) {
        stats_.error = "audio playback already open";
        return false;
    }

    const auto extracted =
        source_.open_fd(fd, 0, length, label, mediacodec::TrackKind::kAudio);
    if (!extracted.ok()) {
        // 没有音轨就走这里。**不静默**：调用方据此把 `clock_source` 写成 `system`，
        // 并在证据里标明 `av_drift` 不可用 —— 见 24-p12c8-design.md §七。
        stats_.error = extracted.message;
        return false;
    }

    const mediacodec::AudioTrackInfo& track = source_.audio_track_info();
    stats_.mime = track.mime;
    stats_.declared_sample_rate = track.sample_rate;
    stats_.declared_channel_count = track.channel_count;
    stats_.container_duration_us = track.duration_us;

    // 声明格式只用于"背离检测"，不用于转换：转换格式一律从解码器的实际输出现取。
    core::audio::PcmFormat declared;
    declared.sample_rate = track.sample_rate;
    declared.channel_count = track.channel_count;
    declared.encoding = core::audio::PcmEncoding::kInt16;
    decoder_.set_declared_format(declared);

    const auto configured = decoder_.configure(track.mime, source_.track_format());
    if (!configured.ok()) {
        stats_.error = "audio decoder configure: " + configured.message;
        return false;
    }
    const auto decoder_started = decoder_.start();
    if (!decoder_started.ok()) {
        stats_.error = "audio decoder start: " + decoder_started.message;
        return false;
    }
    stats_.codec_name = decoder_.codec_name();

    ring_ = std::make_unique<core::audio::SpscAudioRingBuffer>(
        track.channel_count, track.sample_rate * kRingCapacityMillis / 1000);
    stats_.ring_capacity_frames = ring_->capacity_frames();

    aaudio::AaudioStreamConfig config;
    config.sample_rate = track.sample_rate;
    config.channel_count = track.channel_count;
    config.format = aaudio::AaudioStreamConfig::SampleFormat::kFloat;
    config.prefer_exclusive = true;
    config.prefer_low_latency = true;

    ring_source_ = std::make_unique<aaudio::RingBufferSampleSource>(*ring_);
    output_ = std::make_unique<aaudio::AaudioOutputSession>();

    const aaudio::AaudioOpenOutcome opened = output_->open(config, *ring_source_);
    if (!opened.ok()) {
        stats_.error = "aaudio open: " + opened.message;
        output_.reset();
        ring_source_.reset();
        ring_.reset();
        return false;
    }

    // 协商结果与片源不一致就拒绝：本项目不做重采样（Direct Play First）。
    // 硬播只会得到音高错误的输出，而且听感上很难归因。
    if (opened.sample_rate != track.sample_rate ||
        opened.channel_count != track.channel_count) {
        stats_.error = "aaudio negotiated " + std::to_string(opened.sample_rate) + "Hz/" +
                       std::to_string(opened.channel_count) + "ch but source is " +
                       std::to_string(track.sample_rate) + "Hz/" +
                       std::to_string(track.channel_count) +
                       "ch; resampling is intentionally not implemented";
        output_->close();
        output_.reset();
        ring_source_.reset();
        ring_.reset();
        return false;
    }

    stats_.negotiated_sample_rate = opened.sample_rate;
    stats_.negotiated_channel_count = opened.channel_count;
    stats_.negotiated_format =
        std::string(aaudio::platform_format_name(opened.platform_format));
    stats_.sharing_mode = std::string(aaudio::sharing_mode_name(opened.platform_sharing_mode));
    stats_.performance_mode =
        std::string(aaudio::performance_mode_name(opened.platform_performance_mode));
    stats_.frames_per_burst = opened.frames_per_burst;

    // 起流后才建 feeder：先建的话，解码线程会在流还没开时就往里灌，
    // 灌满之后一直等排空，预灌水位就失去意义了。
    feeder_ = std::make_unique<mediacodec::AudioDecodeFeeder>(source_, decoder_, *ring_);
    opened_ = true;
    // P13-C：把内部状态映射到证据字段。
    // 在此之前 `stats_.opened` / `stats_.started` **从未被赋值** ——
    // 渲染器一直在输出它们，于是每一份 sync 证据都写着 `audio.opened: false`，
    // 而同一份证据里 `clock_source: audio_timestamp`、`av_drift.samples: 180`。
    // 一个**恒为 false 的字段**比缺字段更危险：它看起来是个合理的观测值。
    stats_.opened = true;
    return true;
}

bool AudioPlayback::start() {
    if (!opened_ || !feeder_ || !output_) {
        stats_.error = "audio playback is not open";
        return false;
    }
    if (started_) {
        return true;
    }

    feeder_->start();
    stats_.prefill_reached = feeder_->wait_filled(
        stats_.frames_per_burst * kPrefillBursts, kPrefillTimeoutMillis);

    const aaudio::AaudioOpenOutcome started = output_->start();
    if (!started.ok()) {
        stats_.error = "aaudio start: " + started.message;
        feeder_->stop();
        return false;
    }
    started_ = true;
    stats_.started = true;

    core::audio::AudioClockSource* source = clock();
    if (source == nullptr) {
        stats_.error = "aaudio clock unavailable after start";
        return false;
    }

    // 等时钟首次可用。这段就是"启动开销"，必须以设备读数报出来，不能靠墙钟估。
    //
    // ⚠️ 循环里**必须真的调 `now()`**：时钟的 `published_*` / `first_ok_mono_ns`
    // 只在自己被查询时推进。只读 `stats()` 而不查询，`first_ok_mono_ns` 永远是 -1，
    // 于是这里会一直等到超时并报"时钟拿不到时间戳"—— 而实际上时钟早就在走了。
    // C8 第一轮真机就是这么失败的（rc=11，reason 精确指出超时），
    // 说明"拒绝而非静默降级"这条规矩本身是对的：它把一次配置错误变成了可读的失败。
    const std::int64_t deadline = output_->start_mono_ns() + kClockReadyTimeoutNs;
    while (output_->clock()->stats().first_ok_mono_ns < 0) {
        (void)source->now();
        if (aaudio::monotonic_now_ns() > deadline) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    const aaudio::AaudioClockStats ready = output_->clock()->stats();
    if (ready.first_ok_mono_ns < 0) {
        // 时钟一直拿不到有效时间戳：拒绝，不退化成 system。
        stats_.error = "audio clock never produced a valid timestamp within 2s";
        return false;
    }
    stats_.startup_millis = (ready.first_ok_mono_ns - output_->start_mono_ns()) / 1'000'000;
    stats_.clock_start_ns = source->now().position_ns;
    return true;
}

AudioPlayback::SeekResult AudioPlayback::seek_to(std::int64_t target_us) {
    SeekResult out;
    if (!started_ || !feeder_ || !output_ || !ring_ || !ring_source_) {
        out.reason = "audio playback is not running";
        return out;
    }

    // P12-F2A：**不停止 AAudio 流**。
    //
    // F2 的失败链已经量清楚：`output.stop()+start()` 会让 `getTimestamp` 成片失效约 1.7 s，
    // 节拍器随之判 `kFreeRun`，等真时间戳回来时整段判迟到 → `catch_up` 连丢 91 帧。
    // 三次"改锚点时机"的尝试都失败，因为病根在"停流"这个动作本身。
    //
    // 于是改用**静音栅栏**：读者被挡住（`render()` 直接返回 0，一次不碰环），
    // 写者 join 掉，此时清环就是安全的 —— 不需要停流，时间戳不会断。

    // 1. P16-P1B7 / A2：显式凭据替代固定睡眠。
    //
    // 参考 F:\kivo 的 drain 契约（Q3/Q4）：完成由**控制线程**判定，音频回调不写完成状态；
    // 超时是带 deadline 的预算，不是固定 sleep，且必须有独立分类。
    //
    // 为什么 in_flight()==0 就够：计数递增早于判空（189 O1），覆盖"已过判空、还没读环"
    // 的窗口；set_muted 是 release store、回调 acquire load，此后进入的回调必被挡住、不碰环。
    // 因此归零时没有任何读者在动环，下面的 ring_->reset() 才安全。
    ring_source_->set_muted(true);
    constexpr auto kSeekGateBudget = std::chrono::milliseconds(10);
    const auto gate_deadline = std::chrono::steady_clock::now() + kSeekGateBudget;
    while (ring_source_->in_flight() != 0 &&
           std::chrono::steady_clock::now() < gate_deadline) {
        std::this_thread::yield();
    }
    if (ring_source_->in_flight() != 0) {
        // G9：不带着在途读者 reset 环；先解除静音，把非空 reason 交给上层决定是否中止。
        // 上层 resume_after_seek() 无条件执行，会话不会卡在"线程已 join"的状态。
        out.reason = "seek gate timeout: audio callback still inside render() after 10 ms; "
                     "refusing to reset the SPSC ring";
        ring_source_->set_muted(false);
        return out;
    }

    // 2. 停写者。此后没有任何人碰环。
    feeder_->stop();

    // 3. 定位 + 清在途。
    const std::int64_t landing = source_.seek_to(target_us);
    const auto flushed = decoder_.flush();
    ring_->reset();

    // 4. 重新供给并灌到与起播同一水位。
    feeder_->start();
    (void)feeder_->wait_filled(stats_.frames_per_burst * kPrefillBursts, kPrefillTimeoutMillis);

    // 5. **先放音、后对表**：放音之后第一个真实样本才是 landing 对应的音频；
    //    先锚表会让时钟在静音期间继续前进，视频反而提前。
    ring_source_->set_muted(false);
    if (auto* clock = output_->clock(); clock != nullptr && landing >= 0) {
        // 流没重启，帧计数连续，一次 rebase 足够（F2 里"锚两次吸收过渡"的做法不再需要）。
        clock->rebase(landing * 1'000);
    }

    out.landing_us = landing;
    out.ok = landing >= 0 && flushed.ok();
    if (!out.ok && out.reason.empty()) {
        out.reason = landing < 0 ? "audio extractor seek failed"
                                 : ("audio decoder flush failed: " + flushed.message);
    }
    return out;
}

bool AudioPlayback::pause() {
    // 先停供给再停流：反过来的话环会继续被填，resume 时听到的是一段"补上的旧声音"。
    if (feeder_) {
        feeder_->stop();
    }
    return output_ ? output_->pause() : false;
}

bool AudioPlayback::resume() {
    if (!output_ || !output_->resume()) {
        return false;
    }
    if (feeder_) {
        feeder_->start();  // 返回 void：起不来会在下一次 render 里表现为欠载
    }
    return true;
}

void AudioPlayback::stop() {
    if (stopped_) {
        return;
    }
    stopped_ = true;
    if (feeder_) {
        feeder_->stop();
    }
    if (ring_) {
        for (int i = 0; i < kDrainTimeoutMillis && ring_->available_frames() > 0; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // 时钟必须在 stop 之前取：流一停 `getTimestamp` 立刻退回 INVALID_STATE，
    // 那时再读只会拿到回退档的数字，证据就不再是"播放期间的真实时钟"了。
    if (output_) {
        if (auto* source = output_->clock()) {
            stats_.clock_end_ns = source->now().position_ns;
            stats_.clock = output_->clock()->stats();
        }
        stats_.output = output_->stats();
        output_->stop();
    }
    if (feeder_) {
        stats_.feed = feeder_->stats();
    }
    if (!stats_.codec_name.empty()) {
        stats_.output_format = decoder_.output_format();
        stats_.output_format_diverged = decoder_.output_format_diverged();
    }
    if (stats_.error.empty() && !stats_.feed.error.empty()) {
        stats_.error = stats_.feed.error;
    }
    started_ = false;
    // **不把 `stats_.started` 清回去。**
    // 证据是在 `stop()` 之后采集的，如果跟着内部状态一起清，每份 sync 证据都会写
    // `started: false` —— 而那一轮明明出过声。证据记的是"这一轮发生过什么"，
    // 不是"此刻的内部状态"。这是本卡修掉的第二个同类型 bug
    // （第一个是 `opened` 从未被赋值）。
}

core::audio::AudioClockSource* AudioPlayback::clock() noexcept {
    if (!output_) {
        return nullptr;
    }
    return output_->clock();
}

}  // namespace kivo::video::avsync
