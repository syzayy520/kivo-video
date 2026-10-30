#pragma once

#include <cstdint>

namespace kivo::video::core::pipeline {

/// 送包段这一轮该做什么。
///
/// 只有五种，且互斥。它是**结论**，不是过程 —— 过程发生在 `SubmitSequencer` 里。
enum class SubmitStep : std::int32_t {
    kReadSample = 0,     ///< 手上没有待送包，去解封装器读下一包
    kSubmitPending = 1,  ///< 手上有包，送它（送失败/输入满时下次仍然是它）
    kSignalEos = 2,      ///< 轨道读完，发一次 EOS
    kFinished = 3,       ///< EOS 已送出，送包段收工
    kAbort = 4,          ///< 遇到不可继续的错误（读失败 / 加密包被拒）
};

[[nodiscard]] const char* submit_step_name(SubmitStep step) noexcept;

}  // namespace kivo::video::core::pipeline
