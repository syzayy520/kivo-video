// jni_subtitle_bridge.cxx — 212 字幕渲染的 JNI 入口。
//
// 家族树：`jni/` 族，独立于两个播放会话句柄——字幕渲染器与解码会话
// 完全解耦（设计稿 D3：只吃 position_us），句柄自然也是独立的。
//
// 线程契约：LibassRenderer 全状态只在创建/调用线程上使用（211 教训同款）——
// Kotlin 侧由 OverlayController 在固定线程（UI handler）上驱动。
// render 返回 DirectByteBuffer，**内存在下次 render/destroy 前有效**——
// Kotlin 侧必须当帧拷进 Bitmap（约定写在使用方）。

#include <jni.h>

#include <android/log.h>

#include <memory>
#include <string>
#include <vector>

#include "video/subtitle_plane/render/libass_renderer.hpp"

namespace kivo::jni_bridge {

inline constexpr const char* kSubtitleLogTag = "KivoSubtitle";

struct SubtitleHandle {
    std::unique_ptr<kivo::video::subtitle_plane::render::LibassRenderer> renderer;
    std::string last_error;

    void record_error(const char* where, const std::string& message) {
        last_error = std::string(where) + ": " + message;
        __android_log_print(ANDROID_LOG_WARN, kSubtitleLogTag, "%s", last_error.c_str());
    }
};

inline SubtitleHandle* subtitle_handle(jlong raw) noexcept {
    return reinterpret_cast<SubtitleHandle*>(static_cast<std::uintptr_t>(raw));
}

}  // namespace kivo::jni_bridge

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSubtitleCreate(JNIEnv* env,
                                                              jobject /*thiz*/)
    try {
        auto* handle = new kivo::jni_bridge::SubtitleHandle();
        handle->renderer =
            std::make_unique<kivo::video::subtitle_plane::render::LibassRenderer>();
        return reinterpret_cast<jlong>(handle);
    } catch (const std::exception& e) {
        __android_log_print(ANDROID_LOG_WARN, kSubtitleLogTag, "subtitleCreate: %s",
                            e.what());
        return 0;
    } catch (...) {
        return 0;
    }

JNIEXPORT void JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSubtitleDestroy(JNIEnv* env,
                                                               jobject /*thiz*/,
                                                               jlong raw) try {
    delete kivo::jni_bridge::subtitle_handle(raw);
} catch (...) {
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSubtitleAddFont(JNIEnv* env,
                                                               jobject /*thiz*/,
                                                               jlong raw,
                                                               jstring path) try {
    auto* handle = kivo::jni_bridge::subtitle_handle(raw);
    if (handle == nullptr || handle->renderer == nullptr) {
        return 1;
    }
    const char* chars = env->GetStringUTFChars(path, nullptr);
    const std::string font_path = chars != nullptr ? chars : "";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(path, chars);
    }
    std::string error;
    if (!handle->renderer->add_font_file(font_path, error)) {
        handle->record_error("subtitleAddFont", error);
        return 2;
    }
    return 0;
} catch (const std::exception& e) {
    __android_log_print(ANDROID_LOG_WARN, kSubtitleLogTag, "subtitleAddFont: %s", e.what());
    return -1;
} catch (...) {
    return -1;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSubtitleLoad(JNIEnv* env, jobject /*thiz*/,
                                                            jlong raw, jstring path) try {
    auto* handle = kivo::jni_bridge::subtitle_handle(raw);
    if (handle == nullptr || handle->renderer == nullptr) {
        return -1;
    }
    const char* chars = env->GetStringUTFChars(path, nullptr);
    const std::string sub_path = chars != nullptr ? chars : "";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(path, chars);
    }
    std::string error;
    const int events = handle->renderer->load(sub_path, error);
    if (events < 0) {
        handle->record_error("subtitleLoad", error);
    }
    return events;
} catch (const std::exception& e) {
    __android_log_print(ANDROID_LOG_WARN, kSubtitleLogTag, "subtitleLoad: %s", e.what());
    return -1;
} catch (...) {
    return -1;
}

// 渲染一帧。返回 DirectByteBuffer（RGBA，下次渲染前有效），w/h/stride 写进
// `outDims`（[0]=w [1]=h [2]=stride 字节）；无字幕时返回 null。
JNIEXPORT jobject JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSubtitleRender(JNIEnv* env,
                                                              jobject /*thiz*/, jlong raw,
                                                              jint now_ms,
                                                              jintArray out_dims) try {
    auto* handle = kivo::jni_bridge::subtitle_handle(raw);
    if (handle == nullptr || handle->renderer == nullptr) {
        return nullptr;
    }
    auto bitmap = handle->renderer->render(static_cast<int>(now_ms), nullptr);
    if (bitmap.rgba.empty()) {
        return nullptr;
    }
    if (out_dims != nullptr) {
        jint dims[3] = {bitmap.width, bitmap.height, bitmap.width * 4};
        env->SetIntArrayRegion(out_dims, 0, 3, dims);
    }
    return env->NewDirectByteBuffer(bitmap.rgba.data(),
                                    static_cast<jlong>(bitmap.rgba.size()));
} catch (const std::exception& e) {
    __android_log_print(ANDROID_LOG_WARN, kSubtitleLogTag, "subtitleRender: %s", e.what());
    return nullptr;
} catch (...) {
    return nullptr;
}

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSubtitleLastError(JNIEnv* env,
                                                                 jobject /*thiz*/,
                                                                 jlong raw) try {
    auto* handle = kivo::jni_bridge::subtitle_handle(raw);
    if (handle == nullptr) {
        return env->NewStringUTF("handle is null");
    }
    return env->NewStringUTF(handle->last_error.c_str());
} catch (...) {
    return env->NewStringUTF("native exception in subtitleLastError");
}

}  // extern "C"
