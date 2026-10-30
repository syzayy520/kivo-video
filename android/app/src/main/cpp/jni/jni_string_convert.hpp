#pragma once

#include <jni.h>

#include <string>
#include <string_view>

namespace kivo::jni_bridge {

/// 把 jstring 转成 std::string。空指针返回空串，不做异常抛出。
[[nodiscard]] inline std::string to_utf8(JNIEnv* env, jstring value) {
    if (value == nullptr) {
        return {};
    }
    const char* raw = env->GetStringUTFChars(value, nullptr);
    if (raw == nullptr) {
        return {};
    }
    std::string result(raw);
    env->ReleaseStringUTFChars(value, raw);
    return result;
}

/// 把 std::string_view 转成 jstring。失败返回 nullptr（由调用方决定如何报错）。
[[nodiscard]] inline jstring to_jstring(JNIEnv* env, std::string_view value) {
    return env->NewStringUTF(std::string(value).c_str());
}

}  // namespace kivo::jni_bridge
