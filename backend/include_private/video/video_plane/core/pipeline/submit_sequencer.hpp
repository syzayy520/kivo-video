#pragma once

#include <cstdint>

#include "video/video_plane/core/pipeline/submit_types.hpp"

namespace kivo::video::core::pipeline {

/// 送包时序状态机：只回答"下一轮该读、该送、该发 EOS，还是该收工"。
///
/// 为什么值得单独成一个单元：送包段有两条**踩过就丢数据**的规矩，
/// 而它们都藏在一个循环里、靠人读代码守：
///
/// 1. **送失败（输入满）之后不许读下一包。** 读一次就推进了解封装器的读指针，
///    那一包永久丢失。AAC 一包约 21ms，丢几包就是可听的断续（C3 的结论）。
/// 2. **EOS 只发一次，且 pts 取已送出包的最大值。** 发第二次会被平台当成
///    又一帧配置数据；用"最后一包"的 pts 会在 B 帧重排时倒退。
///
/// 把这两条写成状态迁移，就能在桌面上断言，不必等真机丢帧才发现。
///
/// 线程模型：非线程安全，只在送包线程里用。
class SubmitSequencer {
public:
    SubmitSequencer() = default;

    /// 读到一包。`pts_us` 只用于 EOS 的 pts 与诊断。
    void note_sample(std::int64_t pts_us) noexcept;

    /// 这一包**确实**进了平台输入队列。只有送成功才调它。
    void note_submitted() noexcept;

    /// 解封装器报告轨道结束。
    void note_end_of_stream() noexcept;

    /// EOS 包确实进了平台输入队列。
    void note_eos_signaled() noexcept;

    /// 遇到不可继续的错误。`reason` 用静态字面量，热路径不碰 `std::string`。
    void note_aborted(const char* reason) noexcept;

    [[nodiscard]] SubmitStep next() const noexcept;

    [[nodiscard]] bool has_pending() const noexcept { return has_pending_; }

    /// 待送包的 pts（微秒）。`has_pending() == false` 时无意义。
    [[nodiscard]] std::int64_t pending_pts_us() const noexcept { return pending_pts_us_; }

    /// EOS 包该用的 pts：已送出包的最大值。没有送过包时为 0。
    ///
    /// 取最大值而不是"最后一包"，是因为 B 帧重排会让容器顺序与显示顺序不一致；
    /// 平台拿 EOS 的 pts 去挂时间轴，倒退的 pts 会让它判成"时间倒流"。
    [[nodiscard]] std::int64_t eos_presentation_us() const noexcept {
        return max_submitted_pts_us_;
    }

    [[nodiscard]] bool aborted() const noexcept { return aborted_; }

    /// 终止原因。未终止时为空串。
    [[nodiscard]] const char* abort_reason() const noexcept { return abort_reason_; }

private:
    bool has_pending_ = false;
    std::int64_t pending_pts_us_ = 0;
    std::int64_t max_submitted_pts_us_ = 0;

    bool eos_ready_ = false;     ///< 解封装器说读完了
    bool eos_signaled_ = false;  ///< EOS 包已进输入队列
    bool aborted_ = false;

    const char* abort_reason_ = "";
};

}  // namespace kivo::video::core::pipeline
