#pragma once

// 253 家族：mpv 选项的**最小原语** —— 一项选项的表示 + 逐项设置 + 回执检查。
//
// 【为什么单独成文件】mpv 的选项分几组（视频/音频/缓存/字幕），
// 每一组都有自己的失败语义；但"设一项并检查回执"是**同一件事**，
// 复制几遍就会出现"有的组检查回执、有的组不检查"的不一致。
// 于是：一项选项怎么表示、怎么设、怎么算失败 —— 放这里；
// 各组只负责**列出自己的选项**（见 mpv_options.cpp / mpv_sub_options.cpp）。
//
// 【判据】每个选项都必须**要么生效、要么报错** —— mpv 对不存在的选项
// 只返回错误码、不打印任何日志，不检查就等于静默失败。

#include <cstdio>
#include <string>
#include <vector>

#include "mpv/client.h"

namespace kivo::video::mpv {

/// 一个待设的 mpv 选项。
struct Option {
    const char* name;
    const char* value;
    /// 设为 true 时，**这一项失败不算整体失败**（只记录）。
    /// 用于"锦上添花"的选项 —— 例如字体族名不匹配时，字幕仍可能靠内嵌字体渲染。
    bool optional;
};

/// 逐项设置并检查回执。
///
/// @param failures 出参：失败项的描述（"name=value (reason)"），逐条追加。
/// @return true = 全部非 optional 项成功。
inline bool apply_options(mpv_handle* handle, const std::vector<Option>& options,
                          std::vector<std::string>& failures) {
    bool all_ok = true;
    for (const Option& option : options) {
        const int rc = mpv_set_option_string(handle, option.name, option.value);
        if (rc >= 0) {
            continue;
        }
        const char* reason = mpv_error_string(rc);
        failures.push_back(std::string(option.name) + "=" + option.value + " (" +
                           (reason != nullptr ? reason : "unknown") + ")");
        if (!option.optional) {
            all_ok = false;
        }
    }
    return all_ok;
}

/// 把一行文本追加到 mpv 日志文件。
///
/// 【为什么需要它】mpv 对不存在的选项**只返回错误码、不打日志**。
/// 若上层把异常吞掉（Kotlin 侧的 initialize 曾经就是这样），
/// 失败原因就彻底消失了 —— 于是排查只能靠猜。
/// 直接写进 mpv 的日志文件，是唯一"谁都能看见"的地方。
inline void append_option_log(const std::string& log_file, const std::string& line) {
    if (log_file.empty()) {
        return;
    }
    FILE* file = std::fopen(log_file.c_str(), "a");
    if (file == nullptr) {
        return;
    }
    std::fputs(line.c_str(), file);
    std::fputc('\n', file);
    std::fclose(file);
}

}  // namespace kivo::video::mpv
