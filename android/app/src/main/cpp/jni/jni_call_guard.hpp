#pragma once

// JNI 导出的异常边界（P16-P0 / D1）。
//
// 家族树位置：`android/app/src/main/cpp/jni/`，与 `jni_string_convert.hpp` 同属
// 「Java ↔ C++ 边界」这一族。唯一职责：把 C++ 异常转译成 JNI 能带回去的
// 返回码 + 非空 reason。**不含会话状态、不含解码逻辑、不写文件。**
//
// 为什么必须包：C++ 侧的 std::string / std::vector / operator new 都可能抛，
// 而所有 JNI 导出都是 `extern "C"` —— 异常穿过 JNI 帧不会被 JVM 接住，
// 直接 `std::terminate`。本机 HyperOS 的 logcat 对 adb 又完全不可见，
// 故障会退化成「进程静默消失」，连一条线索都不留。
//
// 用 C++ 的**函数 try 块**实现，所以一对宏不占额外行数：
//   JNIEXPORT jint JNICALL Java_..._Foo(JNIEnv* env, ...) KIVO_JNI_BEGIN
//       ... 原函数体 ...
//       KIVO_JNI_END_INT(env, KIVO_JNI_SESSION(handle), "Foo")
// `KIVO_JNI_END_*` 的最后一个 `}` 同时结束 catch 块与函数定义 —— 函数 try 块
// **不需要**普通函数那种额外闭合括号。它替换原来的 `}` 而不是新增一行。
//
// 为什么不用普通的 `try { ... } catch`：那要给 17 个导出各加两行，
// 会把 `jni_player_bridge.cxx` 撑到 322 行，越过家族树 §四 的 300 行线。
// 加异常边界不该以破坏单文件纪律为代价。
//
// 函数 try 块的 catch 只能看见**函数参数**，看不见函数体里的局部变量，
// 所以句柄表达式必须由调用处给出（`KIVO_JNI_SESSION(handle)` 或 `nullptr`）。
//
// 失败时：写 `SessionHandle::last_error`（G9 要求非空 reason）、打 WARN、
// 返回该返回类型的安全值。正常路径不多执行一条指令，证据口径不变。

#include <android/log.h>
#include <jni.h>

#include <cstdio>
#include <exception>
#include <type_traits>

#include "jni_session_handle.hpp"

#define KIVO_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "KivoCinema", __VA_ARGS__)
#define KIVO_LOGW(...) __android_log_print(ANDROID_LOG_WARN, "KivoCinema", __VA_ARGS__)

namespace kivo::jni_bridge {

/// native 层未捕获异常的统一返回码。既有错误码全为正（0..11），-1 不冲突，
/// Kotlin 侧一眼能分出「平台返回的错误」和「native 自己炸了」。
inline constexpr jint kNativeExceptionCode = -1;

/// 模板按 JNI 返回类型给出安全值。
/// 刻意全部走不抛异常的构造：jstring 分支用固定 buffer + snprintf，
/// 不在这个 noexcept 边界里再 new 一个 std::string。
template <class R>
[[nodiscard]] R on_native_exception(JNIEnv* env, SessionHandle* handle, const char* where,
                                    const char* what) noexcept {
    session_record_native_error(handle, where, what);
    KIVO_LOGW("native exception in %s: %s", where, what);

    if constexpr (std::is_same_v<R, void>) {
        return;
    } else if constexpr (std::is_same_v<R, jint>) {
        return kNativeExceptionCode;
    } else if constexpr (std::is_same_v<R, jlong>) {
        return static_cast<jlong>(0);  // 空句柄：创建失败，调用方不会再拿它去 destroy
    } else if constexpr (std::is_same_v<R, jboolean>) {
        // 211 的 Running 查询走这里：异常时按"没在跑"处理，调用方走收尾分支。
        return static_cast<jboolean>(JNI_FALSE);
    } else if constexpr (std::is_same_v<R, jstring>) {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "native exception in %s: %.160s", where, what);
        jstring result = env->NewStringUTF(buffer);
        return result != nullptr ? result : env->NewStringUTF("native exception (detail unavailable)");
    } else {
        static_assert(std::is_same_v<R, void>, "unsupported JNI return type");
        return R{};
    }
}

}  // namespace kivo::jni_bridge

// 从函数参数 `handle`（jlong）还原会话指针；catch 里只能看见参数，看不见函数体局部变量。
#define KIVO_JNI_SESSION(H) reinterpret_cast<::kivo::jni_bridge::SessionHandle*>(H)

#define KIVO_JNI_BEGIN try {
#define KIVO_JNI_END(ENV, HANDLE, WHERE, R)                                                    \
    } catch (const std::exception& e) {                                                           \
        return ::kivo::jni_bridge::on_native_exception<R>((ENV), (HANDLE), (WHERE), e.what());   \
    } catch (...) {                                                                            \
        return ::kivo::jni_bridge::on_native_exception<R>((ENV), (HANDLE), (WHERE),            \
                                                          "unknown C++ exception");            \
    }  // catch 结束 = 函数定义结束，不再补 `}`

#define KIVO_JNI_END_INT(ENV, HANDLE, WHERE) KIVO_JNI_END(ENV, HANDLE, WHERE, jint)
#define KIVO_JNI_END_LONG(ENV, HANDLE, WHERE) KIVO_JNI_END(ENV, HANDLE, WHERE, jlong)
#define KIVO_JNI_END_VOID(ENV, HANDLE, WHERE) KIVO_JNI_END(ENV, HANDLE, WHERE, void)
#define KIVO_JNI_END_STRING(ENV, HANDLE, WHERE) KIVO_JNI_END(ENV, HANDLE, WHERE, jstring)
