// 253 家族：JNI 导出层（会话生命周期）—— **只做一件事**：把 Kotlin 的调用翻译成家族内接口的调用。
//
// 【它是薄壳，这是纪律】
// 本文件**不含**任何 mpv API 调用、任何事件处理、任何渲染逻辑。
// 它只做三件事：
//   1. 把 Kotlin 传来的字符串/对象转成 C++ 类型；
//   2. 调用 mpv_session.hpp 里的接口；
//   3. 把结果与错误码交回 Kotlin。
//
// 【本文件只管"会话生命周期"】
// 创建 / 初始化 / 绑定 Surface / 加载媒体 / 释放。
// 命令与属性各有自己的文件（jni_mpv_command_bridge.cpp / jni_mpv_property_bridge.cpp）——
// 拆开的理由不是行数，而是**关注点**：这三类失败的排查手段完全不同
// （创建失败 = 内核起不来；命令失败 = 意图没生效；属性失败 = 状态读不到）。
//
// 【为什么句柄用 long 而不是对象】
// Kotlin 侧需要跨多次调用持有同一个会话。用裸指针当 handle 有风险，
// 所以我们传的是**受控的索引**：全局表 + 序号。Kotlin 只认数字，
// 拿不到任何 C++ 对象的地址 —— 与旧内核的 `handle` 约定一致。

#include <jni.h>
#include <android/native_window_jni.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "mpv_session.hpp"
#include "mpv_session_table.hpp"
#include "mpv_java_vm.hpp"

namespace kivo::video::mpv {
namespace {

/// 把 Kotlin 的字符串转成 C++ 字符串（空指针 → 空串）。
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

/// 抛一个 Java 异常，把 native 的失败原因交回 Kotlin。
void throw_error(JNIEnv* env, const std::string& message) {
    env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), message.c_str());
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
        kivo::video::mpv::throw_error(env, "register java vm failed: " + vm_error);
        return 0;
    }

    std::string error;
    auto session = Session::create(error);
    if (!session) {
        kivo::video::mpv::throw_error(env, error);
        return 0;
    }
    return kivo::video::mpv::register_session(std::move(session));
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeInitialize(JNIEnv* env, jclass, jlong handle,
                                                   jstring vo, jstring ao,
                                                   jstring log_file,
                                                   jstring sub_fonts_dir,
                                                   jstring sub_font_family) {
    // 【整段持锁】查表 → 调用 → 可能改表 必须是一个原子区间，
    // 否则中途表被别的线程清掉就会拿到野指针（use-after-free）。
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    if (session == nullptr) {
        kivo::video::mpv::throw_error(env, "handle is invalid");
        return -1;
    }
    const std::string vo_name = kivo::video::mpv::to_string(env, vo);
    std::string error;
    const bool ok = session->initialize(
        vo_name.empty() ? "libmpv" : vo_name,
        kivo::video::mpv::to_string(env, ao),
        kivo::video::mpv::to_string(env, log_file),
        kivo::video::mpv::to_string(env, sub_fonts_dir),
        kivo::video::mpv::to_string(env, sub_font_family),
        error);
    if (!ok) {
        kivo::video::mpv::throw_error(env, error);
        return -1;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeAttachSurface(JNIEnv* env, jclass, jlong handle,
                                                      jobject surface) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    if (session == nullptr) {
        kivo::video::mpv::throw_error(env, "handle is invalid");
        return -1;
    }
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        kivo::video::mpv::throw_error(env, "surface is null");
        return -1;
    }
    std::string error;
    const bool ok = session->attach_surface(window, error);
    // ANativeWindow_fromSurface 返回的引用由我们持有 —— mpv 会 retain 一次，
    // 这里必须 release 掉我们这一份，否则 Surface 永远解不开。
    ANativeWindow_release(window);
    if (!ok) {
        kivo::video::mpv::throw_error(env, error);
        return -1;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeAttachCapture(JNIEnv* env, jclass, jlong handle,
                                                       jint width, jint height) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    if (session == nullptr) {
        kivo::video::mpv::throw_error(env, "handle is invalid");
        return -1;
    }
    std::string error;
    if (!session->attach_capture(width, height, error)) {
        kivo::video::mpv::throw_error(env, error);
        return -1;
    }
    return 0;
}

JNIEXPORT jbyteArray JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeReadCaptureFrame(JNIEnv* env, jclass, jlong handle) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    if (session == nullptr) return nullptr;
    std::vector<uint8_t> buf;
    if (!session->read_capture_frame(buf)) return nullptr;
    jbyteArray arr = env->NewByteArray(static_cast<jsize>(buf.size()));
    if (arr == nullptr) return nullptr;
    env->SetByteArrayRegion(arr, 0, static_cast<jsize>(buf.size()),
                            reinterpret_cast<const jbyte*>(buf.data()));
    return arr;
}

JNIEXPORT jdouble JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeCaptureFramePts(JNIEnv* env, jclass, jlong handle) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    if (session == nullptr) return -1.0;
    return session->capture_frame_pts();
}

JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeCaptureFrameGen(JNIEnv* env, jclass, jlong handle) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    if (session == nullptr) return 0;
    return static_cast<jlong>(session->capture_frame_generation());
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeFirstFrameRendered(JNIEnv* env, jclass, jlong handle) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    if (session == nullptr) return 0;
    return session->first_frame_rendered() ? 1 : 0;
}

JNIEXPORT jint JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeLoadFile(JNIEnv* env, jclass, jlong handle,
                                                  jstring url, jdouble start_seconds) {
    kivo::video::mpv::SessionTableLock lock;
    Session* session = kivo::video::mpv::find_session_locked(handle);
    if (session == nullptr) {
        kivo::video::mpv::throw_error(env, "handle is invalid");
        return -1;
    }
    std::string error;
    const bool ok = session->load_file(kivo::video::mpv::to_string(env, url),
                                       static_cast<double>(start_seconds), error);
    if (!ok) {
        kivo::video::mpv::throw_error(env, error);
        return -1;
    }
    return 0;
}

JNIEXPORT void JNICALL
Java_com_kivo_cinema_mpv_MpvNative_nativeRelease(JNIEnv*, jclass, jlong handle) {
    // 【注意】这里不持 SessionTableLock —— erase_session 内部会销毁 Session，
    // 而 Session 的析构要 join 事件线程、释放渲染上下文。
    // 持锁销毁会把其它线程的 JNI 调用一起堵住（那几秒里所有调用都排队）。
    // erase_session 自己加了锁，所以这里不重复加。
    kivo::video::mpv::erase_session(handle);
}

}  // extern "C"
