#pragma once

#include <cstdint>
#include <string>

extern "C" {
#include <libavformat/avio.h>
}

namespace kivo::video::demux {

/// 把只读 fd 接到 libavformat 的 AVIOContext。
///
/// 职责只有三个：按位置读、按位置定位、报告大小。不解析容器、不认识轨道、
/// 不碰解码。这样 FFmpeg 的读取行为与平台解封装器完全一致 —— 都走同一个
/// `HttpRangeFile` 代理 fd，RAnge 校验、共享缓存、取消语义全部复用，
/// 不需要在网络层再开一条旁路。
///
/// 用 `pread` 而不是 `lseek + read`：fd 可能与其他读者共享偏移，
/// 改偏移会让别人的读错位。`pread` 不改变文件偏移，天然互不干扰。
///
/// 线程约束：非线程安全，由单个解封装实例独占。
class AvioBridge {
public:
    /// @param fd 只读 fd，调用方持有并负责最终关闭
    /// @param offset 容器起点，通常为 0
    /// @param length 容器长度，必须是真实文件大小；为 0 或负值视为无效
    AvioBridge(int fd, std::int64_t offset, std::int64_t length);
    ~AvioBridge();

    AvioBridge(const AvioBridge&) = delete;
    AvioBridge& operator=(const AvioBridge&) = delete;

    /// 打开成功返回非空。失败时 `error_message()` 给出原因。
    [[nodiscard]] AVIOContext* context() const { return context_; }
    [[nodiscard]] bool ok() const { return context_ != nullptr; }
    [[nodiscard]] const std::string& error_message() const { return error_message_; }

    /// 已发生的读错误（含取消）。FFmpeg 会把短读当成 EOF，必须由外部再确认一次，
    /// 否则"网络断了"会被当成"文件播完了"。
    [[nodiscard]] bool failed() const { return failed_; }
    void set_cancelled(const bool* flag) { cancelled_ = flag; }

private:
    static int read_packet(void* opaque, std::uint8_t* buffer, int buffer_size);
    static std::int64_t seek(void* opaque, std::int64_t offset, int whence);

    int fd_ = -1;
    std::int64_t offset_ = 0;
    std::int64_t length_ = 0;
    std::int64_t position_ = 0;
    AVIOContext* context_ = nullptr;
    bool failed_ = false;
    const bool* cancelled_ = nullptr;
    std::string error_message_;
};

}  // namespace kivo::video::demux
