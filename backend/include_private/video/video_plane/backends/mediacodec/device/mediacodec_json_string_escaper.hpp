#pragma once

#include <string>
#include <string_view>

namespace kivo::video::mediacodec {

/// 把任意字节序列转义为可安全嵌入 JSON 双引号字符串的形式。
///
/// 只负责转义，不加引号，不做结构拼接 —— 结构由调用方（renderer）决定。
/// 控制字符统一转成 \u00XX，避免上游产生非法 JSON。
[[nodiscard]] std::string escape_json_string(std::string_view input);

}  // namespace kivo::video::mediacodec
