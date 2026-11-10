// 软解音频通路：fd → FFmpeg demux → FFmpeg decode → swresample → 环形缓冲 → AAudio。
//
// 这个文件是本项目里**唯一**允许同时出现 FFmpeg 类型与 AAudio 类型的实现：
// 前者的产物（PCM）正是后者的输入，把它们拆到两个 TU 只是把一次直接的
// 数据传递换成一次接口跳转，换不到任何隔离收益。头文件仍然完全干净。

#include "video/video_plane/backends/avsync/session/soft_audio_session.hpp"

#include <chrono>
#include <cstring>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <unistd.h>

#include "video/video_plane/backends/aaudio/buffer/ring_buffer_sample_source.hpp"
#include "video/video_plane/backends/aaudio/session/aaudio_output_session.hpp"
#include "video/video_plane/core/audio/spsc_audio_ring_buffer.hpp"

namespace kivo::video::avsync {
namespace {

namespace aaudio = kivo::video::aaudio;
namespace core_audio = kivo::video::core::audio;

constexpr int kOutSampleRate = 48000;
constexpr int kOutChannels = 2;
/// 环容量 500 ms：够吸收 SMB 抖动，又不会让暂停/定位的响应迟钝。
constexpr int kRingCapacityFrames = kOutSampleRate / 2;
/// 起流前先灌 200 ms，避免开头就是一串欠载静音。
constexpr int kPrefillFrames = kOutSampleRate / 5;

std::int64_t steady_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

struct FdSource {
    int fd = -1;
    std::int64_t offset = 0;
    std::int64_t length = -1;
    std::int64_t cursor = 0;
};

int read_cb(void* opaque, std::uint8_t* buffer, int size) {
    auto* source = static_cast<FdSource*>(opaque);
    if (source == nullptr || source->fd < 0 || buffer == nullptr || size <= 0) {
        return AVERROR(EIO);
    }
    if (source->length >= 0 && source->cursor >= source->length) {
        return AVERROR_EOF;
    }
    int want = size;
    if (source->length >= 0) {
        const std::int64_t remain = source->length - source->cursor;
        if (remain < want) {
            want = static_cast<int>(remain);
        }
    }
    const ssize_t got = pread(source->fd, buffer, static_cast<size_t>(want),
                              static_cast<off_t>(source->offset + source->cursor));
    if (got <= 0) {
        return got < 0 ? AVERROR(EIO) : AVERROR_EOF;
    }
    source->cursor += got;
    return static_cast<int>(got);
}

std::int64_t seek_cb(void* opaque, std::int64_t offset, int whence) {
    auto* source = static_cast<FdSource*>(opaque);
    if (source == nullptr || source->fd < 0) {
        return AVERROR(EIO);
    }
    if ((whence & ~AVSEEK_FORCE) == AVSEEK_SIZE) {
        return source->length;
    }
    switch (whence & ~AVSEEK_FORCE) {
        case SEEK_SET:
            source->cursor = offset;
            break;
        case SEEK_CUR:
            source->cursor += offset;
            break;
        case SEEK_END:
            if (source->length < 0) {
                return AVERROR(EIO);
            }
            source->cursor = source->length + offset;
            break;
        default:
            return AVERROR(EIO);
    }
    if (source->cursor < 0) {
        source->cursor = 0;
    }
    return source->cursor;
}

}  // namespace

SoftAudioSession::SoftAudioSession() = default;

SoftAudioSession::~SoftAudioSession() { stop(); }

bool SoftAudioSession::open(int fd, std::int64_t length, const std::string& label) {
    if (opened_) {
        stats_.error = "audio session already open";
        return false;
    }
    if (fd < 0) {
        stats_.error = "audio fd is invalid";
        return false;
    }
    fd_ = fd;
    length_ = length;
    label_ = label;
    opened_ = true;
    return true;
}

bool SoftAudioSession::start() {
    if (!opened_) {
        stats_.error = "open() first";
        return false;
    }
    if (running_.load(std::memory_order_relaxed)) {
        stats_.error = "audio session already running";
        return false;
    }
    running_.store(true, std::memory_order_relaxed);
    worker_ = std::thread([this] { run_loop_(); });
    return true;
}

void SoftAudioSession::setPaused(bool paused) {
    paused_.store(paused, std::memory_order_relaxed);
    pause_cv_.notify_all();
}

void SoftAudioSession::seekTo(std::int64_t target_us) {
    seek_request_us_.store(target_us, std::memory_order_relaxed);
    pause_cv_.notify_all();
}

void SoftAudioSession::stop() {
    if (worker_.joinable()) {
        running_.store(false, std::memory_order_relaxed);
        paused_.store(false, std::memory_order_relaxed);
        pause_cv_.notify_all();
        worker_.join();
    }
    opened_ = false;
}

void SoftAudioSession::run_loop_() {
    const std::int64_t started_us = steady_us();
    FdSource source;
    source.fd = fd_;
    source.length = length_;

    const int buffer_size = 64 * 1024;
    auto* avio_buffer = static_cast<std::uint8_t*>(av_malloc(buffer_size));
    if (avio_buffer == nullptr) {
        stats_.error = "avio buffer alloc failed";
        running_.store(false, std::memory_order_relaxed);
        return;
    }
    AVIOContext* avio = avio_alloc_context(avio_buffer, buffer_size, 0, &source, read_cb,
                                           nullptr, seek_cb);
    if (avio == nullptr) {
        av_free(avio_buffer);
        stats_.error = "avio_alloc_context failed";
        running_.store(false, std::memory_order_relaxed);
        return;
    }
    // 与视频软解同一处理：不标可 seek 的话 mkv 读不到 Cues，时间戳不可靠。
    avio->seekable = AVIO_SEEKABLE_NORMAL;

    AVFormatContext* format = avformat_alloc_context();
    if (format == nullptr) {
        av_freep(&avio->buffer);
        avio_context_free(&avio);
        stats_.error = "avformat_alloc_context failed";
        running_.store(false, std::memory_order_relaxed);
        return;
    }
    format->pb = avio;
    format->flags |= AVFMT_FLAG_CUSTOM_IO;

    // 探针预算必须收紧，否则音画起点天然错开。
    //
    // FFmpeg 默认 probesize 是 5 MB、max_analyze_duration 是 5 秒。对这部 20 GB 的
    // REMUX，光是"认出这是 TrueHD 5.1"就要从 SMB 上读走 5 MB —— 实测 1.4 秒。
    // 音频于是比视频晚 1.4 s 才出声，两者又都从 pts=0 起跑，这个差值就永久留在那里，
    // 表现就是"口型一直对不上"。收窄到 1 MiB / 3 秒，探测时间降到百毫秒级。
    //
    // 1 MiB 的依据：TrueHD 码率约 4 Mbps，1 MiB ≈ 2 秒音频，足够认出编码与声道布局；
    // 3 秒的 max_analyze_duration 是给可变帧率/交错的容器留余量。
    format->probesize = 1024 * 1024;
    format->max_analyze_duration = 3 * AV_TIME_BASE;

    if (avformat_open_input(&format, nullptr, nullptr, nullptr) < 0 ||
        avformat_find_stream_info(format, nullptr) < 0) {
        stats_.error = "audio demux open failed";
        if (format != nullptr) {
            format->pb = nullptr;
            avformat_free_context(format);
        }
        av_freep(&avio->buffer);
        avio_context_free(&avio);
        running_.store(false, std::memory_order_relaxed);
        return;
    }
    if (format->duration != AV_NOPTS_VALUE && format->duration > 0) {
        duration_us_.store(format->duration, std::memory_order_relaxed);
    }
    if (format->iformat != nullptr && format->iformat->name != nullptr) {
        stats_.container = format->iformat->name;
    }

    int audio_stream = -1;
    for (unsigned i = 0; i < format->nb_streams; ++i) {
        if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream = static_cast<int>(i);
            break;
        }
    }
    if (audio_stream < 0) {
        stats_.error = "no audio track";
        avformat_close_input(&format);
        av_freep(&avio->buffer);
        avio_context_free(&avio);
        running_.store(false, std::memory_order_relaxed);
        return;
    }

    AVCodecParameters* params = format->streams[audio_stream]->codecpar;
    // 只留音频这一路：视频包在音轨线程里毫无用处，不标丢弃就只能一个个
    // 取出来再扔掉，白花掉读包与解封装的 CPU（1080p REMUX 里视频包占九成）。
    for (unsigned i = 0; i < format->nb_streams; ++i) {
        format->streams[i]->discard =
            (static_cast<int>(i) == audio_stream) ? AVDISCARD_DEFAULT : AVDISCARD_ALL;
    }
    const AVCodec* codec = avcodec_find_decoder(params->codec_id);
    if (codec == nullptr) {
        stats_.error = "no decoder for codec id " + std::to_string(params->codec_id);
        avformat_close_input(&format);
        av_freep(&avio->buffer);
        avio_context_free(&avio);
        running_.store(false, std::memory_order_relaxed);
        return;
    }
    stats_.codec = codec->name != nullptr ? codec->name : "?";
    stats_.source_sample_rate = params->sample_rate;
    stats_.source_channels = params->ch_layout.nb_channels;

    AVCodecContext* decoder = avcodec_alloc_context3(codec);
    if (decoder == nullptr || avcodec_parameters_to_context(decoder, params) < 0 ||
        avcodec_open2(decoder, codec, nullptr) < 0) {
        stats_.error = "audio decoder open failed";
        if (decoder != nullptr) {
            avcodec_free_context(&decoder);
        }
        avformat_close_input(&format);
        av_freep(&avio->buffer);
        avio_context_free(&avio);
        running_.store(false, std::memory_order_relaxed);
        return;
    }

    // ---- 输出侧：环 + AAudio。走定长 48 kHz/stereo/float，重采样交给 swr。 ----
    core_audio::SpscAudioRingBuffer ring(kOutChannels, kRingCapacityFrames);
    aaudio::RingBufferSampleSource ring_source(ring);
    aaudio::AaudioOutputSession output;
    aaudio::AaudioStreamConfig out_config;
    out_config.sample_rate = kOutSampleRate;
    out_config.channel_count = kOutChannels;
    out_config.format = aaudio::AaudioStreamConfig::SampleFormat::kFloat;
    out_config.prefer_exclusive = true;
    out_config.prefer_low_latency = true;
    const aaudio::AaudioOpenOutcome opened = output.open(out_config, ring_source);
    if (!opened.ok()) {
        stats_.error = "aaudio open: " + opened.message;
        avcodec_free_context(&decoder);
        avformat_close_input(&format);
        av_freep(&avio->buffer);
        avio_context_free(&avio);
        running_.store(false, std::memory_order_relaxed);
        return;
    }
    stats_.output_sample_rate = opened.sample_rate;
    stats_.output_channels = opened.channel_count;
    output_open_.store(true, std::memory_order_relaxed);

    // 预灌之后再起流：先起流会立刻开始取数，前几十毫秒全是静音欠载。
    SwrContext* swr = nullptr;
    std::vector<float> converted;
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();
    bool stream_started = false;
    bool completed = false;

    // swr 的输入一旦变了（真高清音轨会在段落间换声道布局，AC3/DTS 的
    // 核心流尤其常见），旧的转换器就再也不成立，硬用会读到越界。所以记住
    // 上一帧的输入规格，一变就重建。
    int swr_src_fmt = -1;
    int swr_src_rate = 0;
    int swr_src_channels = 0;

    auto ensure_swr = [&](const AVFrame* f) -> bool {
        const int channels = f->ch_layout.nb_channels;
        if (swr != nullptr && swr_src_fmt == f->format && swr_src_rate == f->sample_rate &&
            swr_src_channels == channels) {
            return true;
        }
        if (swr != nullptr) {
            swr_free(&swr);
        }
        AVChannelLayout out_layout;
        av_channel_layout_default(&out_layout, kOutChannels);
        const int rc = swr_alloc_set_opts2(&swr, &out_layout, AV_SAMPLE_FMT_FLT, kOutSampleRate,
                                          &f->ch_layout, static_cast<AVSampleFormat>(f->format),
                                          f->sample_rate, 0, nullptr);
        av_channel_layout_uninit(&out_layout);
        if (rc < 0 || swr == nullptr || swr_init(swr) < 0) {
            stats_.error = "swr init failed";
            return false;
        }
        swr_src_fmt = f->format;
        swr_src_rate = f->sample_rate;
        swr_src_channels = channels;
        return true;
    };

    // 起流只有一个条件：环里攒够预灌量。所有地方都走这一处，避免出现
    // "这里起了那里没起"的分叉——AAudio 的 start() 调两次不会报错但会让
    // 缓冲被抽走两倍的量，听起来像开头吞了一口气。
    auto start_stream_if_ready = [&]() {
        if (stream_started) {
            return;
        }
        if (ring.available_frames() >= kPrefillFrames && output.start().ok()) {
            stream_started = true;
        }
    };

    auto drain_frame = [&]() -> bool {
        if (!ensure_swr(frame)) {
            return false;
        }
        if (frame->sample_rate <= 0 || frame->nb_samples <= 0) {
            return true;
        }
        const int max_out = static_cast<int>(av_rescale_rnd(
            swr_get_delay(swr, frame->sample_rate) + frame->nb_samples,
            kOutSampleRate, frame->sample_rate, AV_ROUND_UP));
        if (max_out <= 0) {
            return true;
        }
        converted.resize(static_cast<size_t>(max_out) * kOutChannels);
        // swr_convert 的 out 参数是**平面指针的数组**，不是 PCM 缓冲区本身。
        // 曾经这里写成 `(uint8_t**)converted.data()`，等于让 swr 把 PCM 的头
        // 几个字节当成地址去写 —— 真机崩溃点就是 `swr_convert → swri_audio_convert`
        // 的 fault addr 0x4。输出是 packed FLT，所以只有一个平面。
        std::uint8_t* out_planes[1] = {
            reinterpret_cast<std::uint8_t*>(converted.data())};
        const int produced = swr_convert(swr, out_planes, max_out,
                                         const_cast<const std::uint8_t**>(frame->extended_data),
                                         frame->nb_samples);
        if (produced <= 0) {
            return true;
        }
        ++stats_.frames_decoded;
        // 环满不是错误：短写后等消费者取走。这里 sleep 而不是忙等，
        // 因为 AAudio 的取数节奏由硬件决定，忙等只会白烧一个核。
        int offset = 0;
        while (running_.load(std::memory_order_relaxed) && offset < produced) {
            const int written =
                ring.write(converted.data() + static_cast<size_t>(offset) * kOutChannels,
                           produced - offset);
            offset += written;
            stats_.frames_written += written;
            if (offset >= produced) {
                break;
            }
            ++stats_.ring_full_waits;
            start_stream_if_ready();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        start_stream_if_ready();
        if (frame->pts != AV_NOPTS_VALUE) {
            const std::int64_t written_pts =
                av_rescale_q(frame->pts, format->streams[audio_stream]->time_base,
                             AVRational{1, 1000000});
            position_us_.store(written_pts, std::memory_order_relaxed);
            // 实际出声位置 = 已写入位置 − 环里还没被取走的时长。
            // `available_frames()` 是"已写但 AAudio 还没搬走"的帧数；这部分还没响，
            // 必须扣掉。不扣就会把"排队的"当成"正在响的"，音画对齐会系统性偏早
            // 最多 500 ms（环容量），表现为画面永远比声音慢半拍、怎么调都对不上。
            const std::int64_t queued_us =
                static_cast<std::int64_t>(ring.available_frames()) * 1000000 / kOutSampleRate;
            played_us_.store(written_pts - queued_us, std::memory_order_relaxed);
        }
        return true;
    };

    while (running_.load(std::memory_order_relaxed)) {
        if (const auto req = seek_request_us_.exchange(-1, std::memory_order_relaxed);
            req >= 0) {
            // 定位必须先停消费者再清环：环的 reset() 契约要求两侧都停，
            // 而且不清环的话，定位后头 500 ms 放的还是定位前那一段声音
            // —— 这正是"拖了进度条声音还是旧的"那种感觉的来源。
            if (stream_started) {
                output.stop();
                stream_started = false;
            }
            ring.reset();
            const std::int64_t ts =
                av_rescale_q(req, AVRational{1, 1000000},
                             format->streams[audio_stream]->time_base);
            if (av_seek_frame(format, audio_stream, ts, AVSEEK_FLAG_BACKWARD) >= 0) {
                avcodec_flush_buffers(decoder);
                if (swr != nullptr) {
                    swr_convert(swr, nullptr, 0, nullptr, 0);
                }
                position_us_.store(req, std::memory_order_relaxed);
                // 环已经 reset()，排队时长为 0：跳转后"正在响的"就是目标位置。
                played_us_.store(req, std::memory_order_relaxed);
            }
        }
        if (paused_.load(std::memory_order_relaxed)) {
            // 只置标志的话，环里那 500 ms 还会继续响 —— 用户按下暂停后
            // 声音仍在走，这就是"暂停没生效"。停流才是真正停声。
            if (stream_started) {
                output.stop();
                stream_started = false;
            }
            std::unique_lock<std::mutex> lock(pause_mutex_);
            pause_cv_.wait(lock, [this] {
                return !paused_.load(std::memory_order_relaxed) ||
                       !running_.load(std::memory_order_relaxed) ||
                       seek_request_us_.load(std::memory_order_relaxed) >= 0;
            });
            if (!running_.load(std::memory_order_relaxed)) {
                break;
            }
            continue;
        }

        const auto decode_started = std::chrono::steady_clock::now();
        const int read_rc = av_read_frame(format, packet);
        if (read_rc < 0) {
            if (read_rc == AVERROR_EOF) {
                completed = true;
                break;
            }
            ++stats_.read_errors;
            if (stats_.read_errors > 200) {
                stats_.error = "audio read failed repeatedly";
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        ++stats_.packets_read;
        if (packet->stream_index != audio_stream) {
            av_packet_unref(packet);
            continue;
        }
        ++stats_.packets_sent;
        if (avcodec_send_packet(decoder, packet) >= 0) {
            while (avcodec_receive_frame(decoder, frame) >= 0) {
                if (!drain_frame()) {
                    av_packet_unref(packet);
                    running_.store(false, std::memory_order_relaxed);
                    break;
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
        stats_.decode_ms_total +=
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() -
                                                      decode_started)
                .count();
    }

    if (completed) {
        stats_.reached_eos = true;
    }
    output.stop();
    output_open_.store(false, std::memory_order_relaxed);
    av_packet_free(&packet);
    av_frame_free(&frame);
    if (swr != nullptr) {
        swr_free(&swr);
    }
    avcodec_free_context(&decoder);
    avformat_close_input(&format);
    av_freep(&avio->buffer);
    avio_context_free(&avio);
    stats_.wall_us = steady_us() - started_us;
    running_.store(false, std::memory_order_relaxed);
}

}  // namespace kivo::video::avsync
