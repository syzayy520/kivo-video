// 253 家族：会话实现 —— **只做一件事**：mpv 句柄的创建 / 初始化 / 绑定 Surface / 销毁。
//
// 拆分理由（家族树 §二）：命令下发、事件循环、渲染各有各的线程与语义，
// 都在各自的 .cpp 里（mpv_commands / mpv_event_loop / mpv_video_output）。
// 本文件**不含**任何命令、事件或渲染逻辑。

#include "mpv_session.hpp"

#include <android/native_window.h>

#include "mpv_diag.hpp"
#include "mpv_options.hpp"
#include "mpv_session_impl.hpp"
#include "mpv_video_output.hpp"

namespace kivo::video::mpv {
namespace {

/// 步骤标记：落盘机制与共钟纪元见 mpv_diag.hpp（native 崩溃时 mpv 自己的日志
/// 会跟着断，定位崩溃点只能靠这份原始 write 的诊断）。
void mark_step(const char* log_file, const char* step) {
    kivo::video::mpv::mark_step(log_file, step, "STEP");
}

}  // namespace

bool Session::attach_surface(ANativeWindow* window, std::string& error) {
    if (!impl_ || impl_->handle == nullptr) {
        error = "session not created";
        return false;
    }
    if (window == nullptr) {
        error = "surface is null";
        return false;
    }
    // 渲染层归本会话所有（不是全局静态）—— 理由见 mpv_session_impl.hpp 里
    // "为什么不能是全局"的说明：mpv_render_context_free() 必须早于
    // mpv_terminate_destroy()，否则 mpv 会 abort（真机实测：
    // "Broken API use: mpv_render_context_free() not called."）。
    //
    // 已在跑 ⇒ 只换 EGL window surface。重建渲染上下文会让 mpv 停止推帧
    // （真机黑屏实测），所以 rebind 绝不走 create+start。
    if (impl_->video_output && impl_->video_output->running()) {
        mark_step(impl_->log_path.c_str(), "attach-surface rebind (swap window)");
        return impl_->video_output->set_window(window, error);
    }
    impl_->video_output = create_video_output(impl_->handle, impl_->log_path);
    if (!impl_->video_output) {
        error = "cannot create video output";
        return false;
    }
    mark_step(impl_->log_path.c_str(), "attach-surface enter");
    const bool ok = impl_->video_output->start(window, error);
    mark_step(impl_->log_path.c_str(), ok ? "attach-surface ok" : ("attach-surface failed: " + error).c_str());
    return ok;
}

bool Session::attach_capture(int width, int height, std::string& error) {
    if (!impl_ || impl_->handle == nullptr) {
        error = "session not created";
        return false;
    }
    if (impl_->frame_capture && impl_->frame_capture->running()) {
        error = "capture already running";
        return false;
    }
    impl_->frame_capture = create_frame_capture(impl_->handle, impl_->log_path);
    if (!impl_->frame_capture) {
        error = "cannot create frame capture";
        return false;
    }
    mark_step(impl_->log_path.c_str(), "attach-capture enter");
    const bool ok = impl_->frame_capture->start(width, height, error);
    mark_step(impl_->log_path.c_str(), ok ? "attach-capture ok" : ("attach-capture failed: " + error).c_str());
    return ok;
}

bool Session::read_capture_frame(std::vector<uint8_t>& out) {
    return impl_ && impl_->frame_capture && impl_->frame_capture->read_frame(out);
}

double Session::capture_frame_pts() {
    return impl_ && impl_->frame_capture ? impl_->frame_capture->frame_pts() : -1.0;
}

int Session::video_width() const {
    return impl_ && impl_->video_output ? impl_->video_output->width() : 0;
}

int Session::video_height() const {
    return impl_ && impl_->video_output ? impl_->video_output->height() : 0;
}

Session::~Session() {
    shutdown();
}

std::unique_ptr<Session> Session::create(std::string& error) {
    auto session = std::unique_ptr<Session>(new Session());
    session->impl_ = std::make_unique<SessionImpl>();

    mpv_handle* handle = mpv_create();
    if (handle == nullptr) {
        error = "mpv_create failed";
        return nullptr;
    }
    session->impl_->handle = handle;

    // 【不注册 wakeup 回调】
    // 它的唯一用途是打断阻塞中的 mpv_wait_event —— 而那条路径在真机上会崩
    // （见 mpv_event_loop.cpp）。我们现在用属性轮询，不需要它。
    // 不注册 ⇒ mpv 不会在 core 未就绪时回调我们 ⇒ 也少一个潜在崩溃点。
    return session;
}

void Session::set_event_callback(EventCallback callback) {
    if (impl_ && impl_) {
        impl_->event_callback = std::move(callback);
    }
}

void Session::set_render_callback(RenderCallback callback) {
    if (impl_ && impl_) {
        impl_->render_callback = std::move(callback);
    }
}

bool Session::initialize(const std::string& vo, const std::string& ao,
                        const std::string& log_file, std::string& error) {
    if (!impl_ || impl_->handle == nullptr) {
        error = "session not created";
        return false;
    }
    mpv_handle* handle = impl_->handle;
    impl_->log_path = log_file;
    mark_step(log_file.c_str(), "enter-initialize");

    // ---- 选项设置全部交给 mpv_options.cpp ----
    //
    // 【为什么要搬走】选项设置曾散在本文件里，且**丢弃了 mpv_set_option_string
    // 的返回值**。mpv 对不存在的选项只返回错误码、不打印任何日志，于是
    // 拼错的选项会完全静默 —— 上一轮就因此把"字幕没显示"误判成了
    // "OSD 定位问题"，实际是几个选项压根没设进去。
    // 现在：选项必须**要么生效、要么报错**，见 mpv_options.hpp 的判据。
    mark_step(log_file.c_str(), "before-options");
    std::string hwdec;
    std::string option_error;
    if (!apply_mpv_options(handle, vo, ao, log_file, hwdec, option_error)) {
        error = option_error;
        mark_step(log_file.c_str(), "options-rejected");
        return false;
    }
    // 有降级项（optional 选项失败）时把原因带出去 —— 上层要能看见，
    // 不能只在 native 里默默吞掉。
    if (!option_error.empty()) {
        impl_->degraded_options = option_error;
    }
    impl_->hwdec_ = hwdec;
    mark_step(log_file.c_str(), "after-options");

    mark_step(log_file.c_str(), "before-mpv_initialize");
    int rc = mpv_initialize(handle);
    mark_step(log_file.c_str(), "after-mpv_initialize");
    if (rc < 0) {
        error = "mpv_initialize failed: " + std::string(mpv_error_string(rc));
        return false;
    }

    // hwdec 必须在 initialize **之后**设（见 mpv_options.cpp 里的说明）。
    if (!impl_->hwdec_.empty()) {
        mpv_set_property_string(handle, "hwdec", impl_->hwdec_.c_str());
    }

    impl_->event_thread = std::thread([state = impl_.get()] {
        state->run_event_loop();
    });
    return true;
}

std::string Session::error() const {
    // 【为什么以前恒返回空串】那是因为"失败原因"只走 initialize() 的出参。
    // 但**降级**发生在成功路径上 —— 只有一个出参就装不下。
    // 现在把降级原因也带上来（见 SessionImpl::degraded_options）。
    if (!impl_) {
        return std::string();
    }
    return impl_->degraded_options;
}

void Session::shutdown() {
    if (!impl_ || !impl_) {
        return;
    }
    auto& state = impl_;
    state->stopping.store(true, std::memory_order_release);
    if (state->event_thread.joinable()) {
        state->wake_cv.notify_all();
        state->event_thread.join();
    }
    // 【顺序要紧】先销毁渲染层（它会调 mpv_render_context_free），
    // 再销毁 mpv 句柄。mpv 要求 free 早于 terminate_destroy，
    // 否则报 "Broken API use: mpv_render_context_free() not called." 并 abort。
    if (state->video_output) {
        state->video_output->stop();
        state->video_output.reset();
    }
    if (state->frame_capture) {
        state->frame_capture->stop();
        state->frame_capture.reset();
    }
    if (state->handle != nullptr) {
        mpv_terminate_destroy(state->handle);
        state->handle = nullptr;
    }
    impl_.reset();
}

}  // namespace kivo::video::mpv
