#pragma once

#include <atomic>
#include <cstdint>

namespace kivo::video::mediacodec {

/// 一次 Surface 重建的事实。全部是时刻与计数，不含评价。
struct SurfaceRebuildStats {
    std::int32_t rebinds = 0;
    /// 最近一次：detach 调用 → rebind 调用。**这段是用户的**（切后台多久）。
    std::int64_t last_gap_ms = -1;
    /// 最近一次：rebind 调用 → 重建后第一帧真正交出。**这段是我们的**，判据 ≤ 500ms。
    std::int64_t last_resume_ms = -1;
    /// 最近一次：rebind 之后到首帧之间交出去的帧数（含被按节拍丢掉的）。
    /// 判据 ≤ 3 —— 卡片写的"3 帧内恢复"就是它。
    std::int32_t last_frames_lead = 0;
};

/// 换面的时刻跟踪（P12-D1）。
///
/// 为什么单独一个类而不是塞进 `DecodePipelineCounters`：那个是热路径的纯原子计数、
/// 每秒被读好几次；这里的状态机只在换面时走一遍，语义完全不同。
/// 混进去会让"每帧加一"和"一年走两次"的两类东西共用一套读法。
///
/// 线程模型：`note_detach` / `note_rebind` 在调用线程（JNI 线程）；
/// `note_release` 在收帧线程。全部走原子量，不加锁 —— 收帧线程每帧都要过一次。
class SurfaceRebuildTracker {
public:
    /// Surface 没了（或主动切到 ByteBuffer 模式）。
    void note_detach(std::int64_t mono_ns) noexcept;

    /// 新 Surface 挂上了，开始等第一帧。
    void note_rebind(std::int64_t mono_ns) noexcept;

    /// 收帧段每交出一个缓冲后调用。`rendered == false` 表示这一帧被丢掉或没面可上。
    void note_release(std::int64_t mono_ns, bool rendered) noexcept;

    [[nodiscard]] SurfaceRebuildStats snapshot() const noexcept;

private:
    enum Phase : std::int32_t {
        kIdle = 0,      ///< 没有正在进行的重建
        kDetached = 1,  ///< 面已摘掉，等新面
        kAwaitingFirstFrame = 2,  ///< 面已挂上，等第一帧
    };

    std::atomic<std::int32_t> phase_{kIdle};
    std::atomic<std::int64_t> detach_at_mono_ns_{-1};
    std::atomic<std::int64_t> rebind_at_mono_ns_{-1};
    std::atomic<std::int64_t> first_frame_mono_ns_{-1};
    std::atomic<std::int32_t> rebinds_{0};
    std::atomic<std::int32_t> pending_frames_{0};
    std::atomic<std::int64_t> last_gap_ms_{-1};
    std::atomic<std::int64_t> last_resume_ms_{-1};
    std::atomic<std::int32_t> last_frames_lead_{0};
};

}  // namespace kivo::video::mediacodec
