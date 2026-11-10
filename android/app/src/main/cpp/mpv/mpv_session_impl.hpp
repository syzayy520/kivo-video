#pragma once

// 253 家族：**内部**实现头。家族内私有，不对外暴露。
//
// 为什么需要它：家族里多个 .cpp 都要访问同一个 mpv 句柄与事件同步原语
// （session 管生命周期、event_loop 管事件、commands 管命令、video_output 管渲染）。
// 这些实现细节共享同一份状态，但**对外接口**只有一个（mpv_session.hpp）。
//
// 【家族边界】
//   B4 跨文件调用一律走 mpv_session.hpp（对外）或本文件（家族内部）
//   B5 每个 .cpp ≤ 300 行

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "mpv/client.h"
#include "mpv_video_output.hpp"

namespace kivo::video::mpv {

/// mpv 事件名 → 对外暴露的小写名字。
///
/// mpv 的事件名是大写常量（MPV_EVENT_*），翻译成小写下划线风格便于 Kotlin 侧消费。
/// **归本文件是因为它只服务事件层**：命令层与渲染层不碰事件名。
[[nodiscard]] std::string translate_event_name(mpv_event_id id);

/// 会话内部状态。**只在家族内部可见**。
struct SessionImpl {
    mpv_handle* handle = nullptr;

    std::function<void(const std::string& name, const std::string& payload)> event_callback;
    std::function<void(int width, int height)> render_callback;

    /// 崩溃定位用的日志文件路径（可选）。事件线程与初始化阶段都往这里写步骤标记。
    std::string log_path;
    /// 硬解驱动名。在 initialize **之后**才真正 set_property 给 mpv
    /// （见 mpv_session.cpp 里关于"set_option 会被默认值覆盖"的说明）。
    std::string hwdec_;

    /// mpv 的唤醒机制：mpv 侧有事件时调用它，唤醒我们的事件线程。
    std::mutex wake_mutex;
    std::condition_variable wake_cv;
    std::atomic<bool> wake_pending{false};
    std::atomic<bool> stopping{false};
    std::thread event_thread;

    /// mpv 要求：唤醒回调不能持有我们自己的锁，只置标志 + notify。
    static void wakeup(void* ctx) {
        auto* self = static_cast<SessionImpl*>(ctx);
        self->wake_pending.store(true, std::memory_order_release);
        self->wake_cv.notify_one();
    }

    /// 视频输出层。**必须由本会话持有**（不能是全局静态）。
    ///
    /// 【为什么不能是全局】
    /// mpv 要求 `mpv_render_context_free()` 必须在 `mpv_terminate_destroy()` **之前**调用
    /// （否则 mpv 报 "Broken API use: mpv_render_context_free() not called." 并 abort）。
    /// 若渲染层是全局静态，它的析构时机就与 mpv 句柄的销毁顺序脱钩 —— 真机上正是这样崩的。
    /// 所以：会话持有它，shutdown() 里先销毁它，再销毁 mpv 句柄。
    std::unique_ptr<VideoOutput> video_output;

    /// 事件线程主体。**只在事件层跑**（见 mpv_event_loop.cpp）。
    void run_event_loop();

    /// 状态跃迁检测用的快照。用来判断"变了才发事件"。
    struct StateSnapshot {
        bool eof = false;
        bool paused = false;
        std::string path;
    };

    /// 读关键属性并与快照比对，变了就发事件。**不碰 mpv 的事件队列**
    /// （那条路径在真机上会崩，见 mpv_event_loop.cpp 的说明）。
    void poll_events(StateSnapshot& last);
};

}  // namespace kivo::video::mpv
