#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace kivo::video::mediacodec {

/// 会话收尾的每一步。**顺序就是语义** —— 它记的是"谁必须先死"，不是执行细节。
///
/// 这张表里的每一条都对应一次真机故障，改顺序之前先读
/// `31-p12d4-design.md` §二 的表：
/// 1. 停视频 → 2. 取此刻音频位置 → 3. 停音频 → 4. 放会话 → 5. 放音频
/// → 6. 放窗口 → 7. 放时钟
enum class SessionTeardownStep : std::int32_t {
    /// 停收帧/送包线程。之后没有人再往节拍器里塞帧，节拍器才可以被读。
    kStopVideo = 0,
    /// 取"视频停下这一刻"的音频位置。**必须在停音频之前** —— 停流之后
    /// `getTimestamp` 退回回退档，读到的是收尾开销而不是播放位置（P12-C8 踩过）。
    kSampleAudioPosition = 1,
    /// 停音频：排空环形缓冲，并把 `feed` / `output` / `clock` 三组统计写成终值。
    /// 省掉这一步，`kivo.p12c.sync/1` 的音频段全是 0、`clock_source` 退回 `system`。
    kStopAudio = 2,
    /// 放会话。节拍器持有 `AudioClockSource*` 裸指针，会话死得早就是悬空指针。
    kResetVideo = 3,
    /// 放音频。时钟在 `AudioPlayback` 内部，音频先死 = 时钟悬空。
    kResetAudio = 4,
    /// 放窗口。解码器（在会话内）还指着 `ANativeWindow*`，窗口先放就是 P12-D1 那个崩。
    kResetWindow = 5,
    /// 放时钟。`SystemClockSource` 被节拍器裸指针持有 —— 它是最后一个走的。
    kResetClock = 6,
};

/// 完整收尾序列（销毁走这条）。
///
/// 设计成 `constexpr` 数组而不是三段代码，是这张卡的全部意义：
/// 顺序成了数据，单测才能盯住它；否则它只活在两个人的记忆里。
inline constexpr std::array<SessionTeardownStep, 7> kSessionDestroyPlan{
    SessionTeardownStep::kStopVideo,
    SessionTeardownStep::kSampleAudioPosition,
    SessionTeardownStep::kStopAudio,
    SessionTeardownStep::kResetVideo,
    SessionTeardownStep::kResetAudio,
    SessionTeardownStep::kResetWindow,
    SessionTeardownStep::kResetClock,
};

/// 停止只走前三步。
///
/// **它是上面那份计划的严格前缀，不是另一份列表。** 两份列表可以各自演化，
/// "顺序唯一"就成了口号；做成前缀之后，任何一方改动都会让单测变红。
inline constexpr std::size_t kSessionStopPlanSize = 3;

[[nodiscard]] constexpr std::span<const SessionTeardownStep> session_destroy_plan() noexcept {
    return kSessionDestroyPlan;
}

[[nodiscard]] constexpr std::span<const SessionTeardownStep> session_stop_plan() noexcept {
    return std::span<const SessionTeardownStep>(kSessionDestroyPlan.data(), kSessionStopPlanSize);
}

[[nodiscard]] constexpr const char* session_teardown_step_name(SessionTeardownStep step) noexcept {
    switch (step) {
        case SessionTeardownStep::kStopVideo:
            return "stop_video";
        case SessionTeardownStep::kSampleAudioPosition:
            return "sample_audio_position";
        case SessionTeardownStep::kStopAudio:
            return "stop_audio";
        case SessionTeardownStep::kResetVideo:
            return "reset_video";
        case SessionTeardownStep::kResetAudio:
            return "reset_audio";
        case SessionTeardownStep::kResetWindow:
            return "reset_window";
        case SessionTeardownStep::kResetClock:
            return "reset_clock";
    }
    return "unknown";
}

/// 按计划执行收尾。
///
/// `Hooks` 需要七个同名成员函数：`stop_video` / `sample_audio_position` /
/// `stop_audio` / `reset_video` / `reset_audio` / `reset_window` / `reset_clock`。
///
/// 为什么用模板而不是 `std::function` 表：这份设计要的是**执行只有一份**。
/// 换成可替换对象，"计划对但代码另写一套"就又能藏回来了。
template <typename Hooks>
void run_session_teardown(std::span<const SessionTeardownStep> plan, Hooks& hooks) {
    for (const SessionTeardownStep step : plan) {
        switch (step) {
            case SessionTeardownStep::kStopVideo:
                hooks.stop_video();
                break;
            case SessionTeardownStep::kSampleAudioPosition:
                hooks.sample_audio_position();
                break;
            case SessionTeardownStep::kStopAudio:
                hooks.stop_audio();
                break;
            case SessionTeardownStep::kResetVideo:
                hooks.reset_video();
                break;
            case SessionTeardownStep::kResetAudio:
                hooks.reset_audio();
                break;
            case SessionTeardownStep::kResetWindow:
                hooks.reset_window();
                break;
            case SessionTeardownStep::kResetClock:
                hooks.reset_clock();
                break;
        }
    }
}

}  // namespace kivo::video::mediacodec
