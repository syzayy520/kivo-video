#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"

#include "ffmpeg_product_log_filter.hpp"

#ifdef KIVO_ENABLE_FFMPEG
extern "C" {
#include <android/log.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
#include <libswscale/swscale.h>
}
#endif

#include <atomic>
#include <chrono>
#include <thread>

namespace kivo::cinema_engine {

// 24x 诊断：把 receive_frame_rgba 拆成"avcodec 解码"与"swscale 转 RGBA"两段。
// 全局累加（一次播放只有一个软解会话；会话在 start 时 reset），跨线程用 relaxed
// 原子即可——只是用来读数量级，不参与任何控制流。
namespace {
std::atomic<std::int64_t> g_avcodec_ns{0};
std::atomic<std::int64_t> g_sws_ns{0};
std::atomic<std::int64_t> g_frame_count{0};
std::atomic<std::int64_t> g_send_ns{0};
std::atomic<std::int64_t> g_send_calls{0};
std::atomic<std::int64_t> g_resize_ns{0};

// 帧缓冲池（24x）。只留一块：软解是单线程解码，同一时刻只需要一块在飞。
// 用 thread_local 是因为 receive_frame_rgba 与 recycle_frame_buffer 都跑在解码线程上，
// 不留全局锁就不会在别的解码器上互相干扰。
thread_local std::vector<std::uint8_t> g_frame_buffer_pool;
}  // namespace

// Availability

bool FfmpegAdapter::is_available() {
#ifdef KIVO_ENABLE_FFMPEG
    return true;
#else
    return false;
#endif
}

// Format context lifecycle

FfmpegFormatHandle FfmpegAdapter::open_file(const std::string& path) {
#ifdef KIVO_ENABLE_FFMPEG
    ffmpeg_adapter::install_product_log_filter();
    AVFormatContext* ctx = nullptr;
    int ret = avformat_open_input(&ctx, path.c_str(), nullptr, nullptr);
    if (ret < 0) return {nullptr};
    ret = avformat_find_stream_info(ctx, nullptr);
    if (ret < 0) { avformat_close_input(&ctx); return {nullptr}; }
    return {ctx};
#else
    return {nullptr};
#endif
}

FfmpegFormatHandle FfmpegAdapter::open_url(const std::string& url) {
#ifdef KIVO_ENABLE_FFMPEG
    ffmpeg_adapter::install_product_log_filter();
    AVFormatContext* ctx = nullptr;
    int ret = avformat_open_input(&ctx, url.c_str(), nullptr, nullptr);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        __android_log_print(ANDROID_LOG_ERROR, "KivoFfmpeg",
                            "open_url failed: %s (%s)", url.c_str(), errbuf);
        return {nullptr};
    }
    ret = avformat_find_stream_info(ctx, nullptr);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        __android_log_print(ANDROID_LOG_ERROR, "KivoFfmpeg",
                            "find_stream_info failed: %s (%s)", url.c_str(), errbuf);
        avformat_close_input(&ctx);
        return {nullptr};
    }
    return {ctx};
#else
    return {nullptr};
#endif
}

void FfmpegAdapter::free_format_context(FfmpegFormatHandle& handle) {
#ifdef KIVO_ENABLE_FFMPEG
    if (handle.native) {
        auto* ctx = static_cast<AVFormatContext*>(handle.native);
        avformat_close_input(&ctx);
        handle.native = nullptr;
    }
    if (handle.custom_io_destroy != nullptr && handle.custom_io_context != nullptr) {
        handle.custom_io_destroy(handle.custom_io_context);
    }
    handle.custom_io_context = nullptr;
    handle.custom_io_destroy = nullptr;
#else
    handle.native = nullptr;
    handle.custom_io_context = nullptr;
    handle.custom_io_destroy = nullptr;
#endif
}

// Format context queries

bool FfmpegAdapter::probe_container(FfmpegFormatHandle handle,
                                    std::string& container_format,
                                    double& duration,
                                    int64_t& bitrate_estimate) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!handle.native) return false;
    auto* ctx = static_cast<AVFormatContext*>(handle.native);
    container_format = ctx->iformat->name;
    duration = (ctx->duration != AV_NOPTS_VALUE)
               ? static_cast<double>(ctx->duration) / AV_TIME_BASE : 0.0;
    bitrate_estimate = ctx->bit_rate;
    return true;
#else
    return false;
#endif
}

bool FfmpegAdapter::enumerate_streams(FfmpegFormatHandle handle,
                                      std::vector<int>& video_indices,
                                      std::vector<int>& audio_indices,
                                      std::vector<int>& subtitle_indices) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!handle.native) return false;
    auto* ctx = static_cast<AVFormatContext*>(handle.native);
    video_indices.clear();
    audio_indices.clear();
    subtitle_indices.clear();
    for (unsigned i = 0; i < ctx->nb_streams; i++) {
        AVMediaType type = ctx->streams[i]->codecpar->codec_type;
        switch (type) {
            case AVMEDIA_TYPE_VIDEO:    video_indices.push_back(i); break;
            case AVMEDIA_TYPE_AUDIO:    audio_indices.push_back(i); break;
            case AVMEDIA_TYPE_SUBTITLE: subtitle_indices.push_back(i); break;
            default: break;
        }
    }
    return true;
#else
    return false;
#endif
}

void FfmpegAdapter::discard_streams_except(FfmpegFormatHandle handle, int keep_stream) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!handle.native) return;
    auto* ctx = static_cast<AVFormatContext*>(handle.native);
    for (unsigned i = 0; i < ctx->nb_streams; i++) {
        // 保留的那一路显式设成 DEFAULT：调用方可能在 seek/切轨之后再调一次，
        // 不能假设它此前没被标成丢弃。
        ctx->streams[i]->discard =
            (static_cast<int>(i) == keep_stream) ? AVDISCARD_DEFAULT : AVDISCARD_ALL;
    }
#else
    (void)handle;
    (void)keep_stream;
#endif
}

// Stream info extraction

bool FfmpegAdapter::get_stream_codec_name(FfmpegFormatHandle handle, int stream_index,
                                          std::string& codec_name) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!handle.native) return false;
    auto* ctx = static_cast<AVFormatContext*>(handle.native);
    if (stream_index < 0 || static_cast<unsigned>(stream_index) >= ctx->nb_streams) return false;
    auto* desc = avcodec_descriptor_get(ctx->streams[stream_index]->codecpar->codec_id);
    codec_name = desc ? desc->name : "unknown";
    return true;
#else
    return false;
#endif
}

bool FfmpegAdapter::get_stream_video_info(FfmpegFormatHandle handle, int stream_index,
                                          int& width, int& height, double& fps,
                                          std::string& pixel_format) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!handle.native) return false;
    auto* ctx = static_cast<AVFormatContext*>(handle.native);
    if (stream_index < 0 || static_cast<unsigned>(stream_index) >= ctx->nb_streams) return false;
    auto* stream = ctx->streams[stream_index];
    auto* codecpar = stream->codecpar;
    if (codecpar->codec_type != AVMEDIA_TYPE_VIDEO) return false;
    width = codecpar->width;
    height = codecpar->height;
    fps = (stream->avg_frame_rate.den > 0) ? av_q2d(stream->avg_frame_rate) : 0.0;
    auto* desc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(codecpar->format));
    pixel_format = desc ? desc->name : "unknown";
    return true;
#else
    return false;
#endif
}

bool FfmpegAdapter::get_stream_audio_info(FfmpegFormatHandle handle, int stream_index,
                                          int& sample_rate, int& channels,
                                          std::string& channel_layout,
                                          std::string& sample_format) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!handle.native) return false;
    auto* ctx = static_cast<AVFormatContext*>(handle.native);
    if (stream_index < 0 || static_cast<unsigned>(stream_index) >= ctx->nb_streams) return false;
    auto* stream = ctx->streams[stream_index];
    auto* codecpar = stream->codecpar;
    if (codecpar->codec_type != AVMEDIA_TYPE_AUDIO) return false;
    sample_rate = codecpar->sample_rate;
    channels = codecpar->ch_layout.nb_channels;
    char layout_str[128];
    av_channel_layout_describe(&codecpar->ch_layout, layout_str, sizeof(layout_str));
    channel_layout = std::string(layout_str);
    sample_format = av_get_sample_fmt_name(static_cast<AVSampleFormat>(codecpar->format));
    return true;
#else
    return false;
#endif
}

bool FfmpegAdapter::get_stream_metadata(FfmpegFormatHandle handle, int stream_index,
                                        std::string& language, int64_t& duration,
                                        int64_t& bitrate) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!handle.native) return false;
    auto* ctx = static_cast<AVFormatContext*>(handle.native);
    if (stream_index < 0 || static_cast<unsigned>(stream_index) >= ctx->nb_streams) return false;
    auto* stream = ctx->streams[stream_index];
    auto* lang = av_dict_get(stream->metadata, "language", nullptr, 0);
    language = lang ? lang->value : "";
    duration = (stream->duration != AV_NOPTS_VALUE) ? stream->duration : 0;
    bitrate = stream->codecpar->bit_rate;
    return true;
#else
    return false;
#endif
}

bool FfmpegAdapter::rescale_stream_timestamp_us(FfmpegFormatHandle handle,
                                                int stream_index,
                                                int64_t timestamp,
                                                int64_t& out_microseconds) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!handle.native) return false;
    auto* ctx = static_cast<AVFormatContext*>(handle.native);
    if (stream_index < 0 || static_cast<unsigned>(stream_index) >= ctx->nb_streams) return false;
    out_microseconds = av_rescale_q(timestamp,
                                    ctx->streams[stream_index]->time_base,
                                    AVRational{1, 1000000});
    return true;
#else
    (void)handle;
    (void)stream_index;
    (void)timestamp;
    (void)out_microseconds;
    return false;
#endif
}

// Packet reading

FfmpegReadResult FfmpegAdapter::read_packet(FfmpegFormatHandle handle) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!handle.native) return {.error_message = "null_format_context"};
    auto* ctx = static_cast<AVFormatContext*>(handle.native);

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) return {.error_message = "packet_alloc_failed"};

    int ret = av_read_frame(ctx, pkt);
    if (ret < 0) {
        av_packet_free(&pkt);
        if (ret == AVERROR_EOF || avio_feof(ctx->pb)) {
            return {.eof = true};
        }
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        return {.error_message = std::string(errbuf)};
    }

    FfmpegReadResult result;
    result.success = true;
    result.packet.stream_index = pkt->stream_index;
    // VC-1 在 Matroska 里**只有 dts 没有 pts**（ffprobe 实测：pts_time=N/A、
    // dts_time 递增 41 ms）。原来把 N/A 一律映射成 0，于是整条视频轨的时间戳
    // 都变成 0，进度条永远停在开头（真机 2026-11-10）。
    // 没有 pts 时退回 dts 是 FFmpeg 自己的做法（`AVFrame::best_effort_timestamp`
    // 同源）；有 B 帧时差一帧量级，对节拍与进度都够用，且比"恒 0"正确得多。
    const int64_t effective_pts = (pkt->pts != AV_NOPTS_VALUE) ? pkt->pts : pkt->dts;
    result.packet.pts = (effective_pts != AV_NOPTS_VALUE) ? effective_pts : 0;
    result.packet.dts = (pkt->dts != AV_NOPTS_VALUE) ? pkt->dts : 0;
    result.packet.duration = pkt->duration;
    result.packet.is_key_frame = (pkt->flags & AV_PKT_FLAG_KEY) != 0;

    if (pkt->data && pkt->size > 0) {
        result.packet_data.assign(pkt->data, pkt->data + pkt->size);
    }
    for (int index = 0; index < pkt->side_data_elems; ++index) {
        const AVPacketSideData& side = pkt->side_data[index];
        if (side.data == nullptr || side.size <= 0) {
            continue;
        }
        FfmpegPacketSideData copy;
        copy.type = static_cast<int>(side.type);
        copy.data.assign(side.data, side.data + side.size);
        result.side_data.push_back(std::move(copy));
    }

    av_packet_free(&pkt);
    return result;
#else
    return {.error_message = "ffmpeg_not_available"};
#endif
}

// Seeking

bool FfmpegAdapter::seek(FfmpegFormatHandle handle, double seconds) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!handle.native) return false;
    auto* ctx = static_cast<AVFormatContext*>(handle.native);
    int64_t target_ts = static_cast<int64_t>(seconds * AV_TIME_BASE);
    int ret = av_seek_frame(ctx, -1, target_ts, AVSEEK_FLAG_BACKWARD);
    return ret >= 0;
#else
    return false;
#endif
}

// Codec lifecycle

namespace {

// 只在启用 FFmpeg 时存在：参数类型 `AVCodecContext` / `AVCodec` 来自
// `<libavcodec/avcodec.h>`，而那张头文件本身也在 `KIVO_ENABLE_FFMPEG` 里。
//
// 这里原来没有条件包裹，于是**没启用 FFmpeg 的构建目标一个都编不过**：
// `AVCodecContext` 未声明 → C2182/C2065/C2059 连着报四条。
// 桌面那三个 `kivo_p7*_local_media_*` 测试一直"Not Run"，根因就是它 ——
// 缺二进制不是"没构建"，是**构建不出来**。
// 两个调用点（286 / 304 行的 `#ifdef` 内）本来就只在启用时才会走到。
#ifdef KIVO_ENABLE_FFMPEG
void configure_video_decoder_threading(AVCodecContext* const ctx,
                                       const AVCodec* const codec) noexcept {
    if (ctx == nullptr || codec == nullptr || codec->type != AVMEDIA_TYPE_VIDEO) {
        return;
    }
    // `0` = 交给 FFmpeg 用 `av_cpu_count()`。Android 上那个值受 cgroup 与
    // 大小核调度影响，实测 1080p VC-1 只有 ~12 fps（约 82 ms/帧），
    // 与单线程水平相当。这里显式给一个下限，让 frame threading 真的铺开。
    int detected = static_cast<int>(std::thread::hardware_concurrency());
    if (detected < 2) {
        detected = 2;
    }
    if (detected > 8) {
        detected = 8;  // 超过 8 路对解码没有增益，只会抢渲染线程的核
    }
    ctx->thread_count = detected;
    ctx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
}
#endif

}  // namespace

FfmpegCodecHandle FfmpegAdapter::open_decoder_by_name(const std::string& codec_name) {
#ifdef KIVO_ENABLE_FFMPEG
    const AVCodec* codec = avcodec_find_decoder_by_name(codec_name.c_str());
    if (!codec) return {nullptr};
    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    if (!ctx) return {nullptr};
    configure_video_decoder_threading(ctx, codec);
    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        avcodec_free_context(&ctx);
        return {nullptr};
    }
    return {ctx};
#else
    return {nullptr};
#endif
}

FfmpegCodecHandle FfmpegAdapter::create_codec_context(FfmpegFormatHandle format_handle,
                                                      int stream_index) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!format_handle.native) return {nullptr};
    auto* fmt = static_cast<AVFormatContext*>(format_handle.native);
    if (stream_index < 0 || static_cast<unsigned>(stream_index) >= fmt->nb_streams)
        return {nullptr};
    auto* codecpar = fmt->streams[stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) return {nullptr};
    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    if (!ctx) return {nullptr};
    if (avcodec_parameters_to_context(ctx, codecpar) < 0) {
        avcodec_free_context(&ctx); return {nullptr};
    }
    configure_video_decoder_threading(ctx, codec);
    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        avcodec_free_context(&ctx); return {nullptr};
    }
    return {ctx};
#else
    return {nullptr};
#endif
}

void FfmpegAdapter::free_codec_context(FfmpegCodecHandle& handle) {
#ifdef KIVO_ENABLE_FFMPEG
    if (handle.native) {
        auto* ctx = static_cast<AVCodecContext*>(handle.native);
        avcodec_free_context(&ctx);
        handle.native = nullptr;
    }
#else
    handle.native = nullptr;
#endif
}

FfmpegCodecInfo FfmpegAdapter::get_codec_info(FfmpegCodecHandle handle) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!handle.native) return {};
    auto* ctx = static_cast<AVCodecContext*>(handle.native);
    FfmpegCodecInfo info;
    info.width = ctx->width;
    info.height = ctx->height;
    if (ctx->pix_fmt != AV_PIX_FMT_NONE) {
        auto* desc = av_pix_fmt_desc_get(ctx->pix_fmt);
        info.pixel_format = desc ? desc->name : "unknown";
    }
    return info;
#else
    return {};
#endif
}

// Decode operations

FfmpegSendResult FfmpegAdapter::send_packet(FfmpegCodecHandle codec_handle,
                                            const FfmpegPacketInfo& info,
                                            const uint8_t* data, int size,
                                            const std::vector<FfmpegPacketSideData>& side_data) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!codec_handle.native) return {FfmpegSendStatus::Error, "null_codec_context"};
    auto* ctx = static_cast<AVCodecContext*>(codec_handle.native);

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) return {FfmpegSendStatus::Error, "packet_alloc_failed"};

    pkt->stream_index = info.stream_index;
    pkt->pts = info.pts;
    pkt->dts = info.dts;
    pkt->duration = info.duration;
    if (info.is_key_frame) pkt->flags |= AV_PKT_FLAG_KEY;

    if (data && size > 0) {
        av_packet_from_data(pkt, static_cast<uint8_t*>(av_malloc(size + AV_INPUT_BUFFER_PADDING_SIZE)), size);
        if (!pkt->data) { av_packet_free(&pkt); return {FfmpegSendStatus::Error, "data_alloc_failed"}; }
        memcpy(pkt->data, data, size);
        memset(pkt->data + size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
        pkt->size = size;
    }
    for (const auto& side : side_data) {
        if (side.data.empty()) {
            continue;
        }
        auto* side_buffer = av_packet_new_side_data(
            pkt,
            static_cast<AVPacketSideDataType>(side.type),
            static_cast<std::size_t>(side.data.size()));
        if (side_buffer == nullptr) {
            av_packet_free(&pkt);
            return {FfmpegSendStatus::Error, "side_data_alloc_failed"};
        }
        memcpy(side_buffer, side.data.data(), side.data.size());
    }

    const auto t_send0 = std::chrono::steady_clock::now();
    int ret = avcodec_send_packet(ctx, pkt);
    const auto t_send1 = std::chrono::steady_clock::now();
    g_send_ns.fetch_add(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t_send1 - t_send0).count(),
        std::memory_order_relaxed);
    g_send_calls.fetch_add(1, std::memory_order_relaxed);
    av_packet_free(&pkt);

    if (ret == 0) return {FfmpegSendStatus::Success, ""};
    if (ret == AVERROR(EAGAIN)) return {FfmpegSendStatus::NeedDrain, ""};
    if (ret == AVERROR_EOF) return {FfmpegSendStatus::EndOfStream, ""};
    char errbuf[128];
    av_strerror(ret, errbuf, sizeof(errbuf));
    return {FfmpegSendStatus::Error, std::string(errbuf)};
#else
    return {FfmpegSendStatus::Error, "ffmpeg_not_available"};
#endif
}

bool FfmpegAdapter::receive_frame(FfmpegCodecHandle codec_handle,
                                  FfmpegFrameResult& result) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!codec_handle.native) return false;
    auto* ctx = static_cast<AVCodecContext*>(codec_handle.native);

    AVFrame* frame = av_frame_alloc();
    if (!frame) return false;

    int ret = avcodec_receive_frame(ctx, frame);
    if (ret < 0) {
        av_frame_free(&frame);
        return false;
    }

    result.pts = frame->pts;
    result.duration = frame->duration;

    if (frame->width > 0 && frame->height > 0) {
        result.is_video = true;
        result.width = frame->width;
        result.height = frame->height;
        if (frame->format != AV_PIX_FMT_NONE) {
            auto* desc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(frame->format));
            result.pixel_format = desc ? desc->name : "unknown";
        }
        // Extract real pixel data from AVFrame
        result.plane_count = av_pix_fmt_count_planes(static_cast<AVPixelFormat>(frame->format));
        if (result.plane_count < 1) result.plane_count = 1;
        size_t total_size = 0;
        for (int p = 0; p < result.plane_count && p < 4; ++p) {
            result.linesize[p] = frame->linesize[p];
            int plane_height = (p == 0) ? frame->height : (frame->height + 1) / 2;
            int plane_width_bytes = frame->linesize[p];
            total_size += static_cast<size_t>(plane_height) * plane_width_bytes;
        }
        result.frame_data.resize(total_size);
        size_t offset = 0;
        for (int p = 0; p < result.plane_count && p < 4; ++p) {
            int plane_height = (p == 0) ? frame->height : (frame->height + 1) / 2;
            for (int row = 0; row < plane_height; ++row) {
                memcpy(result.frame_data.data() + offset,
                       frame->data[p] + row * frame->linesize[p],
                       frame->linesize[p]);
                offset += frame->linesize[p];
            }
        }
    } else if (frame->nb_samples > 0) {
        result.is_audio = true;
        result.sample_rate = frame->sample_rate;
        result.channels = frame->ch_layout.nb_channels;
        result.sample_format = av_get_sample_fmt_name(static_cast<AVSampleFormat>(frame->format));
        AVSampleFormat fmt = static_cast<AVSampleFormat>(frame->format);
        const int bytes_per_sample = av_get_bytes_per_sample(fmt);
        const bool planar = av_sample_fmt_is_planar(fmt) != 0;
        if (bytes_per_sample <= 0 || result.channels <= 0) {
            av_frame_free(&frame);
            return false;
        }

        if (planar) {
            const int plane_count = result.channels < 8 ? result.channels : 8;
            const int plane_bytes = frame->nb_samples * bytes_per_sample;
            result.plane_count = plane_count;
            result.frame_data.resize(
                static_cast<size_t>(plane_count) * static_cast<size_t>(plane_bytes));
            size_t offset = 0;
            for (int p = 0; p < plane_count; ++p) {
                result.linesize[p] = plane_bytes;
                if (frame->data[p] != nullptr && plane_bytes > 0) {
                    memcpy(result.frame_data.data() + offset,
                           frame->data[p],
                           static_cast<size_t>(plane_bytes));
                }
                offset += static_cast<size_t>(plane_bytes);
            }
        } else {
            const int packed_bytes =
                frame->nb_samples * result.channels * bytes_per_sample;
            result.plane_count = 1;
            result.linesize[0] = packed_bytes;
            result.frame_data.resize(static_cast<size_t>(packed_bytes));
            if (frame->data[0] != nullptr && packed_bytes > 0) {
                memcpy(result.frame_data.data(),
                       frame->data[0],
                       static_cast<size_t>(packed_bytes));
            }
        }
    }

    av_frame_free(&frame);
    return true;
#else
    return false;
#endif
}

bool FfmpegAdapter::receive_frame_rgba(FfmpegCodecHandle codec_handle,
                                       FfmpegFrameResult& result) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!codec_handle.native) return false;
    auto* ctx = static_cast<AVCodecContext*>(codec_handle.native);

    AVFrame* frame = av_frame_alloc();
    if (!frame) return false;

    const auto t_decode0 = std::chrono::steady_clock::now();
    int ret = avcodec_receive_frame(ctx, frame);
    const auto t_decode1 = std::chrono::steady_clock::now();
    g_avcodec_ns.fetch_add(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t_decode1 - t_decode0).count(),
        std::memory_order_relaxed);
    if (ret < 0) {
        av_frame_free(&frame);
        return false;
    }

    if (frame->width <= 0 || frame->height <= 0 || frame->format == AV_PIX_FMT_NONE) {
        av_frame_free(&frame);
        return false;
    }

    struct ThreadLocalSwsContext {
        SwsContext* context{nullptr};

        ~ThreadLocalSwsContext() {
            if (context != nullptr) {
                sws_freeContext(context);
                context = nullptr;
            }
        }
    };

    thread_local ThreadLocalSwsContext sws_cache;
    // 必须用 BILINEAR：色度是 4:2:0（半分辨率），转 RGBA 时**需要**插值上采样。
    // 曾试过 SWS_POINT（以为同尺寸可以省掉滤波），实测既没变快（±3%），
    // 又会让彩色边缘起锯齿 —— 拿画质换不到性能，不划算。
    sws_cache.context = sws_getCachedContext(
        sws_cache.context,
        frame->width,
        frame->height,
        static_cast<AVPixelFormat>(frame->format),
        frame->width,
        frame->height,
        AV_PIX_FMT_RGBA,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr);
    if (sws_cache.context == nullptr) {
        av_frame_free(&frame);
        return false;
    }

    result = {};
    result.is_video = true;
    result.pts = frame->pts;
    result.duration = frame->duration;
    result.width = frame->width;
    result.height = frame->height;
    result.pixel_format = "rgba";
    result.plane_count = 1;
    result.linesize[0] = frame->width * 4;
    const auto t_resize0 = std::chrono::steady_clock::now();
    const std::size_t need_bytes =
        static_cast<std::size_t>(frame->width) * static_cast<std::size_t>(frame->height) * 4U;
    // 优先从池里取现成的一块（上一个调用方还回来的）。命中时零分配、零清零：
    // sws_scale 会把整块写满，残留内容不会被看到。
    if (g_frame_buffer_pool.size() == need_bytes) {
        result.frame_data.swap(g_frame_buffer_pool);
    } else {
        result.frame_data.resize(need_bytes);
    }
    const auto t_resize1 = std::chrono::steady_clock::now();
    g_resize_ns.fetch_add(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t_resize1 - t_resize0).count(),
        std::memory_order_relaxed);

    std::uint8_t* dst_data[4] = {result.frame_data.data(), nullptr, nullptr, nullptr};
    const int dst_linesize[4] = {frame->width * 4, 0, 0, 0};
    const auto t_sws0 = std::chrono::steady_clock::now();
    const int converted_rows = sws_scale(
        sws_cache.context,
        frame->data,
        frame->linesize,
        0,
        frame->height,
        dst_data,
        dst_linesize);
    const auto t_sws1 = std::chrono::steady_clock::now();
    g_sws_ns.fetch_add(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t_sws1 - t_sws0).count(),
        std::memory_order_relaxed);
    g_frame_count.fetch_add(1, std::memory_order_relaxed);
    av_frame_free(&frame);
    if (converted_rows <= 0) {
        result = {};
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool FfmpegAdapter::seek_file_us(FfmpegFormatHandle handle, int stream_index,
                                 std::int64_t target_us) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!handle.native || stream_index < 0) return false;
    auto* ctx = static_cast<AVFormatContext*>(handle.native);
    if (stream_index >= static_cast<int>(ctx->nb_streams)) return false;
    const AVRational tb = ctx->streams[stream_index]->time_base;
    const std::int64_t ts = av_rescale_q(target_us, AVRational{1, 1000000}, tb);
    // 用 `av_seek_frame` 而不是 `avformat_seek_file`：后者的 min/max 边界语义
    // 要求调用方给合理窗口，传 INT64_MIN/MAX 时 matroska 会直接拒绝（真机实测
    // 返回负值，表现为"暂停时拖进度条毫无反应"）。`AVSEEK_FLAG_BACKWARD`
    // 就是"定位到目标之前最近的关键帧"，正是这里要的语义，且各 demuxer 都支持。
    const int ret = av_seek_frame(ctx, stream_index, ts, AVSEEK_FLAG_BACKWARD);
    return ret >= 0;
#else
    return false;
#endif
}

void FfmpegAdapter::flush_decoder(FfmpegCodecHandle codec_handle) {
#ifdef KIVO_ENABLE_FFMPEG
    if (!codec_handle.native) return;
    auto* ctx = static_cast<AVCodecContext*>(codec_handle.native);
    avcodec_flush_buffers(ctx);
#endif
}

// Utility

std::string FfmpegAdapter::map_error(int ffmpeg_error) {
#ifdef KIVO_ENABLE_FFMPEG
    char buf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ffmpeg_error, buf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(buf);
#else
    return "ffmpeg_not_available";
#endif
}

std::string FfmpegAdapter::get_codec_name_from_id(int codec_id) {
#ifdef KIVO_ENABLE_FFMPEG
    auto* desc = avcodec_descriptor_get(static_cast<AVCodecID>(codec_id));
    return desc ? desc->name : "unknown";
#else
    return "unknown";
#endif
}

std::string FfmpegAdapter::get_pixel_format_name(int pixel_format) {
#ifdef KIVO_ENABLE_FFMPEG
    auto* desc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(pixel_format));
    return desc ? desc->name : "unknown";
#else
    return "unknown";
#endif
}

std::string FfmpegAdapter::get_sample_format_name(int sample_format) {
#ifdef KIVO_ENABLE_FFMPEG
    return av_get_sample_fmt_name(static_cast<AVSampleFormat>(sample_format));
#else
    return "unknown";
#endif
}

std::string FfmpegAdapter::get_channel_layout_description(uint64_t channel_layout) {
#ifdef KIVO_ENABLE_FFMPEG
    // FFmpeg 7.1: use new AVChannelLayout API instead of removed av_get_channel_layout_string
    AVChannelLayout layout{};
    layout.order = AV_CHANNEL_ORDER_NATIVE;
    layout.u.mask = channel_layout;
    char buf[128];
    av_channel_layout_describe(&layout, buf, sizeof(buf));
    return std::string(buf);
#else
    return "unknown";
#endif
}

FfmpegAdapter::DecodeTimings FfmpegAdapter::take_decode_timings() {
    DecodeTimings timings;
    timings.send_ns = g_send_ns.load(std::memory_order_relaxed);
    timings.send_calls = g_send_calls.load(std::memory_order_relaxed);
    timings.resize_ns = g_resize_ns.load(std::memory_order_relaxed);
    timings.avcodec_ns = g_avcodec_ns.load(std::memory_order_relaxed);
    timings.sws_ns = g_sws_ns.load(std::memory_order_relaxed);
    timings.frames = g_frame_count.load(std::memory_order_relaxed);
    return timings;
}

void FfmpegAdapter::reset_decode_timings() {
    g_send_ns.store(0, std::memory_order_relaxed);
    g_send_calls.store(0, std::memory_order_relaxed);
    g_resize_ns.store(0, std::memory_order_relaxed);
    g_avcodec_ns.store(0, std::memory_order_relaxed);
    g_sws_ns.store(0, std::memory_order_relaxed);
    g_frame_count.store(0, std::memory_order_relaxed);
}

void FfmpegAdapter::recycle_frame_buffer(std::vector<uint8_t>&& buffer) {
    // 只接受"正好是一整帧"的块，尺寸不符就丢掉——不同分辨率的帧混进池子会让
    // 下一帧的尺寸判断失效，退回到重新分配，反而更慢。
    if (!buffer.empty()) {
        g_frame_buffer_pool = std::move(buffer);
    }
}

}  // namespace kivo::cinema_engine
