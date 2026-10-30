#include "video/video_plane/backends/mediacodec/decode/mediacodec_codec_name_resolver.hpp"

#include <dlfcn.h>

namespace kivo::video::mediacodec {
namespace {

using GetNameFn = media_status_t (*)(AMediaCodec*, char**);
using ReleaseNameFn = void (*)(AMediaCodec*, char*);

/// 进程内只需解析一次。dlsym 本身线程安全，但重复调用是浪费。
struct NameApi {
    GetNameFn get_name = nullptr;
    ReleaseNameFn release_name = nullptr;
};

[[nodiscard]] const NameApi& name_api() {
    static const NameApi api = [] {
        NameApi resolved;
        resolved.get_name =
            reinterpret_cast<GetNameFn>(dlsym(RTLD_DEFAULT, "AMediaCodec_getName"));
        resolved.release_name =
            reinterpret_cast<ReleaseNameFn>(dlsym(RTLD_DEFAULT, "AMediaCodec_releaseName"));
        return resolved;
    }();
    return api;
}

}  // namespace

std::string resolve_codec_name(AMediaCodec* codec) {
    if (codec == nullptr) {
        return {};
    }

    const NameApi& api = name_api();
    if (api.get_name == nullptr || api.release_name == nullptr) {
        // API 26/27：符号不存在。返回空串是契约，不是失败。
        return {};
    }

    char* raw = nullptr;
    if (api.get_name(codec, &raw) != AMEDIA_OK || raw == nullptr) {
        return {};
    }

    std::string name(raw);
    api.release_name(codec, raw);
    return name;
}

}  // namespace kivo::video::mediacodec
