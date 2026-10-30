#include "video/video_plane/backends/aaudio/device/aaudio_format_names.hpp"

#include <aaudio/AAudio.h>

namespace kivo::video::aaudio {
namespace {

// AAudio 的枚举在 NDK 里是 enum，值互不重复，可以直接 switch。
// 每个函数都保留 default → "unknown"：新 NDK 加了新值时不会编译失败，
// 而且证据里一旦出现 unknown 就能立刻被发现，不至于被静默归错类。
//
// 这份清单是照着 r30 的 aaudio/AAudio.h 逐个抄下来的，不是凭印象写的。
// 早前凭印象写过 AAUDIO_FORMAT_PCM_32_BIT 之类根本不存在的名字，编译期就炸。
constexpr std::string_view kUnknown = "unknown";

}  // namespace

std::string_view platform_format_name(std::int32_t format) {
    switch (format) {
        case AAUDIO_FORMAT_INVALID: return "invalid";
        case AAUDIO_FORMAT_UNSPECIFIED: return "unspecified";
        case AAUDIO_FORMAT_PCM_I16: return "pcm_i16";
        case AAUDIO_FORMAT_PCM_FLOAT: return "pcm_float";
        case AAUDIO_FORMAT_PCM_I24_PACKED: return "pcm_i24_packed";
        case AAUDIO_FORMAT_PCM_I32: return "pcm_i32";
        case AAUDIO_FORMAT_IEC61937: return "iec61937";
        case AAUDIO_FORMAT_MP3: return "mp3";
        case AAUDIO_FORMAT_AAC_LC: return "aac_lc";
        case AAUDIO_FORMAT_AAC_HE_V1: return "aac_he_v1";
        case AAUDIO_FORMAT_AAC_HE_V2: return "aac_he_v2";
        case AAUDIO_FORMAT_AAC_ELD: return "aac_eld";
        case AAUDIO_FORMAT_AAC_XHE: return "aac_xhe";
        case AAUDIO_FORMAT_OPUS: return "opus";
        default: return kUnknown;
    }
}

std::string_view sharing_mode_name(std::int32_t mode) {
    switch (mode) {
        case AAUDIO_SHARING_MODE_EXCLUSIVE: return "exclusive";
        case AAUDIO_SHARING_MODE_SHARED: return "shared";
        default: return kUnknown;
    }
}

std::string_view performance_mode_name(std::int32_t mode) {
    switch (mode) {
        case AAUDIO_PERFORMANCE_MODE_NONE: return "none";
        case AAUDIO_PERFORMANCE_MODE_LOW_LATENCY: return "low_latency";
        case AAUDIO_PERFORMANCE_MODE_POWER_SAVING: return "power_saving";
        default: return kUnknown;
    }
}

std::string_view result_name(std::int32_t result) {
    switch (result) {
        case AAUDIO_OK: return "ok";
        case AAUDIO_ERROR_DISCONNECTED: return "disconnected";
        case AAUDIO_ERROR_ILLEGAL_ARGUMENT: return "illegal_argument";
        case AAUDIO_ERROR_INTERNAL: return "internal";
        case AAUDIO_ERROR_INVALID_STATE: return "invalid_state";
        case AAUDIO_ERROR_INVALID_HANDLE: return "invalid_handle";
        case AAUDIO_ERROR_UNIMPLEMENTED: return "unimplemented";
        case AAUDIO_ERROR_UNAVAILABLE: return "unavailable";
        case AAUDIO_ERROR_NO_FREE_HANDLES: return "no_free_handles";
        case AAUDIO_ERROR_NO_MEMORY: return "no_memory";
        case AAUDIO_ERROR_NULL: return "null";
        case AAUDIO_ERROR_TIMEOUT: return "timeout";
        case AAUDIO_ERROR_WOULD_BLOCK: return "would_block";
        case AAUDIO_ERROR_INVALID_FORMAT: return "invalid_format";
        case AAUDIO_ERROR_OUT_OF_RANGE: return "out_of_range";
        case AAUDIO_ERROR_NO_SERVICE: return "no_service";
        case AAUDIO_ERROR_INVALID_RATE: return "invalid_rate";
        default: return kUnknown;
    }
}

}  // namespace kivo::video::aaudio
