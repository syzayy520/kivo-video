#pragma once

#include <android/native_window.h>

namespace kivo::video::mediacodec {

/// ANativeWindow 的 RAII 持有者。
///
/// 所有权约定：构造时接管一个**已 acquire** 的窗口指针，析构时 release。
/// 从 Java Surface 取窗口的动作（ANativeWindow_fromSurface）留在 JNI 层，
/// 因为那一步需要 JNIEnv —— 平台边界就该画在那里。
///
/// 为什么必须 RAII：解码器持有窗口期间若提前 release，硬解会直接崩在
/// 厂商驱动里，且堆栈完全不可读。把生命周期绑在对象上是最省事的防线。
class SurfaceWindowBinder {
public:
    explicit SurfaceWindowBinder(ANativeWindow* window);
    ~SurfaceWindowBinder();

    SurfaceWindowBinder(const SurfaceWindowBinder&) = delete;
    SurfaceWindowBinder& operator=(const SurfaceWindowBinder&) = delete;
    SurfaceWindowBinder(SurfaceWindowBinder&&) = delete;
    SurfaceWindowBinder& operator=(SurfaceWindowBinder&&) = delete;

    [[nodiscard]] ANativeWindow* get() const { return window_; }
    [[nodiscard]] bool valid() const { return window_ != nullptr; }

    /// 主动放弃窗口（例如 Java 侧 Surface 已销毁）。
    void reset();

private:
    ANativeWindow* window_ = nullptr;
};

}  // namespace kivo::video::mediacodec
