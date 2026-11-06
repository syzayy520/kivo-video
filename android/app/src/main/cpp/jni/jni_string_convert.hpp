#pragma once

#include <jni.h>

#include <string>
#include <string_view>
#include <vector>

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

/// jobjectArray<jstring> → std::vector<std::string>。
/// 归位说明（P16-P0）：这段原本住在 `jni_player_bridge.cxx` 的匿名命名空间里，
/// 但它的职责是**字符串转换**，不是"Java native 实现"。移到这里后 bridge 只留转发，
/// 也顺带把那个文件从 302 行压回家族树 §四 要求的 300 行以内。
/// 空指针返回空 vector；null 元素跳过；局部引用逐个释放。
[[nodiscard]] inline std::vector<std::string> to_string_vector(JNIEnv* env,
                                                                jobjectArray array) {
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
        out.push_back(to_utf8(env, element));
        env->DeleteLocalRef(element);
    }
    return out;
}

}  // namespace kivo::jni_bridge
