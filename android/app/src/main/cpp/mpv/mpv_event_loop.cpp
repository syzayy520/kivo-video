// 253 家族：状态轮询层 —— **只做一件事**：把 mpv 的内部状态翻译成我们对外的事件。
//
// 【为什么不用 mpv 的事件队列】
// 真机实测（K40，3/3 复现）：在 `mpv_initialize` 之后、core 尚未跑起来时，
// **第二次** `mpv_wait_event(handle, ...)` 会让进程直接崩溃 ——
// 第一次成功、第二次崩，且与 timeout 是 0 还是 0.05 无关。
// 把事件线程改成完全不调 `mpv_wait_event`（只 sleep），崩溃消失（3/3 PASS）。
// 崩点因此确认在 mpv 的事件队列路径，不在我们的代码里。
//
// 【我们不依赖事件】
// 我们的上层（播放页）本来就有 100 ms 的 poll 循环（见 NetworkPlaybackSession）。
// 进度、时长、缓冲、暂停、结束，这些都能用 `mpv_get_property` 直接读出来 ——
// **不需要事件队列**。所以这里把"事件"重新定义成"状态跃迁检测"：
// 每次 poll 读几个关键属性，和上一次的快照比对，变了就发事件。
//
// 【代价与取舍】
// 代价：事件延迟从"mpv 立刻推"变成"最多一个 poll 周期"（100 ms）。
// 收益：不碰 mpv 的事件队列 ⇒ 没有崩溃；且状态来源单一（都是属性）。
// 落点、seek 完成等**需要精确时刻**的事，另有专门通道（见 mpv_commands.cpp）。

#include <chrono>
#include <string>

#include "mpv_session.hpp"
#include "mpv_session_impl.hpp"

namespace kivo::video::mpv {
namespace {

/// 上一次 poll 的状态快照。用来判断"状态跃迁"（变了才发事件）。
struct StateSnapshot {
    bool eof = false;
    bool paused = false;
    std::string path;
};

/// 读一个布尔属性。读不到（属性不存在/类型不符）返回 default。
bool read_flag(mpv_handle* handle, const char* name, bool default_value) {
    int64_t value = default_value ? 1 : 0;
    const int rc = mpv_get_property(handle, name, MPV_FORMAT_FLAG, &value);
    return rc < 0 ? default_value : (value != 0);
}

}  // namespace

void SessionImpl::run_event_loop() {
    // 事件线程只负责"按周期调 poll_events"，**不碰 mpv 的事件队列**。
    // 见文件头：那条路径在真机上会崩。
    StateSnapshot last;
    while (!stopping.load(std::memory_order_acquire)) {
        poll_events(last);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void SessionImpl::poll_events(StateSnapshot& last) {
    if (handle == nullptr) {
        return;
    }

    // 1) 文件路径变了 ⇒ 对应 mpv 的 file-loaded。
    std::string path;
    char* path_raw = mpv_get_property_string(handle, "path");
    if (path_raw != nullptr) {
        path = path_raw;
        mpv_free(path_raw);
    }
    if (!path.empty() && path != last.path) {
        last.path = path;
        if (event_callback) {
            event_callback("file-loaded", path);
        }
    }

    // 2) 播放结束。
    const bool eof = read_flag(handle, "eof-reached", false);
    if (eof && !last.eof) {
        last.eof = true;
        if (event_callback) {
            event_callback("end-file", "eof");
        }
    } else if (!eof && last.eof) {
        last.eof = false;
    }

    // 3) 暂停状态变化。
    const bool paused = read_flag(handle, "pause", false);
    if (paused != last.paused) {
        last.paused = paused;
        if (event_callback) {
            event_callback(paused ? "pause" : "unpause", "");
        }
    }
}

}  // namespace kivo::video::mpv
