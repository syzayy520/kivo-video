// 253 家族：离屏取帧后端实现。接口与"为什么不用 ImageReader"见 mpv_gl_capture.hpp。
//
// 【与 mpv_gl_render 的分工】mpv_gl_render 把画面**上屏**（EGL window surface +
// eglSwapBuffers）；本文件把画面**读回 CPU**（pbuffer + 纹理 FBO + glReadPixels）。
// 两者共用 EglWindowSurface 建上下文，但 surface 类型与消费方式完全不同 ——
// 真机已证 window→ImageReader 那条传输坏，读回必须走 FBO。

#include "mpv_gl_capture.hpp"

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "mpv/client.h"
#include "mpv/render_gl.h"
#include "mpv_diag.hpp"
#include "mpv_egl_surface.hpp"

namespace kivo::video::mpv {
namespace {

void* get_proc_address(void* ctx, const char* name) {
    (void)ctx;
    return reinterpret_cast<void*>(eglGetProcAddress(name));
}

void mark_step(const char* path, const char* step) {
    kivo::video::mpv::mark_step(path, step, "CAP");
}

}  // namespace

FrameCapture::FrameCapture(mpv_handle* handle, std::string log_path)
    : handle_(handle), log_path_(std::move(log_path)) {}

FrameCapture::~FrameCapture() { stop(); }

bool FrameCapture::start(int width, int height, std::string& error) {
    if (running_.load(std::memory_order_acquire)) {
        error = "capture already running";
        return false;
    }
    if (width <= 0 || height <= 0) {
        error = "bad capture size";
        return false;
    }
    width_ = width;
    height_ = height;
    stopping_.store(false, std::memory_order_release);
    ready_.store(false, std::memory_order_release);
    failed_.store(false, std::memory_order_release);
    thread_ = std::thread([this] { run(); });
    for (int i = 0; i < 200; ++i) {
        if (ready_.load(std::memory_order_acquire) || failed_.load(std::memory_order_acquire)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (failed_.load(std::memory_order_acquire)) {
        error = failure_;
        stop();
        return false;
    }
    if (!ready_.load(std::memory_order_acquire)) {
        error = "capture start timed out";
        stop();
        return false;
    }
    running_.store(true, std::memory_order_release);
    return true;
}

void FrameCapture::stop() {
    stopping_.store(true, std::memory_order_release);
    if (thread_.joinable()) thread_.join();
    running_.store(false, std::memory_order_release);
}

bool FrameCapture::read_frame(std::vector<uint8_t>& out) {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    if (seq_ == 0 || seq_ == last_read_) return false;
    last_read_ = seq_;
    out = frame_;
    return true;
}

double FrameCapture::frame_pts() {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    return frame_pts_;
}

void FrameCapture::run() {
    EglWindowSurface egl;
    std::string error;
    if (!egl.init_offscreen(width_, height_, error)) {
        fail(error);
        return;
    }
    mpv_render_context* ctx = nullptr;
    mpv_opengl_init_params gl_init{get_proc_address, nullptr};
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    if (mpv_render_context_create(&ctx, handle_, params) < 0) {
        // 失败也要 free：mpv 可能已建好部分状态，不 free 会在 terminate 时 abort。
        if (ctx != nullptr) mpv_render_context_free(ctx);
        fail("mpv_render_context_create failed (capture)");
        egl.shutdown();
        return;
    }
    ready_.store(true, std::memory_order_release);
    render_loop(egl, ctx);
}

void FrameCapture::render_loop(EglWindowSurface& egl, mpv_render_context* ctx) {
    mark_step(log_path_.c_str(), "capture-loop enter");
    GLuint tex = 0;
    GLuint fbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    const bool fbo_ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!fbo_ok) mark_step(log_path_.c_str(), "capture FBO incomplete");

    std::vector<uint8_t> pixels(static_cast<size_t>(width_) * height_ * 4);
    int frame_count = 0;
    while (fbo_ok && !stopping_.load(std::memory_order_acquire)) {
        const uint64_t flags = mpv_render_context_update(ctx);
        if (flags & MPV_RENDER_UPDATE_FRAME) {
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glViewport(0, 0, width_, height_);
            // FLIP_Y=1：mpv 把图像顶行存到 GL y=0，glReadPixels  row0=顶 ⇒ top-down。
            int flip_y = 1;
            mpv_opengl_fbo target{};
            target.fbo = static_cast<int>(fbo);
            target.w = width_;
            target.h = height_;
            mpv_render_param rp[] = {
                {MPV_RENDER_PARAM_OPENGL_FBO, &target},
                {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
                {MPV_RENDER_PARAM_INVALID, nullptr},
            };
            mpv_render_context_render(ctx, rp);
            // mpv 渲染内部会绑/解自己的 FBO，结束后绑定的不一定是我们的 ——
            // 读回前必须重新绑回目标 FBO，否则 glReadPixels 读到空缓冲（真机 px0=0）。
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            // 记录本帧对应的播放位置：上层据此把帧和 seek 目标对齐（防 off-by-one）。
            double pts = -1;
            if (char* ps = mpv_get_property_string(handle_, "time-pos")) {
                pts = std::strtod(ps, nullptr);
                mpv_free(ps);
            }
            {
                std::lock_guard<std::mutex> lock(frame_mutex_);
                frame_ = pixels;
                frame_pts_ = pts;
                ++seq_;
            }
            ++frame_count;
            if (frame_count <= 6 || frame_count % 40 == 0) {
                char line[96];
                std::snprintf(line, sizeof(line), "cap #%d err=0x%x px0=%02x%02x%02x%02x",
                              frame_count, glGetError(), pixels[0], pixels[1], pixels[2], pixels[3]);
                mark_step(log_path_.c_str(), line);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    mark_step(log_path_.c_str(), "capture-loop exit");
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    mpv_render_context_free(ctx);
    egl.shutdown();
}

void FrameCapture::fail(const std::string& reason) {
    failure_ = reason;
    failed_.store(true, std::memory_order_release);
    mark_step(log_path_.c_str(), reason.c_str());
}

std::unique_ptr<FrameCapture> create_frame_capture(mpv_handle* handle, std::string log_path) {
    return std::make_unique<FrameCapture>(handle, std::move(log_path));
}

}  // namespace kivo::video::mpv