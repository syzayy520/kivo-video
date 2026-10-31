#pragma once

#include <cstdint>

namespace kivo::video::mediacodec {

/// 一个会话是**怎么结束**的。三值，不是布尔。
///
/// ## 为什么 `completed` + `error` 两格不够
///
/// 报告里原本只有两格：`completed`（bool）与 `error`（字符串）。两者之间有一格落空 ——
/// 「停了、没报错、也没跑完」。层 3 把这一格明确判为**静默失败**，而且判得对：
/// 在 P14-R 之前，它只有一个来源 —— 某条链忘了写 reason。
///
/// 但"被打断"不是失败。屏幕熄灭、切后台、用户退出，都是**正常收尾**，
/// 只是没走到 EOS。塞进 `error` 会污染故障统计（`completed=false` 于是永远带着
/// 一句其实不是错误的错误）；不写又会变成静默失败。所以补第三值，让这一格有名字。
///
/// 实测：`docs/evidence` 里 214 份 `kivo.p12c.sync/*` 全部 `completed: true`，
/// 零份 `false`。这个状态从来没进过证据 —— 不是因为它不可能，
/// 是因为**没有路径能产出它**（写了就会被读成静默失败）。
enum class SessionEndReason : std::int32_t {
    /// 走到 EOS。`frames_received` 里那一帧 EOS 缓冲只在此时存在。
    kCompleted = 0,
    /// 被外部停掉：没有 EOS，也没有错误。
    kInterrupted = 1,
    /// 出错中止。`error` 非空。
    kFailed = 2,
};

/// 派生规则，**只此一处**。
///
/// 两个报告（`decode/19` 的 `outcome.end_reason`、`sync/29` 的顶层 `end_reason`）
/// 都要写这一格。各写一个 `if` 就是两份记忆，而本工程已经为"两份记忆"付过账
/// （`SYNC_QUANTILE_BLOCK_SCHEMAS` 的两张表只更新一张、S2/S3 的重叠区）。
///
/// `has_error` 由调用方从**它自己那份报告**的 `error` 派生：
/// 解码侧是 `video.error`，同步侧是 `video.error` 与 `audio.error` 里的非空者。
/// 所以两侧的 `completed` 口径也必须各用各的 —— 同步侧的 `completed` 是
/// `video.completed && audio.error.empty()`，不是 `video.completed`。
[[nodiscard]] constexpr SessionEndReason session_end_reason(bool completed,
                                                           bool has_error) noexcept {
    if (has_error) {
        return SessionEndReason::kFailed;
    }
    return completed ? SessionEndReason::kCompleted : SessionEndReason::kInterrupted;
}

/// 写进 JSON 的那个词。
///
/// 未知值回落到 `"failed"` 而不是 `"completed"`：一个认不出来的收尾方式
/// **不许**被读成"跑完了"。判据那一侧按字符串白名单校验，所以这个回落
/// 只会让一份报告变红，不会让它变绿。
[[nodiscard]] constexpr const char* session_end_reason_name(SessionEndReason reason) noexcept {
    switch (reason) {
        case SessionEndReason::kCompleted:
            return "completed";
        case SessionEndReason::kInterrupted:
            return "interrupted";
        case SessionEndReason::kFailed:
            return "failed";
    }
    return "failed";
}

}  // namespace kivo::video::mediacodec
