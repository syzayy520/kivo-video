// jni_ffmpeg_bridge.cxx — 211 软解兜底的 JNI 入口。
//
// 家族树位置：`android/app/src/main/cpp/jni/`，与 `jni_session_handle` 平行——
// 软解会话的生命周期与硬解 `SessionHandle` 无公共面（内部结构、错误码、
// 证据 schema 都不同），共用句柄只会得到一堆 `if (is_ffmpeg)`。
// 句柄同样不对 Java 暴露任何字段，Kotlin 侧只有一个 Long。
//
// fd 约定与 `session_open` 相同：Java 侧持有 ParcelFileDescriptor，
// native 只借用（会话存活期间 fd 必须有效）；顺序由 Kotlin 的 close() 负责。
// 异常边界复用 `jni_call_guard.hpp` 的函数 try 块（handle 传 nullptr——
// FfmpegSessionHandle 没有last_error 记录面，失败原因走返回码 + lastError）。

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>

#include <cstdint>
#include <memory>
#include <string>

#include "jni_call_guard.hpp"
#include "video/video_plane/runtime/local_media/ffmpeg_local_playback_session.hpp"
#include "video/video_plane/runtime/local_media/ffmpeg_render_sink.hpp"

namespace {

namespace local_media = kivo::video::video_plane::runtime::local_media;

/// 软解会话的 JNI 句柄：窗口 sink + 会话，释放顺序由唯一析构负责。
struct FfmpegSessionHandle {
    std::unique_ptr<local_media::FfmpegWindowSink> sink;
    std::unique_ptr<local_media::FfmpegLocalPlaybackSession> session;
    std::string last_error;

    void record_error(const char* where, const std::string& message) {
        last_error = std::string(where) + ": " + message;
        KIVO_LOGW("%s", last_error.c_str());
    }
};

}  // namespace

inline FfmpegSessionHandle* ffmpeg_handle(jlong raw) noexcept {
    return reinterpret_cast<FfmpegSessionHandle*>(static_cast<std::uintptr_t>(raw));
}

std::string to_std_string(JNIEnv* env, jstring value) {
    if (value == nullptr) {
        return {};
    }
    const char* chars = env->GetStringUTFChars(value, nullptr);
    std::string out = chars != nullptr ? chars : "";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(value, chars);
    }
    return out;
}

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegCreate(JNIEnv* env,
                                                            jobject /*thiz*/) KIVO_JNI_BEGIN
    auto* handle = new FfmpegSessionHandle();
    handle->session = std::make_unique<kivo::video::video_plane::runtime::local_media::
                                           FfmpegLocalPlaybackSession>();
    return reinterpret_cast<jlong>(handle);
KIVO_JNI_END_LONG(env, nullptr, "ffmpegCreate")

JNIEXPORT void JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegDestroy(JNIEnv* env,
                                                             jobject /*thiz*/,
                                                             jlong raw) KIVO_JNI_BEGIN
    auto* handle = ffmpeg_handle(raw);
    if (handle == nullptr) {
        return;
    }
    std::string evidence;  // destroy 时证据丢弃——正常路径先走 nativeFfmpegStop 拿走
    (void)handle->session->stop(evidence);
    handle->sink.reset();
    handle->session.reset();
    delete handle;
KIVO_JNI_END_VOID(env, nullptr, "ffmpegDestroy")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegOpen(JNIEnv* env, jobject /*thiz*/,
                                                          jlong raw, jstring label, jint fd,
                                                          jlong length, jobject surface,
                                                          jstring routing_decision,
                                                          jstring routing_reason)
    KIVO_JNI_BEGIN
    auto* handle = ffmpeg_handle(raw);
    if (handle == nullptr || handle->session == nullptr) {
        return 1;
    }
    if (surface == nullptr) {
        handle->record_error("ffmpegOpen", "surface is null");
        return 2;
    }
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        handle->record_error("ffmpegOpen", "ANativeWindow_fromSurface failed");
        return 2;
    }
    // sink 持有自己的引用（构造里 acquire），JNI 的引用立刻放掉。
    handle->sink = std::make_unique<kivo::video::video_plane::runtime::local_media::
                                        FfmpegWindowSink>(window);
    ANativeWindow_release(window);

    kivo::video::video_plane::runtime::local_media::
        FfmpegLocalPlaybackSession::Config config;
    config.fd = fd;
    config.offset = 0;
    config.length = length;
    config.label = to_std_string(env, label);
    config.routing_decision = to_std_string(env, routing_decision);
    config.routing_reason = to_std_string(env, routing_reason);

    const auto outcome = handle->session->open(config, handle->sink.get());
    if (!outcome.ok()) {
        handle->record_error("ffmpegOpen", outcome.message);
        handle->sink.reset();
        return 3;
    }
    return 0;
KIVO_JNI_END_INT(env, nullptr, "ffmpegOpen")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegStart(JNIEnv* env, jobject /*thiz*/,
                                                           jlong raw) KIVO_JNI_BEGIN
    auto* handle = ffmpeg_handle(raw);
    if (handle == nullptr || handle->session == nullptr) {
        return 1;
    }
    const auto outcome = handle->session->start();
    if (!outcome.ok()) {
        handle->record_error("ffmpegStart", outcome.message);
        return 2;
    }
    return 0;
KIVO_JNI_END_INT(env, nullptr, "ffmpegStart")

JNIEXPORT jboolean JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegRunning(JNIEnv* env,
                                                             jobject /*thiz*/,
                                                             jlong raw) KIVO_JNI_BEGIN
    auto* handle = ffmpeg_handle(raw);
    return static_cast<jboolean>(handle != nullptr && handle->session != nullptr &&
                                 handle->session->is_running());
KIVO_JNI_END(env, nullptr, "ffmpegRunning", jboolean)

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegStop(JNIEnv* env, jobject /*thiz*/,
                                                          jlong raw) KIVO_JNI_BEGIN
    auto* handle = ffmpeg_handle(raw);
    if (handle == nullptr || handle->session == nullptr) {
        return env->NewStringUTF("{}");
    }
    std::string evidence;
    const auto outcome = handle->session->stop(evidence);
    if (!outcome.ok()) {
        handle->record_error("ffmpegStop", outcome.message);
    }
    return env->NewStringUTF(evidence.c_str());
KIVO_JNI_END_STRING(env, nullptr, "ffmpegStop")

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegLastError(JNIEnv* env,
                                                               jobject /*thiz*/,
                                                               jlong raw) KIVO_JNI_BEGIN
    auto* handle = ffmpeg_handle(raw);
    if (handle == nullptr) {
        return env->NewStringUTF("handle is null");
    }
    return env->NewStringUTF(handle->last_error.c_str());
KIVO_JNI_END_STRING(env, nullptr, "ffmpegLastError")

}  // extern "C"
