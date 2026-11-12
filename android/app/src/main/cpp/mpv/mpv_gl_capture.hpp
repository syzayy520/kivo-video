#pragma once

// 253 家族：离屏取帧后端 —— **只做一件事**：把 mpv 画面渲染进纹理 FBO 并读回 CPU。
//
// 【为什么不用 ImageReader / EGL window surface】真机红清屏二分定论（v31s/v31w）：
// Adreno 650 上 EGL window surface → ImageReader 的缓冲内容传输是坏的 ——
// eglSwapBuffers 成功、timestamp 推进，但缓冲永远是 gralloc 填充色（连自绘
// glClear 红都到不了 ImageReader；同一循环红清屏却能上主 SurfaceView）。
// 去掉 EGL_RECORDABLE_ANDROID 亦无效（v31w 探针 t=90 仍全黑）。
// 纹理 FBO + glReadPixels 不走那条坏传输，且真机已证 FBO 读回可靠。
//
// 【线程】独立 GL 线程渲染；read_frame 由控制线程调，内部 mutex 交接最新帧。
// 【家族边界】B4 跨文件走 mpv_session.hpp；B5 每个 .cpp ≤ 300 行。

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct mpv_handle;
struct mpv_render_context;

namespace kivo::video::mpv {

class EglWindowSurface;

/// 离屏 mpv 渲染 → 纹理 FBO → glReadPixels → 最新 RGBA 帧（top-down）。
class FrameCapture {
public:
    explicit FrameCapture(mpv_handle* handle, std::string log_path = std::string());
    ~FrameCapture();

    FrameCapture(const FrameCapture&) = delete;
    FrameCapture& operator=(const FrameCapture&) = delete;
    FrameCapture(FrameCapture&&) = delete;
    FrameCapture& operator=(FrameCapture&&) = delete;

    /// 起 GL 线程 + EGL(pbuffer) + mpv 渲染上下文 + 纹理 FBO。失败原因进 error。
    [[nodiscard]] bool start(int width, int height, std::string& error);

    /// 停线程并销毁 GL/mpv 资源。可重复调用。
    void stop();

    [[nodiscard]] bool running() const { return running_.load(std::memory_order_acquire); }

    /// 取最新一帧 RGBA（top-down，w*h*4）。仅当序号比上次读新才拷贝并返回 true。
    /// seek 挂起期的重绘帧（seeking=true）不返回：那是旧画面，慢源上会被
    /// 上层当成本次 seek 的新帧收下导致冻帧（真机 SMB 大文件复现）。
    [[nodiscard]] bool read_frame(std::vector<uint8_t>& out);

    /// 最新一帧渲染时 mpv 的 time-pos（秒），无帧为 -1。仅取证日志用。
    double frame_pts();

    /// 每次 seek 前调一次：代数 +1。渲染线程给每帧盖上当前代数。
    /// 【为什么需要代数】关键帧 seek 会连续落在同一关键帧上，pts 不变；
    /// 用 pts 差判"帧是否属于本次 seek"会把同关键帧的新帧误判成旧帧
    /// （真机：首次拖动整 6s 超时）。代数只增不减，同关键帧也递增。
    void note_seek() { seek_gen_.fetch_add(1, std::memory_order_release); }

    /// 最新一帧的 seek 代数；无帧为 0。
    uint64_t frame_generation();

private:
    void run();
    void render_loop(EglWindowSurface& egl, mpv_render_context* ctx);
    void fail(const std::string& reason);

    mpv_handle* handle_ = nullptr;
    std::string log_path_;
    int width_ = 0;
    int height_ = 0;
    std::thread thread_;
    std::atomic<bool> ready_{false};
    std::atomic<bool> failed_{false};
    std::atomic<bool> running_{false};
    std::atomic<bool> stopping_{false};
    std::string failure_;

    std::mutex frame_mutex_;
    std::vector<uint8_t> frame_;
    uint64_t seq_ = 0;        // guarded by frame_mutex_
    uint64_t last_read_ = 0;  // guarded by frame_mutex_
    double frame_pts_ = -1;   // guarded by frame_mutex_
    uint64_t frame_gen_ = 0;  // guarded by frame_mutex_
    bool frame_seeking_ = true;  // guarded by frame_mutex_
    std::atomic<uint64_t> seek_gen_{0};
};

[[nodiscard]] std::unique_ptr<FrameCapture> create_frame_capture(mpv_handle* handle,
                                                                 std::string log_path = std::string());

}  // namespace kivo::video::mpv