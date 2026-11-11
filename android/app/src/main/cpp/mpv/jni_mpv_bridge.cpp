// 253 家族：JNI 导出层 —— **只做一件事**：把 Kotlin 的调用翻译成家族内接口的调用。
//
// 【它是薄壳，这是纪律】
// 本文件**不含**任何 mpv API 调用、任何事件处理、任何渲染逻辑。
// 它只做三件事：
//   1. 把 Kotlin 传来的字符串/对象转成 C++ 类型；
//   2. 调用 mpv_session.hpp 里的接口；
//   3. 把结果与错误码交回 Kotlin。
//
// 【为什么句柄用 long 而不是对象】
// Kotlin 侧需要跨多次调用持有同一个会话。用裸指针当 handle 有风险，
// 所以我们传的是**受控的索引**：全局表 + 序号。Kotlin 只认数字，
// 拿不到任何 C++ 对象的地址 —— 与旧内核的 `handle` 约定一致。

#include <jni.h>
#include <android/native_window_jni.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "mpv_session.hpp"
#include "mpv_java_vm.hpp"

namespace kivo::video::mpv {
namespace {

/// 会话表。所有 JNI 调用都要经过它，所以**必须加锁**。
///
/// 【为什么用表而不是直接传指针】
/// Kotlin 侧只拿得到一个 long。裸指针会在 use-after-free 时变成野指针崩溃，
/// 而表 + 序号能让"句柄已释放"变成一个明确的错误码。
std::mutex& table_lock() {
    static std::mutex lock;
    return lock;
}

std::unordered_map<jlong, std::unique_ptr<Session>>& session_table() {
    static std::unordered_map<jlong, std::unique_ptr<Session>> table;
    return table;
}

jlong next_handle() {
    static jlong counter = 1;
    return counter++;
}

}  // namespace

}  // namespace kivo::video::mpv

using kivo::video::mpv::Session;

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeCreate(JNIEnv* env, jclass, jobject app_context) {
    // 【必须在创建 mpv 会话之前注册 JavaVM】
    // mpv 的硬解驱动（hwdec=mediacodec*）与音频输出（ao=audiotrack）都在
    // 第一次使用时去 av_jni_get_java_vm() 取 VM —— 宿主不注册就拿不到，
    // 日志里会出现 "No Java virtual machine has been registered"，
    // 表现为硬解初始化失败、音频输出静默失败。
    // mpv-android 的做法见其 app/src/main/jni/main.cpp:44。
    std::string vm_error;
    if (!kivo::video::mpv::register_java_vm(env, app_context, vm_error)) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      ("register java vm failed: " + vm_error).c_str());
        return 0;
    }

    std::string error;
    auto session = Session::create(error);
    if (!session) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), error.c_str());
        return 0;
    }
    const jlong handle = kivo::video::mpv::next_handle();
    std::lock_guard lock(kivo::video::mpv::table_lock());
    kivo::video::mpv::session_table()[handle] = std::move(session);
    return handle;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeInitialize(JNIEnv* env, jclass, jlong handle,
                                                   jstring vo, jstring ao,
                                                   jstring log_file) {
    std::lock_guard lock(kivo::video::mpv::table_lock());
    auto& table = kivo::video::mpv::session_table();
    auto it = table.find(handle);
    if (it == table.end()) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "handle is invalid");
        return -1;
    }
    const char* vo_chars = vo != nullptr ? env->GetStringUTFChars(vo, nullptr) : "libmpv";
    const char* ao_chars = ao != nullptr ? env->GetStringUTFChars(ao, nullptr) : nullptr;
    const char* log_chars = log_file != nullptr ? env->GetStringUTFChars(log_file, nullptr) : nullptr;
    std::string error;
    const bool ok = it->second->initialize(
        vo_chars != nullptr ? vo_chars : "libmpv",
        ao_chars != nullptr ? ao_chars : "",
        log_chars != nullptr ? log_chars : "",
        error);
    if (vo_chars != nullptr) {
        env->ReleaseStringUTFChars(vo, vo_chars);
    }
    if (ao_chars != nullptr) {
        env->ReleaseStringUTFChars(ao, ao_chars);
    }
    if (log_chars != nullptr) {
        env->ReleaseStringUTFChars(log_file, log_chars);
    }
    if (!ok) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), error.c_str());
        return -1;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeAttachSurface(JNIEnv* env, jclass, jlong handle,
                                                      jobject surface) {
    std::lock_guard lock(kivo::video::mpv::table_lock());
    auto& table = kivo::video::mpv::session_table();
    auto it = table.find(handle);
    if (it == table.end()) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "handle is invalid");
        return -1;
    }
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "surface is null");
        return -1;
    }
    std::string error;
    const bool ok = it->second->attach_surface(window, error);
    // ANativeWindow_fromSurface 返回的引用由我们持有 —— mpv 会 retain 一次，
    // 这里必须 release 掉我们这一份，否则 Surface 永远解不开。
    ANativeWindow_release(window);
    if (!ok) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), error.c_str());
        return -1;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeLoadFile(JNIEnv* env, jclass, jlong handle,
                                                  jstring url) {
    std::lock_guard lock(kivo::video::mpv::table_lock());
    auto& table = kivo::video::mpv::session_table();
    auto it = table.find(handle);
    if (it == table.end()) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "handle is invalid");
        return -1;
    }
    const char* chars = env->GetStringUTFChars(url, nullptr);
    std::string error;
    const bool ok = it->second->load_file(chars != nullptr ? chars : "", error);
    env->ReleaseStringUTFChars(url, chars);
    if (!ok) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), error.c_str());
        return -1;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeSeek(JNIEnv* env, jclass, jlong handle, jdouble position,
                                              jboolean exact) {
    std::lock_guard lock(kivo::video::mpv::table_lock());
    auto& table = kivo::video::mpv::session_table();
    auto it = table.find(handle);
    if (it == table.end()) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "handle is invalid");
        return -1;
    }
    std::string error;
    const bool ok = it->second->seek(position, exact == JNI_TRUE, error);
    if (!ok) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), error.c_str());
        return -1;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeCommand(JNIEnv* env, jclass, jlong handle, jstring name,
                                                 jstring args) {
    std::lock_guard lock(kivo::video::mpv::table_lock());
    auto& table = kivo::video::mpv::session_table();
    auto it = table.find(handle);
    if (it == table.end()) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "handle is invalid");
        return -1;
    }
    const char* name_chars = env->GetStringUTFChars(name, nullptr);
    const char* args_chars = args != nullptr ? env->GetStringUTFChars(args, nullptr) : nullptr;
    std::string error;
    const bool ok = it->second->command(name_chars != nullptr ? name_chars : "",
                                        args_chars != nullptr ? args_chars : "", error);
    env->ReleaseStringUTFChars(name, name_chars);
    if (args_chars != nullptr) {
        env->ReleaseStringUTFChars(args, args_chars);
    }
    if (!ok) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), error.c_str());
        return -1;
    }
    return 0;
}

JNIEXPORT jdouble JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativePositionSeconds(JNIEnv*, jclass, jlong handle) {
    std::lock_guard lock(kivo::video::mpv::table_lock());
    auto& table = kivo::video::mpv::session_table();
    auto it = table.find(handle);
    if (it == table.end()) {
        return -1.0;
    }
    return it->second->position_seconds();
}

JNIEXPORT jdouble JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeDurationSeconds(JNIEnv*, jclass, jlong handle) {
    std::lock_guard lock(kivo::video::mpv::table_lock());
    auto& table = kivo::video::mpv::session_table();
    auto it = table.find(handle);
    if (it == table.end()) {
        return -1.0;
    }
    return it->second->duration_seconds();
}

JNIEXPORT void JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeRelease(JNIEnv*, jclass, jlong handle) {
    std::lock_guard lock(kivo::video::mpv::table_lock());
    auto& table = kivo::video::mpv::session_table();
    auto it = table.find(handle);
    if (it != table.end()) {
        table.erase(it);
    }
}

}  // extern "C"
