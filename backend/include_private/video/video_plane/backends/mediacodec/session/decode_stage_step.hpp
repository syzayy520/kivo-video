#pragma once

#include <cstdint>

namespace kivo::video::mediacodec {

/// 流水线里一段跑一轮的结果。两段（送包 / 收帧）与编排者共用这一套词汇，
/// 免得编排层要靠"返回 bool 再猜含义"来拼流程。
enum class StageStep : std::int32_t {
    /// 这一轮做了实事（送出一包 / 收走一帧），可以立刻再来一轮。
    kProgress = 0,

    /// 这一轮没东西可做（输入满 / 暂时没帧）。两段都在同一个线程里跑时，
    /// 这个值就是"该让另半段跑一轮"的信号。
    kIdle = 1,

    /// 这一段收工了（EOS 已发出 / 已收到 EOS 帧），或者已终止。
    /// 终止原因一定已经写进失败槽，不会只有这个枚举而说不出原因。
    kDone = 2,
};

}  // namespace kivo::video::mediacodec
