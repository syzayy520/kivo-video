#pragma once

// 253 家族：EGL 窗口 surface 生命周期 —— 只做创建 / 换面 / 销毁一件事。
//
// 【为什么独立成文件】Surface 重绑（SAF 竖屏往返、横竖屏切换）必须**只换
// EGL window surface，不重建 mpv 渲染上下文**：真机实测旧上下文 free 后再
// create，mpv 不再向新上下文推帧（黑屏，证据 s6-post-saf.png + .diag 里
// 第二段 render-loop 无 first-frame）。mpv-android 的做法同样是渲染上下文
// 不动、换 surface。
//
// 【线程】全部调用都在 GL 线程上；本类自己不加锁。
// 【所有权】init/swap 进来的 ANativeWindow 由本类 retain，shutdown 时 release。

#include <EGL/egl.h>
#include <android/native_window.h>

#include <string>

namespace kivo::video::mpv {

class EglWindowSurface {
public:
    EglWindowSurface() = default;
    ~EglWindowSurface() { shutdown(); }
    EglWindowSurface(const EglWindowSurface&) = delete;
    EglWindowSurface& operator=(const EglWindowSurface&) = delete;

    /// 建立 display/config/context/window surface 并 makeCurrent。
    [[nodiscard]] bool init(ANativeWindow* window, std::string& error);

    /// 离屏变体：pbuffer surface（无窗口），供 FBO 读回取帧用。context 同 init。
    [[nodiscard]] bool init_offscreen(int width, int height, std::string& error);

    /// 换窗口 surface（context 不变）。失败保留旧 surface，原因进 error。
    [[nodiscard]] bool swap(ANativeWindow* window, std::string& error);

    void shutdown();

    EGLDisplay display() const { return display_; }
    EGLSurface surface() const { return surface_; }
    ANativeWindow* window() const { return window_; }

private:
    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLConfig config_ = nullptr;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLSurface surface_ = EGL_NO_SURFACE;
    ANativeWindow* window_ = nullptr;
};

}  // namespace kivo::video::mpv