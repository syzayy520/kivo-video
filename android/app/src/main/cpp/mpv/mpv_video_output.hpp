#pragma once

// 253 家族：视频输出层的**接口**。
//
// 【为什么定义与实现分两个文件】
// `mpv_gl_render.cpp` 要 include EGL/GLES 头文件；`mpv_session.cpp` 只需要
// 拿着一个 `VideoOutput*` 调 start/stop —— 不该被迫看见 GL 头。
// 所以：声明在这里，实现在 mpv_gl_render.cpp。
//
// 【家族边界】
//   B4 跨文件调用走本头文件或 mpv_session.hpp
//   B5 每个 .cpp ≤ 300 行

#include <android/native_window.h>

#include <atomic>
#include <string>
#include <thread>

struct mpv_handle;

namespace kivo::video::mpv {

/// 把 mpv 渲染到 Android Surface。
///
/// 内部起**独立 GL 线程**：EGL 上下文与 Surface 绑定在那个线程，
/// 主线程只交出 ANativeWindow*（本对象自己 retain 一次，stop 时 release）。
class VideoOutput {
public:
    /// @param log_path mpv 日志落盘路径（可选）—— 渲染层用它写自己的诊断。
    explicit VideoOutput(mpv_handle* handle, std::string log_path = std::string());
    ~VideoOutput();

    VideoOutput(const VideoOutput&) = delete;
    VideoOutput& operator=(const VideoOutput&) = delete;
    VideoOutput(VideoOutput&&) = delete;
    VideoOutput& operator=(VideoOutput&&) = delete;

    /// 起 GL 线程并完成 EGL/mpv 渲染上下文的建立。
    /// 失败时 `error()` 里有原因。
    [[nodiscard]] bool start(ANativeWindow* window, std::string& error);

    /// 停线程并销毁 EGL 与 mpv 渲染上下文。可重复调用。
    void stop();

    /// 当前视频尺寸（像素）；还不知道时返回 0。
    [[nodiscard]] int width() const { return width_.load(std::memory_order_relaxed); }
    [[nodiscard]] int height() const { return height_.load(std::memory_order_relaxed); }

private:
    void run();
    void render_loop(void* display, void* surface, void* context, void* render_context);
    void fail(const std::string& reason);

    mpv_handle* handle_ = nullptr;
    ANativeWindow* window_ = nullptr;
    /// mpv 日志落盘路径（可选）。渲染层用它写自己的诊断 ——
    /// 这台设备读不到 app 的 logcat，只能写文件。
    std::string log_path_;
    /// 已渲染帧计数（诊断用）。
    std::atomic<int> frame_count_{0};
    std::thread thread_;
    std::atomic<bool> ready_{false};
    std::atomic<bool> failed_{false};
    std::atomic<bool> running_{false};
    std::atomic<bool> stopping_{false};
    std::atomic<int> width_{0};
    std::atomic<int> height_{0};
    std::string failure_;
};

/// 创建一个视频输出。返回 nullptr 表示创建失败。
[[nodiscard]] std::unique_ptr<VideoOutput> create_video_output(mpv_handle* handle,
                                                         std::string log_path = std::string());

}  // namespace kivo::video::mpv
