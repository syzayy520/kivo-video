#pragma once

#include <string>

namespace kivo::video::mediacodec {

/// 读取 Android 系统属性。属性不存在时返回 "unknown" 而不是空串。
///
/// 这是唯一允许 include <sys/system_properties.h> 的地方：
/// 平台头文件必须被隔离在此，其它文件只看到 std::string。
[[nodiscard]] std::string read_system_property(const char* name);

}  // namespace kivo::video::mediacodec
