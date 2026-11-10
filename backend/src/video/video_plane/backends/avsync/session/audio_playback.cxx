#include <cstdio>
#include "video/video_plane/backends/avsync/session/audio_playback.hpp"

#include <chrono>
#include <algorithm>
#include <thread>

#ifdef KIVO_ENABLE_FFMPEG_AUDIO
#include "video/video_plane/backends/ffmpeg/audio/ffmpeg_audio_decoder.hpp"
#endif

#include "video/video_plane/backends/aaudio/clock/aaudio_monotonic_now.hpp"
#include "video/video_plane/backends/aaudio/device/aaudio_format_names.hpp"
#include "video/video_plane/core/audio/pcm_format.hpp"

namespace kivo::video::avsync {
namespace {

/// 环形缓冲容量（毫秒）。与 `kivo_audio_file_probe` 取同一值：
/// **两边必须一致** —— 否则同一片源在探针和播放器里得到不同的欠载数，
/// 而"哪一份是对照基线"就说不清了。
///
/// 252：300 → 600，依据是**真机读画像**，不是感觉：
///   · SMB 窗口填充 `fill_avg ≈ 55 ms`、`fill_max ≈ 190–260 ms`（1 MiB 块，
///     这台 NAS 的 `max_read_size` 就是 1 MiB）；
///   · 音频与视频共用同一条读路径（同一个 `RangeFd` → 同一个窗口 →
///     同一条 libsmb2 连接），于是音频排在视频后面，一次填充就是一次停顿；
///   · 实测整条管线最长停 **320 ms**（`clock.tail.max_mono_step_ns`），
///     而 300 ms 的环（扣掉约 150 ms 的钟延迟只剩 ≈150 ms 余量）**必然被抽干**
///     → 补静音 → 听得见的杂音。
/// 600 ms 让余量回到 ≈450 ms > 实测最长停顿，代价是多约 300 ms 延迟
/// （A/V 同步不受影响：音频钟是主时钟，音画一起平移）。
/// mpv 的 `demuxer-cache-duration` 默认也是 1 秒量级，这个量级不算奢侈。
constexpr std::int32_t kRingCapacityMillis = 600;

/// 起流前的预灌水位，用 burst 的倍数表达：
/// 一个 burst 只能撑过一次回调；**要盖住一次网络停顿就得预灌到水位线以上**。
/// 252：3 → 15（约 300 ms @48kHz/960 帧每 burst）。原来只预灌 3 个 burst
/// （60 ms），意味着**起播瞬间环几乎是空的** —— 起播后第一次网络停顿就会抽干它。
constexpr std::int32_t kPrefillBursts = 15;

/// 等时钟首个有效时间戳的上限。C4 实测启动开销 24–146 ms，2 s 是宽容的兜底。
constexpr std::int64_t kClockReadyTimeoutNs = 2'000'000'000;

/// 停止时的排空上限，避免把"我们自己停的"记成欠载。
constexpr int kDrainTimeoutMillis = 300;

}  // namespace

AudioPlayback::~AudioPlayback() {
    stop();
}

bool AudioPlayback::open(int fd, std::int64_t length, const std::string& label,
                         int prefill_timeout_millis) {
    if (opened_) {
        stats_.error = "audio playback already open";
        return false;
    }
    if (prefill_timeout_millis <= 0 || prefill_timeout_millis > 30'000) {
        stats_.error = "audio prefill budget must be within 1..30000ms";
        return false;
    }
    stats_.prefill_budget_millis = prefill_timeout_millis;

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
    mediacodec::DecodeOutcome configured;
    // 平台没有解码器的格式一律交给 FFmpeg 软解。
    //
    // 这份名单是"本机 MediaCodec 确实没有"的实测清单，不是猜测：
    // AC3/EAC3 早就不在设备解码器里；TrueHD（audio/true-hd）与 DTS（audio/vnd.dts）
    // 是蓝光 REMUX 最常见的两条音轨，设备同样没有。
    // 名单外的格式仍然走平台：那是历史行为，动它没有收益。
    const bool software_audio =
        track.mime == "audio/ac3" || track.mime == "audio/eac3" ||
        track.mime == "audio/true-hd" || track.mime == "audio/vnd.dts";
    if (software_audio) {
#ifdef KIVO_ENABLE_FFMPEG_AUDIO
        auto software = std::make_unique<ffmpeg::FfmpegAudioDecoder>();
        configured = software->configure(track.mime, declared);
        decoder_ = std::move(software);
#else
        stats_.error = "software audio decoder unavailable for " + track.mime;
        return false;
#endif
    } else {
        auto platform = std::make_unique<mediacodec::MediaCodecAudioDecoder>();
        platform->set_declared_format(declared);
        configured = platform->configure(track.mime, source_.track_format());
        decoder_ = std::move(platform);
    }
    if (!configured.ok()) {
        stats_.error = "audio decoder configure: " + configured.message;
        return false;
    }
    const auto decoder_started = decoder_->start();
    if (!decoder_started.ok()) {
        stats_.error = "audio decoder start: " + decoder_started.message;
        return false;
    }
    stats_.codec_name = decoder_->codec_name();

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
    feeder_ = std::make_unique<mediacodec::AudioDecodeFeeder>(source_, *decoder_, *ring_);
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
    if (!prefill()) return false;

    const aaudio::AaudioOpenOutcome started = output_->start();
    if (!started.ok()) {
        stats_.error = "aaudio start: " + started.message;
        // The owner cancels network reads before joining this feeder.
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

bool AudioPlayback::prepare_seek() {
    if (!started_ || !feeder_ || !output_ || !ring_ || !ring_source_) {
        stats_.error = "audio playback is not running";
        return false;
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
        // 不带着在途读者 reset 环；失败保持静音，宿主统一取消 I/O 并停止会话。
        stats_.error = "seek gate timeout: audio callback still inside render() after 10 ms; "
                     "refusing to reset the SPSC ring";
        return false;
    }
    return true;
}

std::string AudioPlayback::demux_seek_summary() const {
    const auto d = source_.seek_diagnostics();
    char buffer[224] = {};
    std::snprintf(buffer, sizeof(buffer),
                  "backend=%s idx=%d rc=%d target=%lld landed=%lld avio=%lld",
                  d.backend[0] ? d.backend : "none", d.index_entries, d.seek_rc,
                  static_cast<long long>(d.target_us), static_cast<long long>(d.landed_us),
                  static_cast<long long>(d.avio_pos));
    return std::string(buffer);
}

AudioPlayback::SeekResult AudioPlayback::seek_to(std::int64_t target_us) {
    // 252：逐步计时（写进可随时读的槽位），卡住时也能看出卡在哪一步。
    constexpr int kStepPrepare = 0;
    constexpr int kStepFeederStop = 1;
    constexpr int kStepSourceSeek = 2;
    constexpr int kStepDecoderFlush = 3;
    constexpr int kStepRingReset = 4;
    constexpr int kStepFeederStart = 5;
    constexpr int kStepPrefill = 6;
    constexpr int kStepRebase = 7;
    const auto mark = [this](int step, std::int64_t elapsed_ms) {
        if (step >= 0 && step < kSeekStepCount) {
            seek_step_ms_[step] = elapsed_ms;
            seek_step_done_ = step + 1;
        }
    };
    const auto since = [] { return std::chrono::steady_clock::now(); };

    SeekResult out;
    auto t0 = since();
    if (!prepare_seek()) {
        mark(kStepPrepare, 0);
        out.reason = stats_.error;
        return out;
    }
    mark(kStepPrepare, std::chrono::duration_cast<std::chrono::milliseconds>(since() - t0).count());

    // 2. 停写者。此后没有任何人碰环。
    t0 = since();
    feeder_->stop();
    mark(kStepFeederStop, std::chrono::duration_cast<std::chrono::milliseconds>(since() - t0).count());

    // 3. 定位 + 清在途。
    t0 = since();
    const std::int64_t landing = source_.seek_to(target_us);
    mark(kStepSourceSeek, std::chrono::duration_cast<std::chrono::milliseconds>(since() - t0).count());
    out.landing_us = landing;
    if (landing < 0) {
        out.reason = stats_.error = "audio extractor seek failed";
        return out;
    }
    t0 = since();
    const auto flushed = decoder_->flush();
    mark(kStepDecoderFlush, std::chrono::duration_cast<std::chrono::milliseconds>(since() - t0).count());
    if (!flushed.ok()) {
        out.reason = stats_.error = "audio decoder flush failed: " + flushed.message;
        return out;
    }
    t0 = since();
    ring_->reset();
    mark(kStepRingReset, std::chrono::duration_cast<std::chrono::milliseconds>(since() - t0).count());

    // 4. 重新供给并灌到与起播同一水位。
    t0 = since();
    feeder_->start();
    mark(kStepFeederStart, std::chrono::duration_cast<std::chrono::milliseconds>(since() - t0).count());
    t0 = since();
    if (!prefill()) {
        mark(kStepPrefill, std::chrono::duration_cast<std::chrono::milliseconds>(since() - t0).count());
        out.reason = stats_.error;
        return out;
    }
    mark(kStepPrefill, std::chrono::duration_cast<std::chrono::milliseconds>(since() - t0).count());

    // 5. **先放音、后对表**：放音之后第一个真实样本才是 landing 对应的音频；
    //    先锚表会让时钟在静音期间继续前进，视频反而提前。
    ring_source_->set_muted(false);
    t0 = since();
    if (auto* clock = output_->clock(); clock != nullptr && landing >= 0) {
        // 流没重启，帧计数连续，一次 rebase 足够（F2 里"锚两次吸收过渡"的做法不再需要）。
        clock->rebase(landing * 1'000);
    }
    mark(kStepRebase, std::chrono::duration_cast<std::chrono::milliseconds>(since() - t0).count());

    out.ok = true;
    return out;
}

bool AudioPlayback::prefill() {
    const auto began = std::chrono::steady_clock::now();
    stats_.prefill_reached = feeder_->wait_filled(
        stats_.frames_per_burst * kPrefillBursts, stats_.prefill_budget_millis);
    stats_.prefill_last_wait_millis = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - began).count();
    if (stats_.prefill_initial_wait_millis < 0)
        stats_.prefill_initial_wait_millis = stats_.prefill_last_wait_millis;
    stats_.prefill_max_wait_millis = std::max(stats_.prefill_max_wait_millis,
                                            stats_.prefill_last_wait_millis);
    if (feeder_->finished()) {
        const auto feed = feeder_->stats();
        if (!feed.error.empty()) {
            stats_.error = feed.error;
            return false;
        }
        if (feed.reached_eos && ring_->available_frames() > 0) return true;
    }
    if (stats_.prefill_reached) return true;
    // Do not join a potentially blocked network reader here. The owner cancels I/O
    // before stopping this object. No stream starts with an empty PCM ring.
    stats_.error = "audio PCM prefill not ready within " +
                   std::to_string(stats_.prefill_budget_millis) + "ms";
    return false;
}

bool AudioPlayback::pause() {
    // Keep pending packets and partial PCM alive. Once consumption pauses, the bounded
    // ring applies backpressure; stopping the feeder here would discard its partial chunk.
    if (output_ && output_->pause()) return true;
    if (ring_source_) ring_source_->set_muted(true);
    stats_.error = "audio output pause failed";
    return false;
}

bool AudioPlayback::resume() {
    if (output_ && output_->resume()) return true;
    if (ring_source_) ring_source_->set_muted(true);
    stats_.error = "audio output resume failed";
    return false;
}

std::string AudioPlayback::runtime_error() const {
    if (!stats_.error.empty()) return stats_.error;
    // finished acquire publishes the worker's final stats; never read live counters.
    return feeder_ && feeder_->finished() ? feeder_->stats().error : std::string{};
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
        stats_.output_format = decoder_->output_format();
        stats_.output_format_diverged = decoder_->output_format_diverged();
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
