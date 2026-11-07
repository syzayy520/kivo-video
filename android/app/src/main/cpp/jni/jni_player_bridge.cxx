// Java NativePlayer 的全部 native 实现（家族树 §四）。
//
// P16-P0 / D1：每个导出都是**函数 try 块**，一对边界宏贴在函数签名行末与结尾，
// 原函数体逐字不变、行数不增。边界定义见 `jni_call_guard.hpp`，
// 字符串转换见 `jni_string_convert.hpp`。本文件不含任何解码/渲染逻辑。

#include "jni_call_guard.hpp"
#include "jni_string_convert.hpp"

#include "video/video_plane/backends/mediacodec/device/mediacodec_capability_probe.hpp"
#include "video/video_plane/backends/mediacodec/device/mediacodec_framerate_capability.hpp"

#include <string>
#include <vector>

namespace {

constexpr const char* kBackendVersion = "p12b/0.2.0";

}  // namespace

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeVersion(JNIEnv* env, jobject /*thiz*/) KIVO_JNI_BEGIN
    return kivo::jni_bridge::to_jstring(env, kBackendVersion);
    KIVO_JNI_END_STRING(env, nullptr, "nativeVersion")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeProbeCapabilities(
    JNIEnv* env, jobject /*thiz*/, jstring output_json_path, jobjectArray inventory) KIVO_JNI_BEGIN
    const std::string path = kivo::jni_bridge::to_utf8(env, output_json_path);
    const std::vector<std::string> records =
        kivo::jni_bridge::to_string_vector(env, inventory);

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
    KIVO_JNI_END_INT(env, nullptr, "nativeProbeCapabilities")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeProbeFramerates(
    JNIEnv* env, jobject /*thiz*/, jstring output_json_path, jobjectArray records) KIVO_JNI_BEGIN
    const std::string path = kivo::jni_bridge::to_utf8(env, output_json_path);
    const std::vector<std::string> lines = kivo::jni_bridge::to_string_vector(env, records);

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
    KIVO_JNI_END_INT(env, nullptr, "nativeProbeFramerates")

// ---- 解码会话 ------------------------------------------------------------
// 句柄是不透明指针，Kotlin 侧只有一个 Long。释放顺序由 jni_session_handle 统一负责。

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionCreate(JNIEnv* env,
                                                             jobject /*thiz*/) KIVO_JNI_BEGIN
    // session_create() 内部会 new，同样在函数 try 块里：抛 bad_alloc 也不越过 JNI 帧。
    auto* handle = kivo::jni_bridge::session_create();
    KIVO_LOGI("session create: %p", static_cast<void*>(handle));
    return reinterpret_cast<jlong>(handle);
    KIVO_JNI_END_LONG(env, nullptr, "nativeSessionCreate")

JNIEXPORT void JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionDestroy(JNIEnv* env, jobject /*thiz*/,
                                                              jlong handle) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
    if (session != nullptr) {
        kivo::jni_bridge::session_destroy(session);
        KIVO_LOGI("session destroyed");
    }
    // 传 nullptr 而不是 session：若 session_destroy 内部抛了，句柄可能已经释放，
    // 往 last_error 里写就是 UAF。原因由 KIVO_LOGW 留下即可。
    KIVO_JNI_END_VOID(env, nullptr, "nativeSessionDestroy")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionOpen(JNIEnv* env, jobject /*thiz*/,
                                                           jlong handle, jstring label, jint fd,
                                                           jlong length, jobject surface) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
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
    KIVO_JNI_END_INT(env, KIVO_JNI_SESSION(handle), "nativeSessionOpen")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionStart(JNIEnv* env, jobject /*thiz*/,
                                                            jlong handle) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
    std::string error;
    const int code = kivo::jni_bridge::session_start(session, error);
    if (code != 0) {
        KIVO_LOGW("session start failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("session start ok");
    return 0;
    KIVO_JNI_END_INT(env, KIVO_JNI_SESSION(handle), "nativeSessionStart")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionSetPacing(JNIEnv* env, jobject /*thiz*/,
                                                               jlong handle, jint target_fps,
                                                               jboolean allow_drop,
                                                               jint clock_offset_ms,
                                                               jint clock_offset_at_ms,
                                                               jint clock_rate_ppm,
                                                               jboolean drift_gate,
                                                               jboolean step_repay,
                                                               jint drift_band_ms) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
    std::string error;
    const int code = kivo::jni_bridge::session_set_pacing(
        session, static_cast<int>(target_fps), allow_drop == JNI_TRUE,
        static_cast<int>(clock_offset_ms), static_cast<int>(clock_offset_at_ms),
        static_cast<int>(clock_rate_ppm), drift_gate == JNI_TRUE, step_repay == JNI_TRUE,
        static_cast<int>(drift_band_ms), error);
    if (code != 0) {
        KIVO_LOGW("session pacing failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("session pacing on: target_fps=%d allow_drop=%d clock_offset=%dms@%dms "
              "rate=%dppm gate=%d step_repay=%d",
              static_cast<int>(target_fps), allow_drop == JNI_TRUE ? 1 : 0,
              static_cast<int>(clock_offset_ms), static_cast<int>(clock_offset_at_ms),
              static_cast<int>(clock_rate_ppm), drift_gate == JNI_TRUE ? 1 : 0,
              step_repay == JNI_TRUE ? 1 : 0);
    return 0;
    KIVO_JNI_END_INT(env, KIVO_JNI_SESSION(handle), "nativeSessionSetPacing")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionSetPaused(JNIEnv* env, jobject /*thiz*/,
                                                                jlong handle,
                                                                jboolean paused) KIVO_JNI_BEGIN
    std::string error;
    const int code =
        kivo::jni_bridge::session_set_paused(KIVO_JNI_SESSION(handle), paused == JNI_TRUE, error);
    // 失败原因由 `session_set_paused` 写进 `last_error`，这里不再多打一行（bridge 有 300 行硬线）。
    return code;
    KIVO_JNI_END_INT(env, KIVO_JNI_SESSION(handle), "nativeSessionSetPaused")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionSetPipeline(JNIEnv* env,
                                                                 jobject /*thiz*/, jlong handle,
                                                                 jboolean split,
                                                                 jboolean present,
                                                                 jboolean at_time) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
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
    KIVO_JNI_END_INT(env, KIVO_JNI_SESSION(handle), "nativeSessionSetPipeline")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionSeek(JNIEnv* env, jobject /*thiz*/,
                                                           jlong handle, jlong target_us) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
    std::string error;
    const int code =
        kivo::jni_bridge::session_seek(session, static_cast<std::int64_t>(target_us), error);
    if (code != 0) {
        KIVO_LOGW("session seek failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("session seek ok: target=%lld us", static_cast<long long>(target_us));
    return 0;
    KIVO_JNI_END_INT(env, KIVO_JNI_SESSION(handle), "nativeSessionSeek")

// 212：当前媒体位置（µs）——字幕叠加控制器的时间源（P16-P1B26 的 position_us）。
JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionPositionUs(JNIEnv* env,
                                                                 jobject /*thiz*/,
                                                                 jlong handle)
    KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
    return static_cast<jlong>(kivo::jni_bridge::session_position_us(session));
    KIVO_JNI_END_LONG(env, KIVO_JNI_SESSION(handle), "nativeSessionPositionUs")

JNIEXPORT void JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionStop(JNIEnv* env, jobject /*thiz*/,
                                                           jlong handle) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
    kivo::jni_bridge::session_stop(session);
    KIVO_LOGI("session stopped: %s", kivo::jni_bridge::session_summary(session).c_str());
    KIVO_JNI_END_VOID(env, nullptr, "nativeSessionStop")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionRebindSurface(JNIEnv* env, jobject /*thiz*/,
                                                                   jlong handle, jstring label,
                                                                   jobject surface) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
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
    KIVO_JNI_END_INT(env, KIVO_JNI_SESSION(handle), "nativeSessionRebindSurface")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionOpenAudio(JNIEnv* env, jobject /*thiz*/,
                                                               jlong handle, jstring label,
                                                               jint fd, jlong length) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
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
    KIVO_JNI_END_INT(env, KIVO_JNI_SESSION(handle), "nativeSessionOpenAudio")

// 证据/诊断四件套（writeStats / writeSyncStats / summary / lastError）
// 215 起拆至 jni_session_evidence_bridge.cxx——本文件回到家族树 300 行线内。

}  // extern "C"
