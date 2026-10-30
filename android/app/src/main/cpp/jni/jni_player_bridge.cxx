#include "jni_session_handle.hpp"
#include "jni_string_convert.hpp"

#include "video/video_plane/backends/mediacodec/device/mediacodec_capability_probe.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_framerate_capability.hpp"

#include <android/log.h>
#include <jni.h>

#include <string>
#include <vector>

namespace {

constexpr const char* kLogTag = "KivoCinema";
constexpr const char* kBackendVersion = "p12b/0.2.0";

#define KIVO_LOGI(...) __android_log_print(ANDROID_LOG_INFO, kLogTag, __VA_ARGS__)
#define KIVO_LOGW(...) __android_log_print(ANDROID_LOG_WARN, kLogTag, __VA_ARGS__)

[[nodiscard]] std::vector<std::string> to_string_vector(JNIEnv* env, jobjectArray array) {
    std::vector<std::string> out;
    if (array == nullptr) {
        return out;
    }
    const jsize length = env->GetArrayLength(array);
    out.reserve(static_cast<std::size_t>(length));
    for (jsize index = 0; index < length; ++index) {
        auto element = static_cast<jstring>(env->GetObjectArrayElement(array, index));
        if (element == nullptr) {
            continue;
        }
        out.push_back(kivo::jni_bridge::to_utf8(env, element));
        env->DeleteLocalRef(element);
    }
    return out;
}

}  // namespace

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeVersion(JNIEnv* env, jobject /*thiz*/) {
    return kivo::jni_bridge::to_jstring(env, kBackendVersion);
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeProbeCapabilities(
    JNIEnv* env, jobject /*thiz*/, jstring output_json_path, jobjectArray inventory) {
    const std::string path = kivo::jni_bridge::to_utf8(env, output_json_path);
    const std::vector<std::string> records = to_string_vector(env, inventory);

    KIVO_LOGI("probe start: records=%zu path=%s", records.size(), path.c_str());

    const auto outcome = kivo::video::mediacodec::probe_capabilities(records, path);

    if (outcome.code != 0) {
        KIVO_LOGW("probe failed: code=%d message=%s", outcome.code, outcome.message.c_str());
        // 语义契约：失败必须带非空 reason（G9）
        return static_cast<jint>(outcome.code);
    }

    KIVO_LOGI("probe ok: decoders=%zu hardware=%zu tenbit=%zu",
              outcome.decoder_records,
              outcome.hardware_decoder_records,
              outcome.decoder_10bit_records);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeProbeFramerates(
    JNIEnv* env, jobject /*thiz*/, jstring output_json_path, jobjectArray records) {
    const std::string path = kivo::jni_bridge::to_utf8(env, output_json_path);
    const std::vector<std::string> lines = to_string_vector(env, records);

    KIVO_LOGI("framerate probe start: records=%zu path=%s", lines.size(), path.c_str());

    const auto outcome = kivo::video::mediacodec::probe_framerates(path, lines);

    if (outcome.code != 0) {
        KIVO_LOGW("framerate probe failed: code=%d message=%s", outcome.code,
                  outcome.message.c_str());
        // 语义契约：失败必须带非空 reason（G9）—— 返回码之外还要有话说。
        return static_cast<jint>(outcome.code);
    }

    KIVO_LOGI("framerate probe ok: parsed=%zu rejected=%zu", outcome.parsed_records,
              outcome.rejected_records);
    return 0;
}

// ---- 解码会话 ------------------------------------------------------------
// 句柄是不透明指针，Kotlin 侧只有一个 Long。释放顺序由 jni_session_handle 统一负责。

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionCreate(JNIEnv* /*env*/,
                                                             jobject /*thiz*/) {
    auto* handle = kivo::jni_bridge::session_create();
    KIVO_LOGI("session create: %p", static_cast<void*>(handle));
    return reinterpret_cast<jlong>(handle);
}

JNIEXPORT void JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionDestroy(JNIEnv* /*env*/, jobject /*thiz*/,
                                                              jlong handle) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    if (session != nullptr) {
        kivo::jni_bridge::session_destroy(session);
        KIVO_LOGI("session destroyed");
    }
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionOpen(JNIEnv* env, jobject /*thiz*/,
                                                           jlong handle, jstring label, jint fd,
                                                           jlong length, jobject surface) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    const std::string media_label = kivo::jni_bridge::to_utf8(env, label);

    std::string error;
    const int code = kivo::jni_bridge::session_open(
        session, env, surface, static_cast<int>(fd), static_cast<std::int64_t>(length),
        media_label, error);
    if (code != 0) {
        KIVO_LOGW("session open failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("session open ok: %s (fd=%d bytes=%lld)", media_label.c_str(),
              static_cast<int>(fd), static_cast<long long>(length));
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionStart(JNIEnv* /*env*/, jobject /*thiz*/,
                                                            jlong handle) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    std::string error;
    const int code = kivo::jni_bridge::session_start(session, error);
    if (code != 0) {
        KIVO_LOGW("session start failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("session start ok");
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionSetPacing(JNIEnv* /*env*/, jobject /*thiz*/,
                                                               jlong handle, jint target_fps,
                                                               jboolean allow_drop,
                                                               jint clock_offset_ms,
                                                               jint clock_offset_at_ms,
                                                               jint clock_rate_ppm,
                                                               jboolean drift_gate) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    std::string error;
    const int code = kivo::jni_bridge::session_set_pacing(
        session, static_cast<int>(target_fps), allow_drop == JNI_TRUE,
        static_cast<int>(clock_offset_ms), static_cast<int>(clock_offset_at_ms),
        static_cast<int>(clock_rate_ppm), drift_gate == JNI_TRUE, error);
    if (code != 0) {
        KIVO_LOGW("session pacing failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("session pacing on: target_fps=%d allow_drop=%d clock_offset=%dms@%dms "
              "rate=%dppm gate=%d",
              static_cast<int>(target_fps), allow_drop == JNI_TRUE ? 1 : 0,
              static_cast<int>(clock_offset_ms), static_cast<int>(clock_offset_at_ms),
              static_cast<int>(clock_rate_ppm), drift_gate == JNI_TRUE ? 1 : 0);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionSetPipeline(JNIEnv* /*env*/,
                                                                 jobject /*thiz*/, jlong handle,
                                                                 jboolean split,
                                                                 jboolean present,
                                                                 jboolean at_time) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    std::string error;
    const int code = kivo::jni_bridge::session_set_pipeline(
        session, split == JNI_TRUE, present == JNI_TRUE, at_time == JNI_TRUE, error);
    if (code != 0) {
        KIVO_LOGW("session pipeline failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("session pipeline: %s present=%d schedule=%s", split == JNI_TRUE ? "split" : "merged",
              present == JNI_TRUE ? 1 : 0, at_time == JNI_TRUE ? "surface_at_time" : "cpu_paced");
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionSeek(JNIEnv* /*env*/, jobject /*thiz*/,
                                                           jlong handle, jlong target_us) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    std::string error;
    const int code =
        kivo::jni_bridge::session_seek(session, static_cast<std::int64_t>(target_us), error);
    if (code != 0) {
        KIVO_LOGW("session seek failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("session seek ok: target=%lld us", static_cast<long long>(target_us));
    return 0;
}

JNIEXPORT void JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionStop(JNIEnv* /*env*/, jobject /*thiz*/,
                                                           jlong handle) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    kivo::jni_bridge::session_stop(session);
    KIVO_LOGI("session stopped: %s", kivo::jni_bridge::session_summary(session).c_str());
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionRebindSurface(JNIEnv* env, jobject /*thiz*/,
                                                                   jlong handle, jstring label,
                                                                   jobject surface) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    const std::string media_label = kivo::jni_bridge::to_utf8(env, label);

    std::string error;
    const int code = kivo::jni_bridge::session_rebind_window(session, env, surface, error);
    if (code != 0) {
        KIVO_LOGW("session rebind failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("session rebind ok: %s surface=%s", media_label.c_str(),
              surface == nullptr ? "detached" : "attached");
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionOpenAudio(JNIEnv* env, jobject /*thiz*/,
                                                               jlong handle, jstring label,
                                                               jint fd, jlong length) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    const std::string audio_label = kivo::jni_bridge::to_utf8(env, label);

    std::string error;
    const int code = kivo::jni_bridge::session_open_audio(
        session, static_cast<int>(fd), static_cast<std::int64_t>(length), audio_label, error);
    if (code != 0) {
        KIVO_LOGW("session audio failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("session audio ok: %s (fd=%d bytes=%lld)", audio_label.c_str(),
              static_cast<int>(fd), static_cast<long long>(length));
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionWriteSyncStats(JNIEnv* env, jobject /*thiz*/,
                                                                    jlong handle, jstring path) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    const std::string stats_path = kivo::jni_bridge::to_utf8(env, path);

    std::string error;
    const int code = kivo::jni_bridge::session_write_sync_stats(session, stats_path, error);
    if (code != 0) {
        KIVO_LOGW("write sync stats failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("write sync stats ok: %s", stats_path.c_str());
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionWriteStats(JNIEnv* env, jobject /*thiz*/,
                                                                 jlong handle, jstring path) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    const std::string stats_path = kivo::jni_bridge::to_utf8(env, path);

    std::string error;
    const int code = kivo::jni_bridge::session_write_stats(session, stats_path, error);
    if (code != 0) {
        KIVO_LOGW("write stats failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("write stats ok: %s", stats_path.c_str());
    return 0;
}

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionSummary(JNIEnv* env, jobject /*thiz*/,
                                                              jlong handle) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    return kivo::jni_bridge::to_jstring(env, kivo::jni_bridge::session_summary(session));
}

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionLastError(JNIEnv* env, jobject /*thiz*/,
                                                                jlong handle) {
    auto* session = reinterpret_cast<kivo::jni_bridge::SessionHandle*>(handle);
    return kivo::jni_bridge::to_jstring(env, kivo::jni_bridge::session_last_error(session));
}

}  // extern "C"
