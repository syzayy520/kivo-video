// 253 家族：EGL 窗口 surface 生命周期。接口与理由见 mpv_egl_surface.hpp。

#include "mpv_egl_surface.hpp"

#include <EGL/eglext.h>

namespace kivo::video::mpv {
namespace {

/// RGBA8888 + ES2。【为什么去掉 EGL_RECORDABLE_ANDROID】真机红清屏二分定论
/// （v31s）：带 recordable 时 Adreno 650 给 ImageReader 分配"编码器友好"缓冲，
/// GPU 颜色写入不 resolve —— eglSwapBuffers 成功、timestamp 推进，但缓冲永远是
/// gralloc 填充色（自绘红清屏都到不了 ImageReader，同一循环红清屏却能上主屏）。
/// recordable 只对 MediaCodec 输入面有意义，本项目从不录像 ⇒ 去掉，主屏不受影响。
constexpr EGLint kConfigAttribs[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_NONE,
};

/// 不显式指定时 EGL 默认给 ES 1.1，而 mpv render API 要求 ES 2.0+。
constexpr EGLint kContextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

}  // namespace

bool EglWindowSurface::init(ANativeWindow* window, std::string& error) {
    if (window == nullptr) {
        error = "surface is null";
        return false;
    }
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) {
        error = "eglGetDisplay failed";
        return false;
    }
    if (!eglInitialize(display_, nullptr, nullptr)) {
        error = "eglInitialize failed";
        return false;
    }
    EGLint count = 0;
    if (!eglChooseConfig(display_, kConfigAttribs, &config_, 1, &count) || count <= 0) {
        error = "eglChooseConfig failed";
        return false;
    }
    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, kContextAttribs);
    if (context_ == EGL_NO_CONTEXT) {
        error = "eglCreateContext failed";
        return false;
    }
    surface_ = eglCreateWindowSurface(display_, config_, window, nullptr);
    if (surface_ == EGL_NO_SURFACE) {
        error = "eglCreateWindowSurface failed";
        return false;
    }
    if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
        error = "eglMakeCurrent failed";
        return false;
    }
    window_ = window;
    ANativeWindow_acquire(window);
    return true;
}

bool EglWindowSurface::init_offscreen(int width, int height, std::string& error) {
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) {
        error = "eglGetDisplay failed";
        return false;
    }
    if (!eglInitialize(display_, nullptr, nullptr)) {
        error = "eglInitialize failed";
        return false;
    }
    // 离屏要 PBUFFER_BIT 的 config（window 那份是 WINDOW_BIT，不能混用）。
    const EGLint pb_attribs[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_NONE,
    };
    EGLint count = 0;
    if (!eglChooseConfig(display_, pb_attribs, &config_, 1, &count) || count <= 0) {
        error = "eglChooseConfig(pbuffer) failed";
        return false;
    }
    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, kContextAttribs);
    if (context_ == EGL_NO_CONTEXT) {
        error = "eglCreateContext failed";
        return false;
    }
    const EGLint pb_surf[] = {EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE};
    surface_ = eglCreatePbufferSurface(display_, config_, pb_surf);
    if (surface_ == EGL_NO_SURFACE) {
        error = "eglCreatePbufferSurface failed";
        return false;
    }
    if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
        error = "eglMakeCurrent failed";
        return false;
    }
    return true;
}

bool EglWindowSurface::swap(ANativeWindow* window, std::string& error) {
    if (window == nullptr) {
        error = "surface is null";
        return false;
    }
    EGLSurface next = eglCreateWindowSurface(display_, config_, window, nullptr);
    if (next == EGL_NO_SURFACE) {
        error = "eglCreateWindowSurface failed";
        return false;
    }
    if (!eglMakeCurrent(display_, next, next, context_)) {
        eglDestroySurface(display_, next);
        error = "eglMakeCurrent failed";
        return false;
    }
    eglDestroySurface(display_, surface_);
    surface_ = next;
    ANativeWindow_acquire(window);
    ANativeWindow_release(window_);
    window_ = window;
    return true;
}

void EglWindowSurface::shutdown() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
        }
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
        }
        eglTerminate(display_);
    }
    display_ = EGL_NO_DISPLAY;
    context_ = EGL_NO_CONTEXT;
    surface_ = EGL_NO_SURFACE;
    if (window_ != nullptr) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }
}

}  // namespace kivo::video::mpv