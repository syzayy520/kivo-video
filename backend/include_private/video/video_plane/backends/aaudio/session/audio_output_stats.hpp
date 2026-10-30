#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::aaudio {

/// 一次音频输出会话的事实计数。
///
/// 与 `DecodeStats` 同一约定：只记事实，不含评价。是否达标由证据层按 G6/G7 判定，
/// 会话层只如实记录 —— 一旦在这里写"是否合格"，数字就有了立场。
struct AudioOutputStats {
    std::int64_t callbacks = 0;        ///< 数据回调次数
    std::int64_t frames_rendered = 0;  ///< 交给设备的帧数（含补的静音）
    std::int64_t frames_silenced = 0;  ///< 要不到数据、被迫补静音的帧数

    /// 首次从数据源拿到**真实数据**时，已渲染到第几帧（该块的起始序号）。
    /// -1 表示整场都没拿到过真实数据（全程静音）。
    ///
    /// 这个字段是为 C3 的遗留问题加的：`frames_silenced` 实测 128/320/198 帧，
    /// 能确定发生在"起流或收尾的瞬间"，但光看总数**分不清是头还是尾** ——
    /// 而这两种情况的含义完全不同（头部 = 预灌不足，尾部 = 排空太快）。
    std::int64_t first_audible_frame = -1;

    /// 由 `first_audible_frame` 与 `frames_silenced` 直接分解出来的两个事实。
    /// 不在这里做"是否正常"的判断，只把账算清。
    std::int64_t silence_head_frames = 0;  ///< 首块真实数据之前的静音帧数
    std::int64_t silence_tail_frames = 0;  ///< 之后补的静音帧数

    /// 本次会话的欠载次数（已减去开流时的累计基线）。
    /// 非 0 就说明供给跟不上，别处怎么优化都白搭。
    std::int32_t xrun_count = 0;

    std::int64_t wall_millis = 0; ///< 从 start 到查询的墙钟
    bool started = false;
    std::string error;            ///< 非空表示会话已失败
};

}  // namespace kivo::video::aaudio
