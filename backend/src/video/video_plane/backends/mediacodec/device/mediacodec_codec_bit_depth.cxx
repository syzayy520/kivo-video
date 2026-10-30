#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_bit_depth.hpp"

#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_profile_level_ids.hpp"

namespace kivo::video::mediacodec {
namespace {

constexpr std::int32_t kBitDepthUnknown = 0;
constexpr std::int32_t kBitDepth8 = 8;
constexpr std::int32_t kBitDepth10 = 10;

}  // namespace

std::int32_t infer_bit_depth(std::string_view mime, std::int32_t profile) {
    if (profile < 0) {
        return kBitDepthUnknown;
    }

    if (mime == "video/hevc") {
        switch (profile) {
            case kHevcProfileMain:
                return kBitDepth8;
            case kHevcProfileMain10:
            case kHevcProfileMain10Hdr10:
            case kHevcProfileMain10Hdr10Plus:
                return kBitDepth10;
            default:
                return kBitDepthUnknown;
        }
    }

    if (mime == "video/avc") {
        switch (profile) {
            case kAvcProfileBaseline:
            case kAvcProfileMain:
            case kAvcProfileExtended:
            case kAvcProfileHigh:
            case kAvcProfileConstrainedBaseline:
            case kAvcProfileConstrainedHigh:
                return kBitDepth8;
            case kAvcProfileHigh10:
                return kBitDepth10;
            default:
                return kBitDepthUnknown;
        }
    }

    if (mime == "video/x-vnd.on2.vp9") {
        switch (profile) {
            case kVp9Profile0:
            case kVp9Profile1:
                return kBitDepth8;
            case kVp9Profile2:
            case kVp9Profile3:
                return kBitDepth10;
            default:
                return kBitDepthUnknown;
        }
    }

    if (mime == "video/av01") {
        switch (profile) {
            case kAv1ProfileMain8:
                return kBitDepth8;
            case kAv1ProfileMain10:
            case kAv1ProfileMain10Hdr10:
            case kAv1ProfileMain10Hdr10Plus:
                return kBitDepth10;
            default:
                return kBitDepthUnknown;
        }
    }

    return kBitDepthUnknown;
}

}  // namespace kivo::video::mediacodec
