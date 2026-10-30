#pragma once

#include <array>
#include <cstdint>

#include "video/video_plane/core/pace/pace_drift_series.hpp"

namespace kivo::video::core::pace {

/// 最差那一帧的**前因后果**（P13-P）。
///
/// P13-F 的 `DriftExtreme` 只回答"最差在哪"（`index` / `media_ns`）。
/// 但同一个 `lateness` 尖峰有两种完全相反的成因，位置分不出来：
///
///   · **手慢了**：交出动作本身晚了 88 ms —— 线程被抢占 / 解码与呈现同线程被拖住；
///   · **表快了**：画面按时交，可"按时"的基准错了 —— 目标时刻被算早了 88 ms。
///
/// 两者在 `lateness` 上长得一模一样，区别只在下面这条逐帧恒等式里：
///
///     lateness[k] − lateness[k−1] = present_step[k] − due_step[k]
///     present_step[k] = (present[k] − present[k−1]) − frame_duration   ← `present_jitter` 的样本
///     due_step[k]     =  due[k] − (due[k−1] + frame_duration)          ← 本结构新增
///
/// 所以要把 `due_step`、前一帧的 `lateness`、后几帧的 `lateness` 一起出版：
/// 只有 `max` 一个数时，这条恒等式无从下手（论证见 `86-p13p-design.md` §1.2）。
///
/// 后 8 帧是为了分开两种形状：
///   · 一步跳变 → `after` 立刻回到基线；
///   · 持续位移 → `after` 递减若干帧才收敛（时钟回锁）。
/// 喂给记录器的一帧。字段一多就必须成结构 —— 十个裸 `int64_t` 排成一串，
/// 调用方错一个位置编译器只会报"类型都对"，而不报"你传反了"。
struct EpisodeSample {
    std::int64_t lateness_ns = 0;
    std::int64_t media_ns = 0;
    std::int64_t due_step_ns = 0;
    std::int64_t cumulative_present_step_ns = 0;
    std::int64_t cumulative_due_step_ns = 0;
    std::int64_t lateness_at_first_ns = 0;
    /// P13-R：本帧时钟读数与目标时刻的原始值。
    std::int64_t clock_mono_ns = 0;
    std::int64_t clock_elapsed_ns = 0;
    std::int64_t anchor_pts_ns = 0;
    std::int64_t due_at_ns = 0;
};

struct WorstEpisode {
    /// 最差帧之前看几帧。恒等式只需要 1 帧，多留一帧用于分辨"上一帧本身也在爬坡"。
    static constexpr std::int32_t kBefore = 2;
    /// 最差帧之后看几帧。60 fps 下 8 帧 ≈ 133 ms，够看完一次 88 ms 位移的收敛。
    static constexpr std::int32_t kAfter = 8;

    /// false 表示这个会话一帧都没呈现。此时其余字段无意义（不是"零"）。
    bool valid = false;
    /// 时间正序的第几个呈现样本（0 基），**全场累计**。
    std::int32_t index = 0;
    /// 呈现样本总数，`summary()` 回填（在别处数会与 `lateness.count()` 分家）。
    std::int32_t of = 0;
    std::int64_t media_ns = 0;
    std::int64_t lateness_ns = 0;
    /// 该帧 `due` 相对"上一帧 `due` + 一帧"的偏差。负 = 目标时刻被算早了（表快了）。
    /// 首帧没有"上一帧"，恒为 0（锚点在这一帧建立，见 `frame_pacer.cxx`）。
    std::int64_t due_step_ns = 0;

    /// 从**首帧到本帧**两侧各自累积了多少（P13-Q）。
    ///
    /// 单帧的 `due_step_ns` 只看得见"一步"，看不见"慢慢挪"。真机上
    /// `p13d-4k60-r1` 的 88.231 ms 就是这种：几帧里每帧挪一帧左右，
    /// 每一步都 ≤ 16.667 ms，于是 `due_step_frames` 记 0，而 `lateness` 从 88 掉到 5。
    ///
    /// 两个累积量与 `lateness_at_first_ns` 一起满足（逐项相加，无近似）：
    ///
    ///     lateness[k] − lateness_at_first = cumulative_present_step − cumulative_due_step
    ///
    /// 这是本卡唯一一条**跨帧**的恒等式：两侧各自独立测量（`present` 来自时间戳，
    /// `due` 来自钟），所以它能抓到单帧快照抓不到的缓慢位移。
    std::int64_t cumulative_present_step_ns = 0;
    std::int64_t cumulative_due_step_ns = 0;
    /// 首帧的 `lateness`。锚点就在那里建立，所以它只有"醒来 → 交出"那几微秒 ——
    /// 出版它是为了让上面那条恒等式可以**当场复算**，而不是让人去翻 `frame_pacer.cxx`。
    std::int64_t lateness_at_first_ns = 0;

    /// 最差帧当时的钟读数与目标时刻（P13-R）。
    ///
    /// 正常分支的定义式 `due = mono + (pts − anchor_pts − elapsed)` 让这三个数
    /// 与 `media_ns` / `due_at_ns` 一起**可复算**：`due_step` 到底是从"钟走快了"
    /// 还是"帧到得早"冒出来的，不用再猜。
    std::int64_t clock_mono_ns = 0;
    std::int64_t clock_elapsed_ns = 0;
    std::int64_t anchor_pts_ns = 0;
    std::int64_t due_at_ns = 0;

    /// 之前的 `lateness`（时间正序，末尾是紧邻的那一帧）。不足时 `before_count` 更小。
    std::array<std::int64_t, kBefore> before_ns{};
    std::int32_t before_count = 0;
    /// 之后的 `lateness`（时间正序，开头是紧邻的那一帧）。会话结束时可能不满。
    std::array<std::int64_t, kAfter> after_ns{};
    std::int32_t after_count = 0;

    /// 只取位置四格，给仍然按 `DriftExtreme` 读的调用方（报告里的 `worst` 块）。
    [[nodiscard]] DriftExtreme position() const noexcept;
};

/// 「当前最差帧 + 前后邻帧」的累计记录器。
///
/// 为什么单独一个类而不是塞进 `PaceStats`：它有一套自己的状态机
/// （当前最差、前 2 帧的环、后 8 帧的填充、新最差来了要重开），
/// 塞进去会让 `PaceStats` 从"一堆计数器"变成"计数器 + 一个小状态机"。
/// 单独成文件还有一个好处：这一套可以脱离帧循环单测（`t01`–`t06`）。
///
/// 线程模型与 `PaceStats` 一致：非线程安全，只在呈现线程里用。
class WorstEpisodeRecorder {
public:
    /// 喂一帧。参数与 `PaceStats::note_presented()` 同源，必须逐帧调用、顺序调用。
    ///
    /// 喂一帧。
    ///
    /// 参数多到十来个之后一律走结构体（`EpisodeSample`）：十个裸 `int64_t`
    /// 排成一串时，传反两个的编译错误是"类型都对，我不说哪个错"。
    void note(const EpisodeSample& sample) noexcept;

    [[nodiscard]] const WorstEpisode& episode() const noexcept { return episode_; }
    [[nodiscard]] std::int32_t samples() const noexcept { return samples_; }

private:
    WorstEpisode episode_{};
    /// 最近 `kBefore` 帧的 `lateness` 环形缓存。只为了在新最差出现时能回填 `before`。
    std::array<std::int64_t, WorstEpisode::kBefore> recent_{};
    std::int32_t recent_count_ = 0;
    std::int32_t samples_ = 0;
};

}  // namespace kivo::video::core::pace
