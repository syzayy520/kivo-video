#include "video/video_plane/backends/mediacodec/decode/mediacodec_format_guard.hpp"

#include <string_view>

namespace kivo::video::mediacodec {
namespace {

constexpr std::string_view kVideoMimePrefix = "video/";

/// "csd-0" 的常量形式 AMEDIAFORMAT_KEY_CSD_0 是 API 28 才导出的，
/// 键名本身自 API 21 稳定。minSdk 26 只能写字面量。
constexpr const char* kKeyCsd0 = "csd-0";

[[nodiscard]] bool read_int32(AMediaFormat* format, const char* key, std::int32_t& out) {
    return AMediaFormat_getInt32(format, key, &out);
}

[[nodiscard]] bool has_key(AMediaFormat* format, const char* key) {
    std::size_t size = 0;
    void* data = nullptr;
    return AMediaFormat_getBuffer(format, key, &data, &size) && data != nullptr && size > 0;
}

}  // namespace

DecoderFormatOutcome validate_decoder_format(const AMediaFormat* format) {
    if (format == nullptr) {
        return {DecoderFormatOutcome::Code::kNullFormat, "track format is null"};
    }

    auto* mutable_format = const_cast<AMediaFormat*>(format);

    const char* mime = nullptr;
    if (!AMediaFormat_getString(mutable_format, AMEDIAFORMAT_KEY_MIME, &mime) || mime == nullptr) {
        return {DecoderFormatOutcome::Code::kMissingMime, "track format has no mime"};
    }
    if (std::string_view(mime).rfind(kVideoMimePrefix, 0) != 0) {
        return {DecoderFormatOutcome::Code::kMissingMime,
                "track mime is not a video type: " + std::string(mime)};
    }

    std::int32_t width = 0;
    std::int32_t height = 0;
    if (!read_int32(mutable_format, AMEDIAFORMAT_KEY_WIDTH, width) ||
        !read_int32(mutable_format, AMEDIAFORMAT_KEY_HEIGHT, height) || width <= 0 ||
        height <= 0) {
        return {DecoderFormatOutcome::Code::kMissingGeometry,
                "track format has no usable geometry (" + std::to_string(width) + "x" +
                    std::to_string(height) + ")"};
    }

    // VP8/VP9/AV1 不需要 csd（码流自带参数集），只有 AVC/HEVC 依赖 csd-0。
    const std::string_view mime_view(mime);
    const bool needs_codec_config =
        mime_view == "video/avc" || mime_view == "video/hevc";
    if (needs_codec_config && !has_key(mutable_format, kKeyCsd0)) {
        return {DecoderFormatOutcome::Code::kMissingCodecConfig,
                std::string(mime) + " track has no csd-0 (SPS/PPS); configure would fail later"};
    }

    return {};
}

std::string describe_decoder_format(const AMediaFormat* format) {
    if (format == nullptr) {
        return "<null>";
    }
    const char* text = AMediaFormat_toString(const_cast<AMediaFormat*>(format));
    if (text == nullptr) {
        return "<unprintable>";
    }
    return std::string(text);
}

}  // namespace kivo::video::mediacodec
