#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"

// Conditionally include FFmpeg headers
#ifdef KIVO_ENABLE_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
}
#endif

#include <stdexcept>

namespace kivo::cinema_engine {

#ifdef KIVO_ENABLE_FFMPEG

AVFormatContext* FfmpegAdapter::open_file(const std::string& path) {
    AVFormatContext* format_context = nullptr;
    int ret = avformat_open_input(&format_context, path.c_str(), nullptr, nullptr);
    if (ret < 0) {
        return nullptr;
    }
    
    ret = avformat_find_stream_info(format_context, nullptr);
    if (ret < 0) {
        avformat_close_input(&format_context);
        return nullptr;
    }
    
    return format_context;
}

bool FfmpegAdapter::probe_container(AVFormatContext* format_context,
                                   std::string& container_format,
                                   double& duration,
                                   int64_t& bitrate_estimate) {
    if (!format_context) {
        return false;
    }
    
    container_format = format_context->iformat->name;
    duration = (format_context->duration != AV_NOPTS_VALUE) ? 
               static_cast<double>(format_context->duration) / AV_TIME_BASE : 0.0;
    bitrate_estimate = format_context->bit_rate;
    
    return true;
}

bool FfmpegAdapter::enumerate_streams(AVFormatContext* format_context,
                                     std::vector<int>& video_stream_indices,
                                     std::vector<int>& audio_stream_indices,
                                     std::vector<int>& subtitle_stream_indices) {
    if (!format_context) {
        return false;
    }
    
    video_stream_indices.clear();
    audio_stream_indices.clear();
    subtitle_stream_indices.clear();
    
    for (unsigned int i = 0; i < format_context->nb_streams; i++) {
        AVMediaType type = format_context->streams[i]->codecpar->codec_type;
        switch (type) {
            case AVMEDIA_TYPE_VIDEO:
                video_stream_indices.push_back(i);
                break;
            case AVMEDIA_TYPE_AUDIO:
                audio_stream_indices.push_back(i);
                break;
            case AVMEDIA_TYPE_SUBTITLE:
                subtitle_stream_indices.push_back(i);
                break;
            default:
                break;
        }
    }
    
    return true;
}

AVCodecContext* FfmpegAdapter::create_codec_context(AVFormatContext* format_context, int stream_index) {
    if (!format_context || stream_index < 0 || 
        static_cast<unsigned int>(stream_index) >= format_context->nb_streams) {
        return nullptr;
    }
    
    AVCodecParameters* codecpar = format_context->streams[stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        return nullptr;
    }
    
    AVCodecContext* codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        return nullptr;
    }
    
    int ret = avcodec_parameters_to_context(codec_context, codecpar);
    if (ret < 0) {
        avcodec_free_context(&codec_context);
        return nullptr;
    }
    
    ret = avcodec_open2(codec_context, codec, nullptr);
    if (ret < 0) {
        avcodec_free_context(&codec_context);
        return nullptr;
    }
    
    return codec_context;
}

bool FfmpegAdapter::decode_packet(AVCodecContext* codec_context,
                                 AVPacket* packet,
                                 AVFrame* frame) {
    if (!codec_context || !packet || !frame) {
        return false;
    }
    
    int ret = avcodec_send_packet(codec_context, packet);
    if (ret < 0) {
        return false;
    }
    
    ret = avcodec_receive_frame(codec_context, frame);
    return ret == 0;
}

void FfmpegAdapter::free_format_context(AVFormatContext* format_context) {
    if (format_context) {
        avformat_close_input(&format_context);
    }
}

void FfmpegAdapter::free_codec_context(AVCodecContext* codec_context) {
    if (codec_context) {
        avcodec_free_context(&codec_context);
    }
}

void FfmpegAdapter::free_packet(AVPacket* packet) {
    if (packet) {
        av_packet_free(&packet);
    }
}

void FfmpegAdapter::free_frame(AVFrame* frame) {
    if (frame) {
        av_frame_free(&frame);
    }
}

std::string FfmpegAdapter::map_error(int ffmpeg_error) {
    char error_buf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ffmpeg_error, error_buf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(error_buf);
}

std::string FfmpegAdapter::get_codec_name(int codec_id) {
    const AVCodecDescriptor* desc = avcodec_descriptor_get(static_cast<AVCodecID>(codec_id));
    return desc ? desc->name : "unknown";
}

std::string FfmpegAdapter::get_pixel_format_name(int pixel_format) {
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(pixel_format));
    return desc ? desc->name : "unknown";
}

std::string FfmpegAdapter::get_sample_format_name(int sample_format) {
    return av_get_sample_fmt_name(static_cast<AVSampleFormat>(sample_format));
}

std::string FfmpegAdapter::get_channel_layout_description(uint64_t channel_layout) {
    char layout_str[128];
    av_get_channel_layout_string(layout_str, sizeof(layout_str), -1, channel_layout);
    return std::string(layout_str);
}

bool FfmpegAdapter::get_stream_codec_name(AVFormatContext* format_context, int stream_index, std::string& codec_name) {
    if (!format_context || stream_index < 0 ||
        static_cast<unsigned int>(stream_index) >= format_context->nb_streams) {
        return false;
    }
    AVCodecParameters* codecpar = format_context->streams[stream_index]->codecpar;
    const AVCodecDescriptor* desc = avcodec_descriptor_get(codecpar->codec_id);
    codec_name = desc ? desc->name : "unknown";
    return true;
}

bool FfmpegAdapter::get_stream_video_info(AVFormatContext* format_context, int stream_index,
                                          int& width, int& height, double& fps, std::string& pixel_format) {
    if (!format_context || stream_index < 0 ||
        static_cast<unsigned int>(stream_index) >= format_context->nb_streams) {
        return false;
    }
    auto* stream = format_context->streams[stream_index];
    auto* codecpar = stream->codecpar;
    if (codecpar->codec_type != AVMEDIA_TYPE_VIDEO) return false;
    width = codecpar->width;
    height = codecpar->height;
    if (stream->avg_frame_rate.den > 0) {
        fps = av_q2d(stream->avg_frame_rate);
    } else {
        fps = 0.0;
    }
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(codecpar->format));
    pixel_format = desc ? desc->name : "unknown";
    return true;
}

bool FfmpegAdapter::get_stream_audio_info(AVFormatContext* format_context, int stream_index,
                                          int& sample_rate, int& channels, std::string& channel_layout, std::string& sample_format) {
    if (!format_context || stream_index < 0 ||
        static_cast<unsigned int>(stream_index) >= format_context->nb_streams) {
        return false;
    }
    auto* stream = format_context->streams[stream_index];
    auto* codecpar = stream->codecpar;
    if (codecpar->codec_type != AVMEDIA_TYPE_AUDIO) return false;
    sample_rate = codecpar->sample_rate;
    channels = codecpar->ch_layout.nb_channels;
    char layout_str[128];
    av_get_channel_layout_string(layout_str, sizeof(layout_str), -1, codecpar->ch_layout.u.mask);
    channel_layout = std::string(layout_str);
    sample_format = av_get_sample_fmt_name(static_cast<AVSampleFormat>(codecpar->format));
    return true;
}

bool FfmpegAdapter::get_stream_metadata(AVFormatContext* format_context, int stream_index,
                                        std::string& language, int64_t& duration, int64_t& bitrate) {
    if (!format_context || stream_index < 0 ||
        static_cast<unsigned int>(stream_index) >= format_context->nb_streams) {
        return false;
    }
    auto* stream = format_context->streams[stream_index];
    auto* lang = av_dict_get(stream->metadata, "language", nullptr, 0);
    language = lang ? lang->value : "";
    duration = (stream->duration != AV_NOPTS_VALUE) ? stream->duration : 0;
    bitrate = stream->codecpar->bit_rate;
    return true;
}

#else

// Stub implementations when FFmpeg is not enabled
AVFormatContext* FfmpegAdapter::open_file(const std::string& path) {
    return nullptr;
}

bool FfmpegAdapter::probe_container(AVFormatContext* format_context,
                                   std::string& container_format,
                                   double& duration,
                                   int64_t& bitrate_estimate) {
    return false;
}

bool FfmpegAdapter::enumerate_streams(AVFormatContext* format_context,
                                     std::vector<int>& video_stream_indices,
                                     std::vector<int>& audio_stream_indices,
                                     std::vector<int>& subtitle_stream_indices) {
    return false;
}

AVCodecContext* FfmpegAdapter::create_codec_context(AVFormatContext* format_context, int stream_index) {
    return nullptr;
}

bool FfmpegAdapter::decode_packet(AVCodecContext* codec_context,
                                 AVPacket* packet,
                                 AVFrame* frame) {
    return false;
}

void FfmpegAdapter::free_format_context(AVFormatContext* format_context) {}
void FfmpegAdapter::free_codec_context(AVCodecContext* codec_context) {}
void FfmpegAdapter::free_packet(AVPacket* packet) {}
void FfmpegAdapter::free_frame(AVFrame* frame) {}

std::string FfmpegAdapter::map_error(int ffmpeg_error) {
    return "ffmpeg_not_enabled";
}

std::string FfmpegAdapter::get_codec_name(int codec_id) {
    return "unknown";
}

std::string FfmpegAdapter::get_pixel_format_name(int pixel_format) {
    return "unknown";
}

std::string FfmpegAdapter::get_sample_format_name(int sample_format) {
    return "unknown";
}

std::string FfmpegAdapter::get_channel_layout_description(uint64_t channel_layout) {
    return "unknown";
}

bool FfmpegAdapter::get_stream_codec_name(AVFormatContext* format_context, int stream_index, std::string& codec_name) {
    return false;
}

bool FfmpegAdapter::get_stream_video_info(AVFormatContext* format_context, int stream_index,
                                          int& width, int& height, double& fps, std::string& pixel_format) {
    return false;
}

bool FfmpegAdapter::get_stream_audio_info(AVFormatContext* format_context, int stream_index,
                                          int& sample_rate, int& channels, std::string& channel_layout, std::string& sample_format) {
    return false;
}

bool FfmpegAdapter::get_stream_metadata(AVFormatContext* format_context, int stream_index,
                                        std::string& language, int64_t& duration, int64_t& bitrate) {
    return false;
}

#endif

}  // namespace kivo::cinema_engine