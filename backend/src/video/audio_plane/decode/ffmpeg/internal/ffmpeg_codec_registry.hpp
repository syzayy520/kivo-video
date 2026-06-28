// ffmpeg_codec_registry.hpp — P6C internal: P6 codec id <-> FFmpeg AVCodecID mapping
// INTERNAL header: includes FFmpeg headers (allowed under backend/src/.../internal/).
// avformat/demux is FORBIDDEN — avcodec layer only.
#pragma once

#include <cstdint>

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavcodec/packet.h>

namespace kivo::video::audio_plane::backend::decode::ffmpeg::internal {

// P6 codec id values are stable across the P6 contract surface. They are
// intentionally distinct from FFmpeg's AVCodecID so the contract layer never
// imports FFmpeg types. The mapping below is the single point of translation.
//
//   1 = AAC,        2 = MP3,       3 = AC3,    4 = EAC3,
//   5 = DTS,         6 = TrueHD,    7 = FLAC,   8 = Vorbis,
//   9 = Opus,        0 or unknown -> AV_CODEC_ID_NONE
inline AVCodecID to_avcodec_id(int32_t p6_codec_id) noexcept {
    switch (p6_codec_id) {
        case 1:  return AV_CODEC_ID_AAC;
        case 2:  return AV_CODEC_ID_MP3;
        case 3:  return AV_CODEC_ID_AC3;
        case 4:  return AV_CODEC_ID_EAC3;
        case 5:  return AV_CODEC_ID_DTS;
        case 6:  return AV_CODEC_ID_TRUEHD;
        case 7:  return AV_CODEC_ID_FLAC;
        case 8:  return AV_CODEC_ID_VORBIS;
        case 9:  return AV_CODEC_ID_OPUS;
        default: return AV_CODEC_ID_NONE;
    }
}

// True when the P6 codec id maps to a real AVCodecID (i.e. not NONE).
inline bool is_supported(int32_t p6_codec_id) noexcept {
    return to_avcodec_id(p6_codec_id) != AV_CODEC_ID_NONE;
}

// Whether the codec requires codec-private data (extradata) to be supplied to
// the decoder before the first packet can be decoded.
//
//   - AAC family needs AudioSpecificConfig (extradata).
//   - Vorbis needs the identification/setup headers (extradata).
//   - Opus needs the OpusHead-like config block as extradata under FFmpeg.
//   - MP3 is self-syncing per-frame and does not strictly need extradata.
//   - AC3/EAC3/DTS/TrueHD/FLAC do not strictly require extradata, though
//     FLAC optionally carries streaminfo in extradata.
//
// Default is conservative: assume extradata is needed for unknown codecs so
// the caller fails safe rather than attempting to decode header-less.
inline bool requires_codec_private_data(AVCodecID id) noexcept {
    switch (id) {
        case AV_CODEC_ID_AAC:
            // AAC (incl. AAC-LC / HE-AAC variants resolved to AV_CODEC_ID_AAC)
            // requires AudioSpecificConfig.
            return true;
        case AV_CODEC_ID_VORBIS:
            // Vorbis requires identification/comment/setup headers.
            return true;
        case AV_CODEC_ID_OPUS:
            // Opus requires the channel mapping / pre-skip config block.
            return true;
        case AV_CODEC_ID_MP3:
        case AV_CODEC_ID_AC3:
        case AV_CODEC_ID_EAC3:
        case AV_CODEC_ID_DTS:
        case AV_CODEC_ID_TRUEHD:
        case AV_CODEC_ID_FLAC:
            return false;
        default:
            // Conservative: assume needed when unknown.
            return true;
    }
}

// Human-readable name for evidence / logging. Returns a static string literal
// for the P6 codec id. Unknown ids return "Unknown".
inline const char* p6_codec_id_name(int32_t p6_codec_id) noexcept {
    switch (p6_codec_id) {
        case 1:  return "AAC";
        case 2:  return "MP3";
        case 3:  return "AC3";
        case 4:  return "EAC3";
        case 5:  return "DTS";
        case 6:  return "TrueHD";
        case 7:  return "FLAC";
        case 8:  return "Vorbis";
        case 9:  return "Opus";
        case 0:  return "None";
        default: return "Unknown";
    }
}

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg::internal
