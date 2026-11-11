// 253 家族：JNI 导出层（命令与状态）—— **只做一件事**：把"动作"和"状态"翻译成家族内调用。
//
// 【两类调用，语义不同】
//   · 命令（command）：一次性动作 —— seek / pause / loadfile。
//   · 属性（property）：当前状态 —— 倍速是多少、当前音轨是几号、字幕轨有哪些。
// 两者都在这里，因为它们**成对出现**：状态必须能写（改倍速）、
// 且写完必须能读回来（UI 上显示"1.5×"）。
// 拆成两个文件反而会让"写用一套语法、读用另一套"这件事更难发现。
//
// 【属性路径语法】
// mpv 支持 `track-list/count`、`track-list/0/title` 这样的路径。
// 这意味着轨道列表**不需要**在 native 侧解析成 JSON 再传给 Kotlin ——
// 逐项读字符串即可，少一层序列化、少一处格式漂移。

#include <jni.h>

#include <string>

#include "mpv_session.hpp"
#include "mpv_session_table.hpp"

namespace kivo::video::mpv {
namespace {

std::string to_string(JNIEnv* env, jstring value) {
    if (value == nullptr) {
        return std::string();
    }
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (chars == nullptr) {
        return std::string();
    }
    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

void throw_error(JNIEnv* env, const std::string& message) {
    env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), message.c_str());
}

}  // namespace
}  // namespace kivo::video::mpv

using kivo::video::mpv::Session;

extern "C" {

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeSeek(JNIEnv* env, jclass, jlong handle, jdouble position,
                                              jboolean exact) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    if (session == nullptr) {
        kivo::video::mpv::throw_error(env, "handle is invalid");
        return -1;
    }
    std::string error;
    const bool ok = session->seek(position, exact == JNI_TRUE, error);
    if (!ok) {
        kivo::video::mpv::throw_error(env, error);
        return -1;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeCommand(JNIEnv* env, jclass, jlong handle, jstring name,
                                                 jstring args) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    if (session == nullptr) {
        kivo::video::mpv::throw_error(env, "handle is invalid");
        return -1;
    }
    std::string error;
    const bool ok = session->command(kivo::video::mpv::to_string(env, name),
                                     kivo::video::mpv::to_string(env, args), error);
    if (!ok) {
        kivo::video::mpv::throw_error(env, error);
        return -1;
    }
    return 0;
}

JNIEXPORT jdouble JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativePositionSeconds(JNIEnv*, jclass, jlong handle) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    return session == nullptr ? -1.0 : session->position_seconds();
}

JNIEXPORT jdouble JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeDurationSeconds(JNIEnv*, jclass, jlong handle) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    return session == nullptr ? -1.0 : session->duration_seconds();
}

JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeGetProperty(JNIEnv* env, jclass, jlong handle,
                                                     jstring name) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    if (session == nullptr) {
        kivo::video::mpv::throw_error(env, "handle is invalid");
        return nullptr;
    }
    std::string value;
    std::string error;
    if (!session->property(kivo::video::mpv::to_string(env, name), value, error)) {
        // 【读不到不是错误】属性在某些状态下本来就不存在
        // （比如媒体还没加载时读 duration）。返回 null 让 Kotlin 自行判断，
        // 而不是抛异常 —— 轮询场景下抛异常会把整个轮询打断。
        return nullptr;
    }
    return env->NewStringUTF(value.c_str());
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeSetProperty(JNIEnv* env, jclass, jlong handle,
                                                      jstring name, jstring value) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    if (session == nullptr) {
        kivo::video::mpv::throw_error(env, "handle is invalid");
        return -1;
    }
    std::string error;
    const bool ok = session->set_property(kivo::video::mpv::to_string(env, name),
                                           kivo::video::mpv::to_string(env, value), error);
    if (!ok) {
        kivo::video::mpv::throw_error(env, error);
        return -1;
    }
    return 0;
}

}  // extern "C"
