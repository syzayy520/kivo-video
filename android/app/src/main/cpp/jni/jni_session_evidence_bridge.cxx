// 会话证据导出与诊断读取的 JNI 实现（215 拆出：单文件单职责）。
//
// 家族树 §四 的 jni_player_bridge.cxx 越过 300 行线（M1 期间新增导出所致），
// 按职责拆出"证据/诊断"四件套到本文件：writeStats / writeSyncStats / summary / lastError。
// 边界宏用法与主 bridge 完全一致（jni_call_guard.hpp），本文件不含任何解码/渲染逻辑。

#include "jni_call_guard.hpp"
#include "jni_string_convert.hpp"

#include <string>

extern "C" {

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionWriteSyncStats(JNIEnv* env, jobject /*thiz*/,
                                                                    jlong handle, jstring path) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
    const std::string stats_path = kivo::jni_bridge::to_utf8(env, path);

    std::string error;
    const int code = kivo::jni_bridge::session_write_sync_stats(session, stats_path, error);
    if (code != 0) {
        KIVO_LOGW("write sync stats failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("write sync stats ok: %s", stats_path.c_str());
    return 0;
    KIVO_JNI_END_INT(env, KIVO_JNI_SESSION(handle), "nativeSessionWriteSyncStats")

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionWriteStats(JNIEnv* env, jobject /*thiz*/,
                                                                 jlong handle, jstring path) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
    const std::string stats_path = kivo::jni_bridge::to_utf8(env, path);

    std::string error;
    const int code = kivo::jni_bridge::session_write_stats(session, stats_path, error);
    if (code != 0) {
        KIVO_LOGW("write stats failed: code=%d reason=%s", code, error.c_str());
        return static_cast<jint>(code);
    }
    KIVO_LOGI("write stats ok: %s", stats_path.c_str());
    return 0;
    KIVO_JNI_END_INT(env, KIVO_JNI_SESSION(handle), "nativeSessionWriteStats")

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionSummary(JNIEnv* env, jobject /*thiz*/,
                                                              jlong handle) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
    return kivo::jni_bridge::to_jstring(env, kivo::jni_bridge::session_summary(session));
    KIVO_JNI_END_STRING(env, KIVO_JNI_SESSION(handle), "nativeSessionSummary")

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_bridge_NativePlayer_nativeSessionLastError(JNIEnv* env, jobject /*thiz*/,
                                                                jlong handle) KIVO_JNI_BEGIN
    auto* session = KIVO_JNI_SESSION(handle);
    return kivo::jni_bridge::to_jstring(env, kivo::jni_bridge::session_last_error(session));
    KIVO_JNI_END_STRING(env, KIVO_JNI_SESSION(handle), "nativeSessionLastError")

}  // extern "C"
