#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"

#include "ffmpeg_product_log_filter.hpp"

#ifdef KIVO_ENABLE_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
#include <libswscale/swscale.h>
}
#endif

namespace kivo::cinema_engine {

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
    result.packet.pts = (pkt->pts != AV_NOPTS_VALUE) ? pkt->pts : 0;
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

void configure_video_decoder_threading(AVCodecContext* const ctx,
                                       const AVCodec* const codec) noexcept {
    if (ctx == nullptr || codec == nullptr || codec->type != AVMEDIA_TYPE_VIDEO) {
        return;
    }
    ctx->thread_count = 0;
    ctx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
}

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

    int ret = avcodec_send_packet(ctx, pkt);
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

    int ret = avcodec_receive_frame(ctx, frame);
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
    result.frame_data.resize(
        static_cast<std::size_t>(frame->width) *
        static_cast<std::size_t>(frame->height) * 4U);

    std::uint8_t* dst_data[4] = {result.frame_data.data(), nullptr, nullptr, nullptr};
    const int dst_linesize[4] = {frame->width * 4, 0, 0, 0};
    const int converted_rows = sws_scale(
        sws_cache.context,
        frame->data,
        frame->linesize,
        0,
        frame->height,
        dst_data,
        dst_linesize);
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

}  // namespace kivo::cinema_engine
