#include "video/video_plane/backends/mediacodec/session/audio_decode_feeder.hpp"

#include <chrono>
#include <algorithm>
#include <cmath>

#include "video/video_plane/core/audio/pcm_format_converter.hpp"
#include "video/video_plane/core/audio/pcm_channel_downmix.hpp"

namespace kivo::video::mediacodec {
namespace {

/// 没送进包、也没收到帧时的等待上限。值取得很小（2ms）：
/// 音频只需要每 20ms 左右产出一块，等待本身不构成延迟；
/// 但如果设成 0 就会变成忙等，白白烧掉一个核。
constexpr std::int64_t kIdleReceiveTimeoutUs = 2000;

/// 环满时的退让粒度。1ms 足以让消费端取走一个 burst（K40 上约 4ms 一个）。
constexpr auto kRingFullSleep = std::chrono::milliseconds(1);

/// 输入已结束、却始终收不到平台 EOS 标志时的兜底轮数。
/// 一些解码器只吐完数据就沉默，不发 EOS；没有兜底就会永久等下去。
constexpr int kMaxIdleRoundsWithoutEos = 500;

}  // namespace

AudioDecodeFeeder::AudioDecodeFeeder(MediaExtractorSource& source,
                                     core::audio::AudioDecoder& decoder,
                                     core::audio::SpscAudioRingBuffer& ring)
    : source_(source), decoder_(decoder), ring_(ring) {}

AudioDecodeFeeder::~AudioDecodeFeeder() {
    stop();
}

void AudioDecodeFeeder::start() {
    if (running_.exchange(true)) {
        return;
    }
    stop_requested_.store(false);
    // A new decode run follows seek/flush. Keep cumulative counts, never carry
    // terminal flags into the new segment (otherwise it stops after one packet).
    stats_.reached_eos = false;
    stats_.eos_flag_seen = false;
    stats_.error.clear();
    finished_.store(false);
    worker_ = std::thread(&AudioDecodeFeeder::loop, this);
}

void AudioDecodeFeeder::stop() {
    if (!running_.load()) {
        return;
    }
    stop_requested_.store(true);
    if (worker_.joinable()) {
        worker_.join();
    }
    running_.store(false);
}

bool AudioDecodeFeeder::wait_filled(std::int32_t min_frames, int timeout_ms) const {
    const int rounds = timeout_ms;
    for (int i = 0; i < rounds; ++i) {
        if (ring_.available_frames() >= min_frames) {
            return true;
        }
        if (finished_.load(std::memory_order_acquire)) {
            // 解码已结束：短音轨可能永远到不了目标水位，此时"没灌满"不代表失败。
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return ring_.available_frames() >= min_frames;
}

void AudioDecodeFeeder::loop() {
    bool input_done = false;
    bool has_pending = false;
    mediacodec::SampleView pending{};
    std::int64_t last_pts = -1;
    int idle_rounds = 0;

    while (!stop_requested_.load(std::memory_order_relaxed)) {
        bool progress = false;

        // 1) 取下一包。已有一包在途时绝不覆盖它——
        //    SampleView 指向解封装器内部缓冲，再读一次就把数据覆盖了。
        if (!has_pending && !input_done) {
            mediacodec::SampleView view;
            const auto read_status = source_.read_next_sample(view);
            if (read_status == mediacodec::SampleReadStatus::kEndOfStream) {
                const auto eos = decoder_.signal_end_of_stream(last_pts);
                if (eos.ok()) {
                    input_done = true;
                    progress = true;
                } else if (!eos.try_again()) {
                    stats_.error = eos.message;
                    break;
                }
            } else if (read_status == mediacodec::SampleReadStatus::kOk) {
                pending = view;
                has_pending = true;
            } else {
                stats_.error = "read_next_sample returned failure";
                break;
            }
        }

        // 2) 送包。TRY_AGAIN 就留到下一轮，绝不丢包——丢一包就是一段可听的缺口。
        if (has_pending) {
            const auto submitted = decoder_.submit(pending, 0);
            if (submitted.ok()) {
                ++stats_.packets_submitted;
                last_pts = pending.presentation_time_us;
                has_pending = false;
                progress = true;
            } else if (!submitted.try_again()) {
                stats_.error = submitted.message;
                break;
            }
        }

        // 3) 收帧。有进展就不等，没进展才等一个短超时。
        bool got_data = false;
        drain_once(progress ? 0 : kIdleReceiveTimeoutUs, got_data);
        if (!stats_.error.empty()) {
            break;
        }

        if (got_data) {
            progress = true;
            idle_rounds = 0;
        } else if (!progress) {
            ++idle_rounds;
        }

        if (stats_.eos_flag_seen) {
            stats_.reached_eos = true;
            break;
        }

        // 4) 输入结束且持续空转：判定解码器不会再吐东西（兜底，见常量注释）。
        if (input_done && !progress && idle_rounds > kMaxIdleRoundsWithoutEos) {
            stats_.reached_eos = true;
            break;
        }
    }

    finished_.store(true, std::memory_order_release);
}

void AudioDecodeFeeder::drain_once(std::int64_t timeout_us, bool& got_data) {
    got_data = false;

    core::audio::PcmChunk chunk;
    const auto received = decoder_.receive(chunk, timeout_us);

    if (received.end_of_stream()) {
        stats_.eos_flag_seen = true;
    } else if (!received.ok() && !received.try_again()) {
        stats_.error = received.message;
        return;
    }

    if (chunk.size == 0 || chunk.frame_count <= 0) {
        return;
    }

    const auto& output = decoder_.output_format();
    if (!output.is_valid()) {
        stats_.error = "decoder reported a valid chunk but no usable output format";
        return;
    }

    // 解码器输出格式与容器声明不一致时立即失败。
    // 采样率不同 → 音高与时长全错；声道数不同 → 交错步长错位，出来的是噪声。
    // 两者都不能"先播着看看"。
    if (decoder_.output_format_diverged()) {
        stats_.error = "decoder output format diverged from container declaration (rate=" +
                       std::to_string(output.sample_rate) + " channels=" +
                       std::to_string(output.channel_count) + ")";
        return;
    }

    // 环形缓冲的声道数是构造期定死的，两侧必须一致，否则写入即错位。
    //
    // 唯一的例外是 5.1 → 立体声（显式开启时）：解码器出 6ch，环收 2ch，
    // 中间由本文件做一次标准下混。理由见头文件 DownmixPolicy 的注释：
    // 真高清素材的人声几乎全在 FC，交给平台做 6→2 会把人声压掉。
    const bool downmix_requested =
        downmix_policy_ == DownmixPolicy::kAllow &&
        output.channel_count == core::audio::kSourceChannels5Point1 &&
        ring_.channel_count() == core::audio::kStereoChannels;
    if (!downmix_requested && output.channel_count != ring_.channel_count()) {
        stats_.error = "channel count mismatch: decoder=" + std::to_string(output.channel_count) +
                       " ring=" + std::to_string(ring_.channel_count());
        return;
    }

    ++stats_.chunks_received;
    stats_.frames_decoded += chunk.frame_count;

    const auto needed = static_cast<std::size_t>(chunk.frame_count) *
                        static_cast<std::size_t>(output.channel_count);
    if (converted_.size() < needed) {
        converted_.resize(needed);
    }

    const auto converted = core::audio::convert_to_float(output, chunk.data, chunk.size,
                                                         converted_.data(), chunk.frame_count);
    if (!converted.ok()) {
        stats_.error = "pcm conversion failed: " + converted.message;
        return;
    }

    for (std::size_t i = 0; i < static_cast<std::size_t>(converted.frames_written) * output.channel_count; ++i) {
        if (!std::isfinite(converted_[i])) {
            stats_.error = "decoder produced non-finite PCM";
            return;
        }
        stats_.decoded_peak = std::max(stats_.decoded_peak, std::abs(converted_[i]));
    }

    if (downmix_requested) {
        // 5.1 → 立体声。矩阵见 core/audio/pcm_channel_downmix.hpp：
        // FC 各 0.7071 进左右（LFE 丢弃），保住承载人声的中置声道。
        const std::size_t needed =
            static_cast<std::size_t>(converted.frames_written) * core::audio::kStereoChannels;
        if (downmixed_.size() < needed) {
            downmixed_.resize(needed);
        }
        const std::int32_t written = core::audio::downmix_5point1_to_stereo(
            converted_.data(), converted.frames_written, downmixed_.data(),
            static_cast<std::int32_t>(downmixed_.size() / core::audio::kStereoChannels));
        if (written != converted.frames_written) {
            stats_.error = "5.1 to stereo downmix wrote " + std::to_string(written) +
                           " frames but expected " + std::to_string(converted.frames_written);
            return;
        }
        for (std::size_t i = 0; i < static_cast<std::size_t>(written) * core::audio::kStereoChannels; ++i) {
            if (!std::isfinite(downmixed_[i])) {
                stats_.error = "5.1 to stereo downmix produced non-finite PCM";
                return;
            }
            stats_.downmixed_peak = std::max(stats_.downmixed_peak, std::abs(downmixed_[i]));
        }
        stats_.downmixed_frames += written;
        push_frames(downmixed_.data(), written);
        got_data = true;
        return;
    }

    push_frames(converted_.data(), converted.frames_written);
    got_data = true;
}

void AudioDecodeFeeder::push_frames(const float* interleaved, std::int32_t frame_count) {
    if (frame_count <= 0) {
        return;
    }

    const std::int32_t channels = ring_.channel_count();
    const float* cursor = interleaved;
    std::int32_t remaining = frame_count;

    while (remaining > 0 && !stop_requested_.load(std::memory_order_relaxed)) {
        const std::int32_t written = ring_.write(cursor, remaining);
        if (written <= 0) {
            // 环满 = 消费端还没取走。这是背压，不是错误；等一会儿再来。
            ++stats_.ring_full_waits;
            std::this_thread::sleep_for(kRingFullSleep);
            continue;
        }
        cursor += static_cast<std::size_t>(written) * static_cast<std::size_t>(channels);
        remaining -= written;
        stats_.frames_written += written;
    }

    if (remaining > 0) {
        // 只可能因为停止请求而丢弃。必须计数，否则就是偷偷丢音频。
        stats_.frames_dropped += remaining;
    }
}

}  // namespace kivo::video::mediacodec
