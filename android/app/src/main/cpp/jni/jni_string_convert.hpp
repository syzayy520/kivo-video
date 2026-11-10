#pragma once

#include <jni.h>

#include <string>
#include <string_view>
#include <vector>

#include "utf16_codec.hpp"
#include "utf8_codec.hpp"

namespace kivo::jni_bridge {

/// 把 jstring 转成 UTF-8。空指针返回空串，不做异常抛出。
///
/// 250 D8：改走 `GetStringChars`（UTF-16）+ 自己的编码器。原实现用
/// `GetStringUTFChars`，它给的是 **modified UTF-8** —— BMP 外的字符（emoji、
/// 部分符号）会被编成 CESU-8 的六字节代理对；交给 libsmb2 当路径用时，
/// 服务器按标准 UTF-8 解码就是另外两个字，于是"文件明明在那儿却打不开"。
[[nodiscard]] inline std::string to_utf8(JNIEnv* env, jstring value) {
    if (value == nullptr) {
        return {};
    }
    const jsize length = env->GetStringLength(value);
    if (length <= 0) {
        return {};
    }
    const jchar* raw = env->GetStringChars(value, nullptr);
    if (raw == nullptr) {
        return {};
    }
    std::string result = utf16_to_utf8(std::u16string_view(
        reinterpret_cast<const char16_t*>(raw), static_cast<std::size_t>(length)));
    env->ReleaseStringChars(value, raw);
    return result;
}

/// 把 UTF-8 转成 jstring。失败返回 nullptr（由调用方决定如何报错）。
///
/// 250 D8：改走 `NewString`（UTF-16）。原实现用 `NewStringUTF`，它要求输入是
/// **modified UTF-8**，而 SMB 文件名是标准 UTF-8 —— 含 4 字节序列（emoji）时
/// CheckJNI 下直接 abort，不检查时得到乱码。两个方向都错，所以两个函数一起改。
[[nodiscard]] inline jstring to_jstring(JNIEnv* env, std::string_view value) {
    const std::u16string units = utf8_to_utf16(value);
    return env->NewString(reinterpret_cast<const jchar*>(units.data()),
                          static_cast<jsize>(units.size()));
}

/// jobjectArray<jstring> → std::vector<std::string>。
///
/// 归位说明（P16-P0）：这段原本住在 `jni_player_bridge.cxx` 的匿名命名空间里，
/// 但它的职责是**字符串转换**，不是"Java native 实现"。空指针返回空 vector；
/// null 元素跳过；局部引用逐个释放。
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
