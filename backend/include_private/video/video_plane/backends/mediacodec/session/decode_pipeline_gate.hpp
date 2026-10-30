#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace kivo::video::mediacodec {

/// 两段之间唯一的三件事：**停不停**、**输入有没有腾出空位**、**输入段收工了没有**。
///
/// 刻意只给这三个：跨线程每多传一个信号，就多一处"两边理解不一致"的地方。
///
/// 为什么不需要"输出可能有了"这个信号：收帧段收不到帧时直接在平台上阻塞
/// （`dequeueOutputBuffer(timeout)`），平台自己就是最好的通知源，
/// 我们再加一层信号反而多一条会丢的路径。
///
/// 线程安全：所有方法可在任意线程调用。
class DecodePipelineGate {
public:
    [[nodiscard]] bool stop_requested() const noexcept {
        return stop_.load(std::memory_order_acquire);
    }

    /// 请求停止并唤醒所有等待者。可重复调用。
    void request_stop() noexcept;

    /// 送包段在输入满（`TRY_AGAIN`）之后等空位。
    ///
    /// @return true = 等到了（或被叫醒），false = 超时或已请求停止。
    ///         **两条路都必须重试送包**：丢掉这一包等于丢一帧，还可能丢关键帧。
    [[nodiscard]] bool wait_for_input_room(std::int64_t timeout_us) noexcept;

    /// 收帧段每次 `release_frame` 之后调用：有输出槽归还，输入槽大概率也能再塞。
    void signal_input_room() noexcept;

    /// 送包段发完 EOS 后调用。收帧段用它判断"再没有新包了"，
    /// 从而把"等下一帧"和"解码器卡死"分开。
    void note_submit_finished() noexcept;
    [[nodiscard]] bool submit_finished() const noexcept {
        return submit_finished_.load(std::memory_order_acquire);
    }

    /// `FramePacer` 只认一个原子停止标志，这里把同一个量交出去，
    /// 免得多出一个"节拍器自己的停止量"要同步。
    [[nodiscard]] const std::atomic<bool>* stop_flag() const noexcept { return &stop_; }

    /// 起新一轮之前清账。只允许在没有 worker 运行时调用。
    void reset_for_start() noexcept;

private:
    std::atomic<bool> stop_{false};
    std::atomic<bool> submit_finished_{false};

    std::mutex mutex_;
    std::condition_variable room_cv_;
};

}  // namespace kivo::video::mediacodec
