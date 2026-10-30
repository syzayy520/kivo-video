#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_implementation_class.hpp"

namespace kivo::video::mediacodec {
namespace {

constexpr std::string_view kSoftware = "software";
constexpr std::string_view kVendor = "vendor";
constexpr std::string_view kUnknown = "unknown";

}  // namespace

std::string_view classify_implementation(std::string_view codec_name) {
    const auto contains = [codec_name](std::string_view needle) {
        return codec_name.find(needle) != std::string_view::npos;
    };

    if (contains("c2.android.") || contains("google") || contains(".sw") ||
        contains("software")) {
        return kSoftware;
    }
    if (contains("qti") || contains("qcom") || contains("exynos") || contains("mtk") ||
        contains("kirin") || contains("hisi")) {
        return kVendor;
    }
    return kUnknown;
}

}  // namespace kivo::video::mediacodec
