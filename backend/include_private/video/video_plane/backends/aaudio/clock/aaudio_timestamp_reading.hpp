#pragma once

#include <cstdint>

namespace kivo::video::aaudio {

/// 一次 `AAudioStream_getTimestamp` 的结论。
///
/// 分类刻意比"成功/失败"更细，因为 NDK 文档明确写着两件事：
///
/// 1. "Timestamps are only valid when the stream is in STARTED.
///    `AAUDIO_ERROR_INVALID_STATE` will be returned if the stream is not started.
///    Note that because requestStart() is asynchronous, timestamps will not be valid
///    until a short time after calling requestStart().
///    **So `AAUDIO_ERROR_INVALID_STATE` should not be considered a fatal error.**"
///    —— 启动期那 24–146 ms（C1 实测）拿到的就是这个码，它是**正常现象**。
///    若把它计入失败并触发回退，证据里会凭空出现一堆"主源不可靠"的记录。
///
/// 2. `AAUDIO_ERROR_UNAVAILABLE` 的官方注释里就写着 "or a timestamp is not available"，
///    所以它也必须单独归类，不能和真的出错混在一起。
enum class AaudioTimestampStatus : std::int32_t {
    kOk = 0,
    kNotStarted = 1,     ///< INVALID_STATE：流还没进 STARTED，重试即可
    kUnavailable = 2,    ///< UNAVAILABLE：此刻拿不到时间戳
    kTimeout = 3,        ///< TIMEOUT：操作超时
    kDisconnected = 4,   ///< DISCONNECTED：设备断开，流已不可再用（致命，不是暂时性）
    kInvalidStream = 5,  ///< 空流指针（调用方 bug）
    kOther = 6,
};

[[nodiscard]] const char* aaudio_timestamp_status_name(AaudioTimestampStatus status) noexcept;

/// 纯数据，不含 `AAudioStream*` —— 平台类型只活在 reader 的 .cxx 里。
struct AaudioTimestampReading {
    AaudioTimestampStatus status = AaudioTimestampStatus::kOther;

    /// status == kOk 时有效。
    /// 语义（NDK 原文）："the time at which a particular frame **was played** on a speaker"，
    /// 即**已呈现**帧数，已含硬件延迟补偿；首帧是 0。
    std::int64_t frame_position = 0;
    std::int64_t time_nanos = 0;

    std::int32_t raw_result = 0;  ///< aaudio_result_t 原值，便于对照 NDK 查表

    [[nodiscard]] bool ok() const noexcept { return status == AaudioTimestampStatus::kOk; }

    /// 启动期/暂时性问题是"再等等"，不该触发回退；设备断开和脏指针才是真故障。
    [[nodiscard]] bool transient() const noexcept {
        return status == AaudioTimestampStatus::kNotStarted ||
               status == AaudioTimestampStatus::kUnavailable || status == AaudioTimestampStatus::kTimeout;
    }
};

}  // namespace kivo::video::aaudio
