// 253 家族：GL 渲染层 —— **只做一件事**：把 mpv 的画面渲染到 Android 的 Surface 上。
//
// 【为什么必须自己管 EGL】
// 上游 mpv **没有** `android-surface` 属性，也没有 `gpu-context=android` ——
// 那是 mpv-android 给 mpv 打的私有补丁（他们的 MPVView.kt 用
// `gpu-context=android` + `android-surface-size`）。
// 我们编的是**上游 mpv v0.39**，所以只能走标准的渲染上下文 API：
//     mpv_render_context_create → mpv_opengl_init_params → 每帧 mpv_render_context_render
//
// 【硬解】
// 会话初始化时设 `hwdec=mediacodec-copy` ⇒ 解码出来的帧在 GL 侧是零拷贝纹理。
//
// 【线程】
// 渲染跑在**独立 GL 线程**上：EGL 上下文与 Surface 都绑定在那个线程。
//
// 【Surface 重绑 = 换面，不重建上下文】
// 真机实测：free 旧渲染上下文再 create 新的，mpv 不再向新上下文推帧 ⇒ 黑屏
// （SAF 竖屏往返复现，证据 s6-post-saf.png）。所以 rebind 只往队列里放一个新
// ANativeWindow，GL 线程在循环里换 EGL window surface（见 mpv_egl_surface）。

#include <cstdio>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "mpv/client.h"
#include "mpv_diag.hpp"
#include "mpv_egl_surface.hpp"
#include "mpv_video_output.hpp"
#include "mpv/render_gl.h"

namespace kivo::video::mpv {
namespace {

/// mpv 通过这个回调拿 GL 函数指针。签名由 mpv 定死。
void* get_proc_address(void* ctx, const char* name) {
    (void)ctx;
    return reinterpret_cast<void*>(eglGetProcAddress(name));
}

/// 步骤标记：渲染层诊断。落盘机制与共钟纪元见 mpv_diag.hpp
/// （mpv 自己的日志在 native 崩溃时会跟着断，且这台设备读不到 app 的 logcat）。
void mark_step(const char* path, const char* step) {
    kivo::video::mpv::mark_step(path, step, "RND");
}

}  // namespace

VideoOutput::VideoOutput(mpv_handle* handle, std::string log_path)
    : handle_(handle), log_path_(std::move(log_path)) {}

VideoOutput::~VideoOutput() {
    stop();
}

bool VideoOutput::start(ANativeWindow* window, std::string& error) {
    if (running_.load(std::memory_order_acquire)) {
        error = "video output already running";
        return false;
    }
    if (window == nullptr) {
        error = "surface is null";
        return false;
    }
    window_ = window;
    ANativeWindow_acquire(window);
    stopping_.store(false, std::memory_order_release);
    ready_.store(false, std::memory_order_release);
    failed_.store(false, std::memory_order_release);
    thread_ = std::thread([this] { run(); });

    // 等渲染线程把 EGL 与 mpv 渲染上下文准备好（或失败）。最多 2 秒。
    for (int i = 0; i < 200; ++i) {
        if (ready_.load(std::memory_order_acquire) || failed_.load(std::memory_order_acquire)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (failed_.load(std::memory_order_acquire)) {
        error = failure_;
        stop();
        return false;
    }
    if (!ready_.load(std::memory_order_acquire)) {
        error = "video output start timed out";
        stop();
        return false;
    }
    running_.store(true, std::memory_order_release);
    return true;
}

bool VideoOutput::set_window(ANativeWindow* window, std::string& error) {
    if (!running_.load(std::memory_order_acquire)) {
        error = "video output not running";
        return false;
    }
    if (window == nullptr) {
        error = "surface is null";
        return false;
    }
    ANativeWindow_acquire(window);
    std::lock_guard<std::mutex> lock(swap_mutex_);
    if (pending_window_ != nullptr) {
        ANativeWindow_release(pending_window_);
    }
    pending_window_ = window;
    return true;
}

void VideoOutput::stop() {
    stopping_.store(true, std::memory_order_release);
    if (thread_.joinable()) {
        thread_.join();
    }
    running_.store(false, std::memory_order_release);
    std::lock_guard<std::mutex> lock(swap_mutex_);
    if (pending_window_ != nullptr) {
        ANativeWindow_release(pending_window_);
        pending_window_ = nullptr;
    }
}

void VideoOutput::run() {
    EglWindowSurface egl;
    std::string error;
    if (!egl.init(window_, error)) {
        ANativeWindow_release(window_);
        window_ = nullptr;
        fail(error);
        return;
    }
    // EGL 已 retain 一份，start() 那份交还，所有权归 egl。
    ANativeWindow_release(window_);
    window_ = nullptr;

    mpv_render_context* render_context = nullptr;
    // mpv v0.39 的新签名：第三个参数是 **参数数组**，不是字符串。
    mpv_opengl_init_params gl_init{get_proc_address, nullptr};
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    if (mpv_render_context_create(&render_context, handle_, params) < 0) {
        // 【为什么失败也要 free】
        // mpv_render_context_create 内部可能已经建好了部分状态，
        // 只是渲染后端（vo）加载失败才返回错误 —— 此时 mpv 仍然要求
        // 对这个上下文调用 mpv_render_context_free()，
        // 否则 mpv 在 terminate 时报 "Broken API use" 并 abort。
        if (render_context != nullptr) {
            mpv_render_context_free(render_context);
        }
        fail("mpv_render_context_create failed");
        egl.shutdown();
        return;
    }

    ready_.store(true, std::memory_order_release);
    render_loop(egl, render_context);
}

void VideoOutput::apply_pending_swap(EglWindowSurface& egl) {
    ANativeWindow* next = nullptr;
    {
        std::lock_guard<std::mutex> lock(swap_mutex_);
        next = pending_window_;
        pending_window_ = nullptr;
    }
    if (next == nullptr) {
        return;
    }
    std::string error;
    if (egl.swap(next, error)) {
        mark_step(log_path_.c_str(), "window swapped");
    } else {
        mark_step(log_path_.c_str(), ("window swap failed: " + error).c_str());
    }
    ANativeWindow_release(next);
}

void VideoOutput::render_loop(EglWindowSurface& egl, mpv_render_context* render_context) {
    mark_step(log_path_.c_str(), "render-loop enter");
    int swap_count = 0;
    int zero_size_logged = 0;
    while (!stopping_.load(std::memory_order_acquire)) {
        apply_pending_swap(egl);
        // mpv v0.39：update 不再接收 flags 指针，直接返回 flags。
        const uint64_t flags = mpv_render_context_update(render_context);
        if (flags & MPV_RENDER_UPDATE_FRAME) {
            GLint viewport[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_VIEWPORT, viewport);
            width_.store(viewport[2], std::memory_order_relaxed);
            height_.store(viewport[3], std::memory_order_relaxed);

            // 【为什么要有 Surface 尺寸兜底】
            // Android 的 Surface 在 layout 完成前尺寸是 0，此时 GL_VIEWPORT 也是 0。
            // w/h 为 0 时 mpv 不知道该渲染到多大的画面 ⇒ 解码在跑但屏幕全黑。
            // ANativeWindow_getWidth/Height 直接问窗口，不依赖 GL 状态。
            int target_w = viewport[2];
            int target_h = viewport[3];
            if (target_w <= 0 || target_h <= 0) {
                target_w = ANativeWindow_getWidth(egl.window());
                target_h = ANativeWindow_getHeight(egl.window());
            }
            if (target_w <= 0 || target_h <= 0) {
                if (zero_size_logged < 3) {
                    ++zero_size_logged;
                    mark_step(log_path_.c_str(), "frame skipped: zero target size");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                continue;
            }

            // 【为什么必须传 FLIP_Y = 1】
            // Android Surface 的缓冲按图像约定（左上原点）被系统读取，而 GL
            // 渲染到 FBO 用 GL 约定（左下原点）。不翻转 ⇒ 上下颠倒。
            // render.h 写着 FLIP_Y "currently ignored"，但 mpv 0.39 的实现支持
            // （video/out/gpu/libmpv_gpu.c 读它）—— 文档与实现不一致，以实现为准。
            int flip_y = 1;
            mpv_opengl_fbo fbo{};
            fbo.fbo = 0;
            fbo.w = target_w;
            fbo.h = target_h;
            mpv_render_param render_params[] = {
                {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
                {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
                {MPV_RENDER_PARAM_INVALID, nullptr},
            };
            mpv_render_context_render(render_context, render_params);
            // 取证：render 后的 GL 错误码与真实 FBO 绑定，主/预览两内核同口径对照。
            if (swap_count < 6 || swap_count % 40 == 0) {
                GLint bound_fbo = -1;
                glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound_fbo);
                char rl[96];
                std::snprintf(rl, sizeof(rl), "post #%d err=0x%x fbo=%d vp=%dx%d",
                              swap_count, glGetError(), bound_fbo, target_w, target_h);
                mark_step(log_path_.c_str(), rl);
            }
            // 每帧都记 swap 结果：暂停态每次 seek 只出一帧，60 帧一记永远看不到。
            const EGLBoolean swapped = eglSwapBuffers(egl.display(), egl.surface());
            const EGLint swap_err = eglGetError();
            ++swap_count;
            // 【首帧的真实时刻】画面真正上屏 = swap 成功。
            // 之前 Kotlin 侧用"播放时钟起跳"当首帧，比这个早 1.5–2.8 s（真机实测）。
            if (swapped == EGL_TRUE) {
                first_frame_.store(true, std::memory_order_release);
            }
            char line[96];
            std::snprintf(line, sizeof(line), "swap #%d ok=%d err=0x%x target=%dx%d",
                          swap_count, static_cast<int>(swapped), swap_err, target_w, target_h);
            mark_step(log_path_.c_str(), line);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    mark_step(log_path_.c_str(), "render-loop exit");
    mpv_render_context_free(render_context);
    egl.shutdown();
}

void VideoOutput::fail(const std::string& reason) {
    failure_ = reason;
    failed_.store(true, std::memory_order_release);
    mark_step(log_path_.c_str(), reason.c_str());
}

std::unique_ptr<VideoOutput> create_video_output(mpv_handle* handle, std::string log_path) {
    return std::make_unique<VideoOutput>(handle, std::move(log_path));
}

}  // namespace kivo::video::mpv