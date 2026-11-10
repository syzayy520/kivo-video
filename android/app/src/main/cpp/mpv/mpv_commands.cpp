// 253 家族：命令层 —— **只做一件事**：把上层的意图翻译成 mpv 命令并执行。
//
// 【为什么不塞进 session.cpp】
// mpv 的命令有**返回值语义**（失败会带原因），与句柄生命周期是两件事：
// 生命周期是"创建/销毁"，命令是"每次调用都要判成败"。分开才各自说得清。
//
// 【seek 的两种模式】
//   · 快速（默认）：落到关键帧，立刻起播。误差可能达到一整个 GOP。
//   · 精确：mpv 的 `hr-seek=exact`，解到目标帧。代价是落位期要解码。
// 上层按用户选择切（与 Rex 的 `seek.accurate` 开关同一思路）。

#include <cstdio>
#include <string>

#include "mpv_session.hpp"
#include "mpv_session_impl.hpp"

namespace kivo::video::mpv {
namespace {

/// 打开一个媒体文件。mpv 的 `loadfile` 接受 URL 字符串。
bool load_file_impl(mpv_handle* handle, const std::string& url, std::string& error) {
    const char* command[] = {"loadfile", url.c_str(), "replace", nullptr};
    const int rc = mpv_command(handle, command);
    if (rc < 0) {
        error = "loadfile failed: " + std::string(mpv_error_string(rc));
        return false;
    }
    return true;
}

/// 下发一条命令。参数已按 mpv 的命令行格式序列化。
bool command_impl(mpv_handle* handle, const std::string& name, const std::string& args,
                  std::string& error) {
    const std::string line = args.empty() ? name : (name + " " + args);
    const int rc = mpv_command_string(handle, line.c_str());
    if (rc < 0) {
        error = "command failed: " + std::string(mpv_error_string(rc));
        return false;
    }
    return true;
}

/// 读一个属性（字符串形式）。读不到是**正常**的（例如还没起播时读 time-pos）。
bool property_impl(mpv_handle* handle, const std::string& name, std::string& out,
                   std::string& error) {
    char* value = mpv_get_property_string(handle, name.c_str());
    if (value == nullptr) {
        error = "property not available: " + name;
        return false;
    }
    out = value;
    mpv_free(value);
    return true;
}

/// 跳到 `position_seconds`。
///
/// 精确模式用 mpv 的 `seek <pos> absolute exact` —— 第三个参数 `exact` 让 mpv
/// 解到目标帧而不是关键帧。这是 mpv 原生能力，我们不自己实现 hr-seek。
bool seek_impl(mpv_handle* handle, double position_seconds, bool exact, std::string& error) {
    char position[32];
    std::snprintf(position, sizeof(position), "%.3f", position_seconds);
    const std::string mode = exact ? "absolute+exact" : "absolute";
    const std::string line = std::string("seek ") + position + " " + mode;
    const int rc = mpv_command_string(handle, line.c_str());
    if (rc < 0) {
        error = "seek failed: " + std::string(mpv_error_string(rc));
        return false;
    }
    return true;
}

}  // namespace

bool Session::load_file(const std::string& url, std::string& error) {
    if (!impl_ || !impl_ || impl_->handle == nullptr) {
        error = "session not created";
        return false;
    }
    return load_file_impl(impl_->handle, url, error);
}

bool Session::command(const std::string& name, const std::string& args, std::string& error) {
    if (!impl_ || !impl_ || impl_->handle == nullptr) {
        error = "session not created";
        return false;
    }
    return command_impl(impl_->handle, name, args, error);
}

bool Session::property(const std::string& name, std::string& out, std::string& error) {
    if (!impl_ || !impl_ || impl_->handle == nullptr) {
        error = "session not created";
        return false;
    }
    return property_impl(impl_->handle, name, out, error);
}

bool Session::seek(double position_seconds, bool exact, std::string& error) {
    if (!impl_ || !impl_ || impl_->handle == nullptr) {
        error = "session not created";
        return false;
    }
    return seek_impl(impl_->handle, position_seconds, exact, error);
}

double Session::position_seconds() {
    if (!impl_ || !impl_ || impl_->handle == nullptr) {
        return -1.0;
    }
    double position = 0.0;
    const int rc = mpv_get_property(impl_->handle, "time-pos", MPV_FORMAT_DOUBLE, &position);
    return rc < 0 ? -1.0 : position;
}

}  // namespace kivo::video::mpv
