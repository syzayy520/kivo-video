#pragma once

#include <cstdint>

namespace kivo::video::aaudio {

/// 单调时钟的纳秒读数，**必须**与 `AAudioStream_getTimestamp(CLOCK_MONOTONIC, ...)`
/// 返回的 `timeNanoseconds` 同一把尺子。
///
/// 为什么不用 `std::chrono::steady_clock`：标准只说它单调，**不保证**它就是
/// `CLOCK_MONOTONIC`。两者哪怕只差 1 ms，外推方向就会反（算出来的位置比采样点还早）。
/// libc++ 在 Android 上实现为 `CLOCK_MONOTONIC` 是事实，不是契约。
///
/// 平台的差异只在实现文件里，头文件保持纯声明 ——
/// 这样 `AaudioClockSource` 可以在桌面机器上编译并单测。
[[nodiscard]] std::int64_t monotonic_now_ns() noexcept;

}  // namespace kivo::video::aaudio
