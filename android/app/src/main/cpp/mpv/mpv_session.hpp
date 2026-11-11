#pragma once

// 253 家族：libmpv 播放内核 —— 家族内唯一的接口头。
//
// 【家族边界】（见 docs/design/backend-core/253-mpv-kernel-family-tree.md）
//   B1 本家族不得引用 backend/src/video/video_plane/backends/ 的任何头文件
//   B2 本家族不得直接调用 FFmpeg API（那是 mpv 的职责）
//   B3 本家族不得碰 Android UI（上层用 Kotlin 订阅回调）
//   B4 跨文件调用一律走本头文件
//   B5 每个 .cpp ≤ 300 行
//
// 【为什么自己再包一层，而不是直接在 JNI 里调 mpv_*】
//   mpv 的 client API 是**同步阻塞**的（mpv_wait_event 会挂住）。
//   JNI 线程不能被挂住 —— 否则 ANR，且上层拿不到任何进度。
//   所以本家族把 mpv 的事件循环搬到一个**自己的线程**上，
//   用"唤醒回调 + 条件变量"把事件转成 Kotlin 侧的回调。

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

struct ANativeWindow;

namespace kivo::video::mpv {

/// 会话内部状态。**家族内部**（见 mpv_session_impl.hpp）。
struct SessionImpl;

/// 一个内核会话的对外能力。**唯一**入口是 `Session::create()`。
///
/// 线程模型：
///   · `Session` 本身不是线程安全的 —— 所有方法都必须在**同一个**控制线程调用
///     （我们从 JNI 侧用一把锁串起来）。
///   · 内部有一个**事件线程**（`EventLoop`），它读 mpv 事件并转成回调。
///   · 视频渲染走 mpv 自己的 `render` 回调，在 GL 线程上被调用。
class Session {
public:
    /// 事件回调。`name` 是 mpv 的事件名（"file-loaded"/"end-file"/…）。
    using EventCallback = std::function<void(const std::string& name, const std::string& payload)>;

    /// 渲染回调（在 mpv 的渲染线程上被调用）。`width/height` 是目标尺寸。
    using RenderCallback = std::function<void(int width, int height)>;

    Session() = default;
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;

    /// 创建会话。失败时 `error()` 里有原因。
    [[nodiscard]] static std::unique_ptr<Session> create(std::string& error);

    /// 把 mpv 的事件回调挂上（会话创建后、initialize 之前调）。
    void set_event_callback(EventCallback callback);

    /// 设置渲染回调。**必须在 initialize 之前**调 —— mpv 的渲染上下文
    /// 在 `mpv_render_context_create` 时就要知道有没有渲染器。
    void set_render_callback(RenderCallback callback);

    /// 初始化 mpv 核心。
    /// @param vo mpv 的视频输出名（Android 上用 "libmpv"）。
    /// @param ao mpv 的音频输出名；空 = "null"（不出声，见实现里的说明）。
    /// @param log_file mpv 日志落盘路径；空 = 不落盘。
    [[nodiscard]] bool initialize(const std::string& vo, const std::string& ao,
                                 const std::string& log_file, std::string& error);

    /// 绑定一个 Android Surface 作为视频输出目标。
    ///
    /// 内部会起一个**独立的 GL 线程**（EGL 上下文与 Surface 都绑定在那个线程）。
    /// 上层不需要（也不能）自己碰 GL。
    [[nodiscard]] bool attach_surface(ANativeWindow* window, std::string& error);

    /// 当前视频尺寸（像素）；还不知道时返回 0。
    [[nodiscard]] int video_width() const;
    [[nodiscard]] int video_height() const;

    /// 打开一个 URL（可以是本地路径，也可以是 `smb://` / `http://`）。
    [[nodiscard]] bool load_file(const std::string& url, std::string& error);

    /// 下发一条 mpv 命令。`args` 是命令名之后的参数（已按 mpv 的格式序列化）。
    [[nodiscard]] bool command(const std::string& name, const std::string& args, std::string& error);

    /// 跳转到 `position_seconds`。
    /// `exact` 为真时用 mpv 的精确 seek（`hr-seek=exact`），会解到目标帧。
    [[nodiscard]] bool seek(double position_seconds, bool exact, std::string& error);

    /// 读一个属性（字符串形式）。
    [[nodiscard]] bool property(const std::string& name, std::string& out, std::string& error);

    /// 渲染一帧到已绑定的 Surface。`mpv_render_context_render` 的薄封装。
    [[nodiscard]] bool render(std::string& error);

    /// 当前播放位置（秒）。读不到返回 -1。
    [[nodiscard]] double position_seconds();

    /// 总时长（秒）。读不到（媒体还没加载完）返回 -1。
    [[nodiscard]] double duration_seconds();

    /// 最后一帧错误信息。
    [[nodiscard]] std::string error() const;

    /// 释放。**必须在所有渲染回调停止之后**调用。
    void shutdown();

private:
    std::unique_ptr<SessionImpl> impl_;
};
}  // namespace kivo::video::mpv
