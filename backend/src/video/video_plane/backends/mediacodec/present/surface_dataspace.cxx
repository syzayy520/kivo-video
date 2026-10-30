#include "video/video_plane/backends/mediacodec/present/surface_dataspace.hpp"

#include <android/data_space.h>
#include <android/native_window.h>
#include <dlfcn.h>

namespace kivo::video::mediacodec {
namespace {

/// `ANativeWindow_setBuffersDataSpace` 的函数指针类型。
/// 用 dlsym 而不是直接调用：该符号 API 28 才有，而 minSdk = 26。
/// 直接调在 26/27 上会在加载期解析失败 —— 那是"一装就崩"，不是"少个功能"。
using SetBuffersDataSpaceFn = std::int32_t (*)(ANativeWindow*, std::int32_t);

struct ResolvedSymbol {
    SetBuffersDataSpaceFn fn = nullptr;
    std::string error;  ///< 解析失败的原因；成功时为空
};

/// 解析一次并缓存。
///
/// **Android 13 实测更正（2026-10-30，P12-E2）**：设计稿写的是
/// `dlsym(RTLD_DEFAULT, ...)`，实际在 K40 上返回 null —— 因为这个符号
/// **不在 `libandroid.so` 里**，它在 `libnativewindow.so` 里：
///
/// ```
/// $ strings /system/lib64/libandroid.so | grep -c ANativeWindow_setBuffersDataSpace
/// 0
/// $ strings /system/lib64/libnativewindow.so | grep -c ANativeWindow_setBuffersDataSpace
/// 1
/// ```
///
/// 而 `RTLD_DEFAULT` 只搜**已加载的**全局命名空间；工程没有链接 `nativewindow`，
/// 自然搜不到。所以先 `dlopen` 拿句柄再按句柄查，`RTLD_DEFAULT` 只作兜底
/// （万一某个平台把它导出在 libandroid 里）。
///
/// 这次错误的代价是：第一轮真机证据里 `dataspace.applied=false` +
/// reason 写着"API < 28"，而设备是 API 33 —— 一句错话比没有话更糟。
[[nodiscard]] ResolvedSymbol resolve_set_buffers_data_space() {
    ResolvedSymbol out;
    if (void* handle = dlopen("libnativewindow.so", RTLD_NOW | RTLD_LOCAL)) {
        if (auto* symbol = dlsym(handle, "ANativeWindow_setBuffersDataSpace")) {
            out.fn = reinterpret_cast<SetBuffersDataSpaceFn>(symbol);
            return out;
        }
        out.error = "libnativewindow.so is loaded but does not export "
                    "ANativeWindow_setBuffersDataSpace";
    } else {
        const char* why = dlerror();
        out.error = std::string("dlopen(libnativewindow.so) failed: ") +
                    (why != nullptr ? why : "unknown reason");
    }

    if (auto* symbol = dlsym(RTLD_DEFAULT, "ANativeWindow_setBuffersDataSpace")) {
        out.fn = reinterpret_cast<SetBuffersDataSpaceFn>(symbol);
        out.error.clear();
        return out;
    }
    if (out.error.empty()) {
        out.error = "ANativeWindow_setBuffersDataSpace is not available in the default namespace";
    }
    return out;
}

[[nodiscard]] const ResolvedSymbol& resolved_symbol() {
    static const ResolvedSymbol resolved = resolve_set_buffers_data_space();
    return resolved;
}

/// `ANativeWindow_getBuffersDataSpace`（API 29）。解析方式与 set 完全相同，
/// 因为两个符号在同一个库里；写成两个函数而不是模板，是为了让失败原因各自独立。
using GetBuffersDataSpaceFn = std::int32_t (*)(ANativeWindow*);

struct ResolvedGetter {
    GetBuffersDataSpaceFn fn = nullptr;
    std::string error;
};

[[nodiscard]] ResolvedGetter resolve_get_buffers_data_space() {
    ResolvedGetter out;
    if (void* handle = dlopen("libnativewindow.so", RTLD_NOW | RTLD_LOCAL)) {
        if (auto* symbol = dlsym(handle, "ANativeWindow_getBuffersDataSpace")) {
            out.fn = reinterpret_cast<GetBuffersDataSpaceFn>(symbol);
            return out;
        }
        out.error = "libnativewindow.so is loaded but does not export "
                    "ANativeWindow_getBuffersDataSpace";
    } else {
        const char* why = dlerror();
        out.error = std::string("dlopen(libnativewindow.so) failed: ") +
                    (why != nullptr ? why : "unknown reason");
    }
    if (auto* symbol = dlsym(RTLD_DEFAULT, "ANativeWindow_getBuffersDataSpace")) {
        out.fn = reinterpret_cast<GetBuffersDataSpaceFn>(symbol);
        out.error.clear();
        return out;
    }
    if (out.error.empty()) {
        out.error = "ANativeWindow_getBuffersDataSpace is not available in the default namespace";
    }
    return out;
}

[[nodiscard]] const ResolvedGetter& resolved_getter() {
    static const ResolvedGetter resolved = resolve_get_buffers_data_space();
    return resolved;
}

}  // namespace

DataSpaceOutcome apply_surface_dataspace(ANativeWindow* window, HdrKind kind) {
    DataSpaceOutcome out;
    if (kind == HdrKind::kSdr) {
        out.reason = "source is SDR; its dataspace is left untouched on purpose";
        return out;
    }
    if (window == nullptr) {
        out.reason = "surface window is null";
        return out;
    }

    const std::int32_t data_space =
        kind == HdrKind::kHdr10 ? ADATASPACE_BT2020_PQ : ADATASPACE_BT2020_HLG;

    const SetBuffersDataSpaceFn fn = resolved_symbol().fn;
    if (fn == nullptr) {
        // 原因照抄解析阶段记下的那一句：它包含"试过哪个库、为什么没成"，
        // 比一句笼统的"不支持"有用得多。
        out.reason = resolved_symbol().error;
        return out;
    }

    const std::int32_t status = fn(window, data_space);
    if (status != 0) {
        // 非 0 是 errno 风格的值。这里不解释具体含义，只把值带出去 —— 解释要查平台源码，
        // 写错比不写更糟。
        out.reason = "ANativeWindow_setBuffersDataSpace failed with " + std::to_string(status);
        return out;
    }

    out.applied = true;
    out.value = data_space;
    return out;
}

DataSpaceReadback read_surface_dataspace(ANativeWindow* window) {
    DataSpaceReadback out;
    if (window == nullptr) {
        out.reason = "surface window is null";
        return out;
    }
    const GetBuffersDataSpaceFn fn = resolved_getter().fn;
    if (fn == nullptr) {
        out.reason = resolved_getter().error;
        return out;
    }
    const std::int32_t value = fn(window);
    if (value < 0) {
        // 负值只可能是错误码。不解释具体含义（与 set 的处置一致：写错比不写更糟）。
        out.reason = "ANativeWindow_getBuffersDataSpace returned " + std::to_string(value);
        return out;
    }
    out.ok = true;
    out.value = value;
    return out;
}

}  // namespace kivo::video::mediacodec
