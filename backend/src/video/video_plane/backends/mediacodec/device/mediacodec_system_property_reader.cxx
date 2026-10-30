#include "video/video_plane/backends/mediacodec/device/mediacodec_system_property_reader.hpp"

#include <sys/system_properties.h>

namespace kivo::video::mediacodec {
namespace {

constexpr const char* kUnknownValue = "unknown";

}  // namespace

std::string read_system_property(const char* name) {
    if (name == nullptr) {
        return kUnknownValue;
    }

    char buffer[PROP_VALUE_MAX] = {};
    const int length = __system_property_get(name, buffer);
    if (length <= 0) {
        return kUnknownValue;
    }
    return std::string(buffer, static_cast<std::size_t>(length));
}

}  // namespace kivo::video::mediacodec
