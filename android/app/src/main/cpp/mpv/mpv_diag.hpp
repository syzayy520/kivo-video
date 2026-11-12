#pragma once

// 253 家族：步骤诊断落盘（.diag）。会话（STEP）与渲染层（RND）共用同一单调
// 纪元 —— 不共钟，两种行就排不到一条时间线上（真机实测：swap 与 seek 的先后
// 只能靠猜）。独立 .diag 文件：mpv 自己的日志 fd 不带 O_APPEND，追加行会被
// 启动日志洪水覆盖。

#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <string>

namespace kivo::video::mpv {

inline long long monotonic_ms() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<long long>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
}

inline void mark_step(const char* log_file, const char* step, const char* tag) {
    if (log_file == nullptr || log_file[0] == 0) {
        return;
    }
    static const long long epoch = monotonic_ms();
    const std::string diag = std::string(log_file) + ".diag";
    const int fd = ::open(diag.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd < 0) {
        return;
    }
    const std::string line =
        std::string(tag) + " +" + std::to_string(monotonic_ms() - epoch) + "ms " + step + "\n";
    (void)write(fd, line.data(), line.size());
    ::close(fd);
}

}  // namespace kivo::video::mpv