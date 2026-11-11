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

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <android/native_window.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "mpv/client.h"
#include "mpv_video_output.hpp"
#include "mpv/render_gl.h"

namespace kivo::video::mpv {
namespace {

/// EGL 配置：RGBA8888 + ES2 + 可录制到 Surface（Android 必需 EGL_RECORDABLE_ANDROID）。
constexpr EGLint kEglConfigAttribs[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RECORDABLE_ANDROID, 1,
    EGL_NONE,
};

/// mpv 通过这个回调拿 GL 函数指针。签名由 mpv 定死。
void* get_proc_address(void* ctx, const char* name) {
    (void)ctx;
    return reinterpret_cast<void*>(eglGetProcAddress(name));
}

}  // namespace

namespace {

/// 步骤标记：渲染层的诊断。mpv 自己的日志在 native 崩溃时会跟着断，
/// 而且这台设备读不到 app 的 logcat —— 所以渲染层自己往文件里写。
const char kNewline = 10;

void mark_step(const char* path, const char* step) {
    if (path == nullptr || path[0] == 0) {
        return;
    }
    const int fd = ::open(path, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd < 0) {
        return;
    }
    const std::string line = std::string("RND ") + step;
    (void)write(fd, line.data(), line.size());
    (void)write(fd, &kNewline, 1);
    ::close(fd);
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

void VideoOutput::stop() {
    stopping_.store(true, std::memory_order_release);
    if (thread_.joinable()) {
        thread_.join();
    }
    running_.store(false, std::memory_order_release);
    if (window_ != nullptr) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }
}

void VideoOutput::run() {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        fail("eglGetDisplay failed");
        return;
    }
    if (!eglInitialize(display, nullptr, nullptr)) {
        fail("eglInitialize failed");
        return;
    }
    EGLConfig config = nullptr;
    EGLint config_count = 0;
    if (!eglChooseConfig(display, kEglConfigAttribs, &config, 1, &config_count) || config_count <= 0) {
        fail("eglChooseConfig failed");
        return;
    }
    EGLContext context = EGL_NO_CONTEXT;
    // 【必须显式指定 GLES 版本】
    // 不指定时 EGL 默认给 **GLES 1.1**，而 mpv 的 render API 要求 **ES 2.0+** ——
    // 真机日志：`GL_VERSION='OpenGL ES-CM 1.1'` + `OpenGL not initialized.`
    // ⇒ mpv_render_context_create 直接失败。
    const EGLint kContextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, kContextAttribs);
    if (context == EGL_NO_CONTEXT) {
        fail("eglCreateContext failed");
        return;
    }
    EGLSurface surface = eglCreateWindowSurface(display, config, window_, nullptr);
    if (surface == EGL_NO_SURFACE) {
        fail("eglCreateWindowSurface failed");
        return;
    }
    if (!eglMakeCurrent(display, surface, surface, context)) {
        fail("eglMakeCurrent failed");
        return;
    }

        mpv_render_context* render_context = nullptr;
        // mpv v0.39 的新签名：第三个参数是 **参数数组**，不是字符串。
        //   MPV_RENDER_PARAM_API_TYPE          → 用哪种渲染 API（"opengl"）
        //   MPV_RENDER_PARAM_OPENGL_INIT_PARAMS → OpenGL 初始化参数（含 get_proc_address）
        // mpv_opengl_init_params 只有两个字段：get_proc_address 与它的 ctx。
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
                render_context = nullptr;
            }
            fail("mpv_render_context_create failed");
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglDestroySurface(display, surface);
            eglDestroyContext(display, context);
            eglTerminate(display);
            return;
        }

    ready_.store(true, std::memory_order_release);
    render_loop(display, surface, context, render_context);
}

void VideoOutput::render_loop(void* display_ptr, void* surface_ptr, void* context_ptr,
                              void* render_context_ptr) {
    EGLDisplay display = static_cast<EGLDisplay>(display_ptr);
    EGLSurface surface = static_cast<EGLSurface>(surface_ptr);
    EGLContext context = static_cast<EGLContext>(context_ptr);
    auto* render_context = static_cast<mpv_render_context*>(render_context_ptr);

    while (!stopping_.load(std::memory_order_acquire)) {
        // mpv v0.39：update 不再接收 flags 指针，直接返回 flags。
        const uint64_t flags = mpv_render_context_update(render_context);
        if (flags & MPV_RENDER_UPDATE_FRAME) {
            // 目标尺寸取当前视口（Surface 的尺寸由 Android 决定）。
            GLint viewport[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_VIEWPORT, viewport);
            width_.store(viewport[2], std::memory_order_relaxed);
            height_.store(viewport[3], std::memory_order_relaxed);

            // 【为什么要有 Surface 尺寸兜底】
            // Android 的 Surface 在 layout 完成前尺寸是 0，此时 GL_VIEWPORT 也是 0。
            // 而 mpv 的 mpv_opengl_fbo 用 w/h 描述输出目标 —— w/h 为 0 时
            // mpv 不知道该渲染到多大的画面，结果就是**解码在跑、GPU 纹理在更新，
            // 但屏幕上什么都没有**（真机实测：播放页显示"正在播放"，画面全黑）。
            // ANativeWindow_getWidth/Height 直接问窗口，不依赖 GL 状态，更可靠。
            int target_w = viewport[2];
            int target_h = viewport[3];
            if (target_w <= 0 || target_h <= 0) {
                target_w = ANativeWindow_getWidth(window_);
                target_h = ANativeWindow_getHeight(window_);
            }
            if (target_w <= 0 || target_h <= 0) {
                // 尺寸还没确定：这一帧跳过，等 Surface layout 完再画。
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                continue;
            }

            // render 同样用参数数组：MPV_RENDER_PARAM_OPENGL_FBO 指定渲染目标。
            mpv_opengl_fbo fbo{};
            fbo.fbo = 0;
            fbo.w = target_w;
            fbo.h = target_h;
            mpv_render_param render_params[] = {
                {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
                {MPV_RENDER_PARAM_INVALID, nullptr},
            };
            mpv_render_context_render(render_context, render_params);
            eglSwapBuffers(display, surface);

            // 诊断：每 60 帧记一次实际用的目标尺寸。
            // 【为什么需要】画面黑时，唯一能区分"Surface 尺寸为 0"与
            // "mpv 没画"的办法就是把尺寸打出来（这台设备读不到 app 的 logcat，
            // 所以只能写文件）。
            if (++frame_count_ >= 60) {
                frame_count_ = 0;
                char line[96];
                std::snprintf(line, sizeof(line), "RND render vp=%dx%d target=%dx%d",
                              viewport[2], viewport[3], target_w, target_h);
                mark_step(log_path_.c_str(), line);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    mpv_render_context_free(render_context);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
}

void VideoOutput::fail(const std::string& reason) {
    failure_ = reason;
    failed_.store(true, std::memory_order_release);
}

std::unique_ptr<VideoOutput> create_video_output(mpv_handle* handle, std::string log_path) {
    return std::make_unique<VideoOutput>(handle, std::move(log_path));
}

}  // namespace kivo::video::mpv
