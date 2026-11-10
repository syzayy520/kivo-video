#include "video/video_plane/backends/ffmpeg/demux/codec_config_builder.hpp"

#include <cstdlib>
#include <vector>

extern "C" {
#include <libavutil/dict.h>
}

namespace kivo::video::demux {
namespace {

// A/B 实验记录（2026-11-09 18:05）：把 hvcC 拆成 AnnexB 参数集后，
// `frames_received` 2 → 3、`rate` 0.36 → 0.0，**更差**。
// 说明 csd 形状不是"914 包只出 2 帧"的根因，且 hvcC + AVCC 的原始组合
// 至少还能出帧。转换函数已删除，不留无用的分支让人误以为它在生效。

constexpr const char* kKeyMime = "mime";
constexpr const char* kKeyWidth = "width";
constexpr const char* kKeyHeight = "height";
constexpr const char* kKeyDuration = "durationUs";
constexpr const char* kKeySampleRate = "sample-rate";
constexpr const char* kKeyChannelCount = "channel-count";
constexpr const char* kKeyCsd0 = "csd-0";
constexpr const char* kKeyCsd1 = "csd-1";
constexpr const char* kKeyRotation = "rotation";

/// 容器时长统一成微秒。轨道 duration 优先；缺失时退回整个容器的
/// `AVFormatContext::duration`（单位就是 AV_TIME_BASE，即微秒）。
[[nodiscard]] std::int64_t duration_us(AVFormatContext* ctx, AVStream* stream) {
    if (stream->duration != AV_NOPTS_VALUE && stream->duration > 0) {
        return static_cast<std::int64_t>(stream->duration * av_q2d(stream->time_base) * 1'000'000.0);
    }
    if (ctx->duration != AV_NOPTS_VALUE && ctx->duration > 0) {
        return static_cast<std::int64_t>(ctx->duration);
    }
    return -1;
}

[[nodiscard]] std::int32_t rotation_degrees(AVStream* stream) {
    const AVDictionaryEntry* entry = av_dict_get(stream->metadata, "rotate", nullptr, 0);
    if (entry == nullptr || entry->value == nullptr) {
        return 0;
    }
    const int value = std::atoi(entry->value);
    if (value == 90 || value == 180 || value == 270) {
        return value;
    }
    return 0;
}

}  // namespace

const char* CodecConfigBuilder::android_mime(const AVCodecID codec) {
    switch (codec) {
        case AV_CODEC_ID_HEVC: return "video/hevc";
        case AV_CODEC_ID_H264: return "video/avc";
        case AV_CODEC_ID_MPEG4: return "video/mp4v-es";
        case AV_CODEC_ID_VP8: return "video/x-vnd.on2.vp8";
        case AV_CODEC_ID_VP9: return "video/x-vnd.on2.vp9";
        case AV_CODEC_ID_AV1: return "video/av01";
        case AV_CODEC_ID_AAC: return "audio/mp4a-latm";
        case AV_CODEC_ID_AC3: return "audio/ac3";
        case AV_CODEC_ID_EAC3: return "audio/eac3";
        case AV_CODEC_ID_DTS: return "audio/vnd.dts";
        case AV_CODEC_ID_TRUEHD: return "audio/true-hd";
        case AV_CODEC_ID_FLAC: return "audio/flac";
        case AV_CODEC_ID_OPUS: return "audio/opus";
        case AV_CODEC_ID_VORBIS: return "audio/vorbis";
        case AV_CODEC_ID_MP3: return "audio/mpeg";
        default: return nullptr;
    }
}

BuiltTrackFormat CodecConfigBuilder::build(AVFormatContext* ctx, AVStream* stream,
                                           const TrackKind kind) {
    BuiltTrackFormat out{};

    const char* mime = android_mime(stream->codecpar->codec_id);
    if (mime == nullptr) {
        out.message = "unsupported codec id " + std::to_string(stream->codecpar->codec_id);
        return out;
    }

    AMediaFormat* format = AMediaFormat_new();
    if (format == nullptr) {
        out.message = "AMediaFormat_new returned null";
        return out;
    }
    AMediaFormat_setString(format, kKeyMime, mime);
    AMediaFormat_setInt64(format, kKeyDuration, duration_us(ctx, stream));

    // csd-0 直接透传 extradata。对 MKV 而言 HEVC/H.264 的 CodecPrivate 本身就是
    // hvcC/avcC，而 AVCC 形状的样本配 avcC 形状的 csd 是 MediaCodec 的标准组合。
    // 这里刻意不做 AnnexB 转换：转换会改变样本与 csd 的封装一致性，
    // 是"能编过但花屏"的典型来源；真需要转换时必须有实测证据再改。
    if (stream->codecpar->extradata != nullptr && stream->codecpar->extradata_size > 0) {
        AMediaFormat_setBuffer(format, kKeyCsd0, stream->codecpar->extradata,
                               stream->codecpar->extradata_size);
    }

    if (kind == TrackKind::kVideo) {
        if (stream->codecpar->width <= 0 || stream->codecpar->height <= 0) {
            AMediaFormat_delete(format);
            out.message = "video track reports invalid geometry " +
                          std::to_string(stream->codecpar->width) + "x" +
                          std::to_string(stream->codecpar->height);
            return out;
        }
        AMediaFormat_setInt32(format, kKeyWidth, stream->codecpar->width);
        AMediaFormat_setInt32(format, kKeyHeight, stream->codecpar->height);

        out.video.mime = mime;
        out.video.width = stream->codecpar->width;
        out.video.height = stream->codecpar->height;
        out.video.duration_us = duration_us(ctx, stream);
        out.video.rotation_degrees = rotation_degrees(stream);
        if (out.video.rotation_degrees != 0) {
            AMediaFormat_setInt32(format, kKeyRotation, out.video.rotation_degrees);
        }
    } else {
        const int sample_rate = stream->codecpar->sample_rate;
        const int channels = stream->codecpar->ch_layout.nb_channels;
        if (sample_rate <= 0 || channels <= 0) {
            AMediaFormat_delete(format);
            out.message = "audio track reports invalid format (rate=" +
                          std::to_string(sample_rate) + " channels=" + std::to_string(channels) + ")";
            return out;
        }
        AMediaFormat_setInt32(format, kKeySampleRate, sample_rate);
        AMediaFormat_setInt32(format, kKeyChannelCount, channels);

        out.audio.mime = mime;
        out.audio.sample_rate = sample_rate;
        out.audio.channel_count = channels;
        out.audio.duration_us = duration_us(ctx, stream);
    }

    out.format = format;
    out.ok = true;
    return out;
}

}  // namespace kivo::video::demux
