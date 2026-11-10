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
#include "video/video_plane/backends/avsync/session/soft_audio_session.hpp"
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

/// 软解音频的独立句柄。
///
/// 为什么不做进上面那个：视频句柄持有一个 ANativeWindow，音频没有窗口；
/// 两者的 open 参数、失败码、销毁顺序都不一样，合成一个只会到处写 `if`。
struct SoftAudioHandle {
    std::unique_ptr<kivo::video::avsync::SoftAudioSession> session;
    std::string last_error;
};

}  // namespace

inline FfmpegSessionHandle* ffmpeg_handle(jlong raw) noexcept {
    return reinterpret_cast<FfmpegSessionHandle*>(static_cast<std::uintptr_t>(raw));
}

inline SoftAudioHandle* soft_audio_handle(jlong raw) noexcept {
    return reinterpret_cast<SoftAudioHandle*>(static_cast<std::uintptr_t>(raw));
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
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegOpenUrl(JNIEnv* env, jobject /*thiz*/,
                                                             jlong raw, jstring label,
                                                             jstring url, jobject surface,
                                                             jstring routing_decision,
                                                             jstring routing_reason)
    KIVO_JNI_BEGIN
    auto* handle = ffmpeg_handle(raw);
    if (handle == nullptr || handle->session == nullptr) {
        return 1;
    }
    if (surface == nullptr) {
        handle->record_error("ffmpegOpenUrl", "surface is null");
        return 2;
    }
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        handle->record_error("ffmpegOpenUrl", "ANativeWindow_fromSurface failed");
        return 2;
    }
    handle->sink = std::make_unique<kivo::video::video_plane::runtime::local_media::
                                        FfmpegWindowSink>(window);
    ANativeWindow_release(window);

    kivo::video::video_plane::runtime::local_media::
        FfmpegLocalPlaybackSession::Config config;
    config.url = to_std_string(env, url);
    config.label = to_std_string(env, label);
    config.routing_decision = to_std_string(env, routing_decision);
    config.routing_reason = to_std_string(env, routing_reason);

    const auto outcome = handle->session->open(config, handle->sink.get());
    if (!outcome.ok()) {
        handle->record_error("ffmpegOpenUrl", outcome.message);
        handle->sink.reset();
        return 3;
    }
    return 0;
KIVO_JNI_END_INT(env, nullptr, "nativeFfmpegOpenUrl")

JNIEXPORT void JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegSeekTo(JNIEnv* env, jobject /*thiz*/,
                                                            jlong raw, jlong targetUs)
    KIVO_JNI_BEGIN
    auto* handle = ffmpeg_handle(raw);
    if (handle == nullptr || handle->session == nullptr) return;
    (void)handle->session->seekTo(static_cast<std::int64_t>(targetUs));
    KIVO_JNI_END_VOID(env, nullptr, "nativeFfmpegSeekTo")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegSetPaused(JNIEnv* env, jobject /*thiz*/,
                                                               jlong raw, jboolean paused)
    KIVO_JNI_BEGIN
    auto* handle = ffmpeg_handle(raw);
    if (handle == nullptr || handle->session == nullptr) {
        return 1;
    }
    const auto outcome = handle->session->setPaused(paused == JNI_TRUE);
    if (!outcome.ok()) {
        handle->record_error("ffmpegSetPaused", outcome.message);
        return 2;
    }
    return 0;
KIVO_JNI_END_INT(env, nullptr, "ffmpegSetPaused")

JNIEXPORT void JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegSetAudioClockUs(JNIEnv* env,
                                                                     jobject /*thiz*/,
                                                                     jlong raw, jlong clockUs)
    KIVO_JNI_BEGIN
    auto* handle = ffmpeg_handle(raw);
    if (handle == nullptr || handle->session == nullptr) return;
    handle->session->setAudioClockUs(static_cast<std::int64_t>(clockUs));
    KIVO_JNI_END_VOID(env, nullptr, "nativeFfmpegSetAudioClockUs")

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegPositionUs(JNIEnv* env, jobject /*thiz*/,
                                                                jlong raw)
    KIVO_JNI_BEGIN
    auto* handle = ffmpeg_handle(raw);
    if (handle == nullptr || handle->session == nullptr) return -1;
    return static_cast<jlong>(handle->session->positionUs());
    KIVO_JNI_END_LONG(env, nullptr, "nativeFfmpegPositionUs")

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeFfmpegDurationUs(JNIEnv* env, jobject /*thiz*/,
                                                                jlong raw)
    KIVO_JNI_BEGIN
    auto* handle = ffmpeg_handle(raw);
    if (handle == nullptr || handle->session == nullptr) return -1;
    return static_cast<jlong>(handle->session->durationUs());
    KIVO_JNI_END_LONG(env, nullptr, "nativeFfmpegDurationUs")


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

// ---- 软解音频：与上面的视频会话平行，句柄互不相通 ----

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioCreate(JNIEnv* env,
                                                               jobject /*thiz*/) KIVO_JNI_BEGIN
    auto* handle = new SoftAudioHandle();
    handle->session = std::make_unique<kivo::video::avsync::SoftAudioSession>();
    return reinterpret_cast<jlong>(handle);
KIVO_JNI_END_LONG(env, nullptr, "softAudioCreate")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioOpen(JNIEnv* env, jobject /*thiz*/,
                                                             jlong raw, jstring label, jint fd,
                                                             jlong length) KIVO_JNI_BEGIN
    auto* handle = soft_audio_handle(raw);
    if (handle == nullptr || handle->session == nullptr) {
        return 1;
    }
    if (!handle->session->open(fd, length, to_std_string(env, label))) {
        handle->last_error = handle->session->stats().error;
        KIVO_LOGW("softAudioOpen: %s", handle->last_error.c_str());
        return 2;
    }
    return 0;
KIVO_JNI_END_INT(env, nullptr, "softAudioOpen")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioStart(JNIEnv* env, jobject /*thiz*/,
                                                              jlong raw) KIVO_JNI_BEGIN
    auto* handle = soft_audio_handle(raw);
    if (handle == nullptr || handle->session == nullptr) {
        return 1;
    }
    if (!handle->session->start()) {
        handle->last_error = handle->session->stats().error;
        return 2;
    }
    return 0;
KIVO_JNI_END_INT(env, nullptr, "softAudioStart")

JNIEXPORT void JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioSetPaused(JNIEnv* env, jobject /*thiz*/,
                                                                  jlong raw, jboolean paused)
    KIVO_JNI_BEGIN
    auto* handle = soft_audio_handle(raw);
    if (handle != nullptr && handle->session != nullptr) {
        handle->session->setPaused(paused == JNI_TRUE);
    }
KIVO_JNI_END_VOID(env, nullptr, "softAudioSetPaused")

JNIEXPORT void JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioSeekTo(JNIEnv* env, jobject /*thiz*/,
                                                               jlong raw, jlong target_us)
    KIVO_JNI_BEGIN
    auto* handle = soft_audio_handle(raw);
    if (handle != nullptr && handle->session != nullptr) {
        handle->session->seekTo(static_cast<std::int64_t>(target_us));
    }
KIVO_JNI_END_VOID(env, nullptr, "softAudioSeekTo")

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioPositionUs(JNIEnv* env, jobject /*thiz*/,
                                                                   jlong raw) KIVO_JNI_BEGIN
    auto* handle = soft_audio_handle(raw);
    if (handle == nullptr || handle->session == nullptr) return -1;
    return static_cast<jlong>(handle->session->positionUs());
KIVO_JNI_END_LONG(env, nullptr, "softAudioPositionUs")

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioPlayedUs(JNIEnv* env, jobject /*thiz*/,
                                                                jlong raw) KIVO_JNI_BEGIN
    auto* handle = soft_audio_handle(raw);
    if (handle == nullptr || handle->session == nullptr) return -1;
    return static_cast<jlong>(handle->session->playedUs());
KIVO_JNI_END_LONG(env, nullptr, "softAudioPlayedUs")

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioDurationUs(JNIEnv* env, jobject /*thiz*/,
                                                                   jlong raw) KIVO_JNI_BEGIN
    auto* handle = soft_audio_handle(raw);
    if (handle == nullptr || handle->session == nullptr) return -1;
    return static_cast<jlong>(handle->session->durationUs());
KIVO_JNI_END_LONG(env, nullptr, "softAudioDurationUs")

JNIEXPORT jboolean JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioRunning(JNIEnv* env, jobject /*thiz*/,
                                                                jlong raw) KIVO_JNI_BEGIN
    auto* handle = soft_audio_handle(raw);
    return static_cast<jboolean>(handle != nullptr && handle->session != nullptr &&
                                 handle->session->isRunning());
KIVO_JNI_END(env, nullptr, "softAudioRunning", jboolean)

JNIEXPORT jboolean JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioHasOutput(JNIEnv* env, jobject /*thiz*/,
                                                                  jlong raw) KIVO_JNI_BEGIN
    auto* handle = soft_audio_handle(raw);
    return static_cast<jboolean>(handle != nullptr && handle->session != nullptr &&
                                 handle->session->hasOutput());
KIVO_JNI_END(env, nullptr, "softAudioHasOutput", jboolean)

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioStats(JNIEnv* env, jobject /*thiz*/,
                                                              jlong raw) KIVO_JNI_BEGIN
    auto* handle = soft_audio_handle(raw);
    if (handle == nullptr || handle->session == nullptr) {
        return env->NewStringUTF("{}");
    }
    const auto& s = handle->session->stats();
    std::string json = "{\"codec\":\"" + s.codec + "\",\"container\":\"" + s.container +
                       "\",\"packets_read\":" + std::to_string(s.packets_read) +
                       ",\"frames_written\":" + std::to_string(s.frames_written) +
                       ",\"ring_full_waits\":" + std::to_string(s.ring_full_waits) +
                       ",\"read_errors\":" + std::to_string(s.read_errors) +
                       ",\"src_rate\":" + std::to_string(s.source_sample_rate) +
                       ",\"src_channels\":" + std::to_string(s.source_channels) +
                       ",\"out_rate\":" + std::to_string(s.output_sample_rate) +
                       ",\"eos\":" + (s.reached_eos ? "true" : "false") + ",\"error\":\"" +
                       s.error + "\"}";
    return env->NewStringUTF(json.c_str());
KIVO_JNI_END_STRING(env, nullptr, "softAudioStats")

JNIEXPORT void JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioStop(JNIEnv* env, jobject /*thiz*/,
                                                             jlong raw) KIVO_JNI_BEGIN
    auto* handle = soft_audio_handle(raw);
    if (handle != nullptr && handle->session != nullptr) {
        handle->session->stop();
    }
KIVO_JNI_END_VOID(env, nullptr, "softAudioStop")

JNIEXPORT void JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSoftAudioDestroy(JNIEnv* env, jobject /*thiz*/,
                                                                jlong raw) KIVO_JNI_BEGIN
    auto* handle = soft_audio_handle(raw);
    if (handle == nullptr) {
        return;
    }
    if (handle->session != nullptr) {
        handle->session->stop();
        handle->session.reset();
    }
    delete handle;
KIVO_JNI_END_VOID(env, nullptr, "softAudioDestroy")

}  // extern "C"
