#pragma once

#include <cstdint>
#include <string>

#include "video/video_plane/backends/mediacodec/decode/hdr_kind.hpp"

namespace kivo::video::mediacodec {

/// Surface dataspace 的设置结果。
///
/// **`applied == true` 只表示"我们把这个值设进去了"**，不表示合成器认账 ——
/// 后者要 SurfaceFlinger 侧的证据，属于 E3。
///
/// 归属：这个结构既是 `present/surface_dataspace` 的返回类型，也是
/// `session/decode_stats` 的证据字段，所以放在 `decode/` 的纯数据层，
/// 让上屏层与证据层都向下依赖，而不是互相依赖。
struct DataSpaceOutcome {
    bool applied = false;
    /// 实际设置的值（`ADATASPACE_*`）。未设置时 0。
    std::int32_t value = 0;
    /// 为什么没设。`applied == true` 时为空。
    std::string reason;
};

/// 窗口 dataspace 的**回读**结果（P12-E3）。
///
/// 与 `DataSpaceOutcome` 的区别必须记住：
/// - `DataSpaceOutcome.applied` = "我们调用成功、返回 0"；
/// - `DataSpaceReadback.ok/value` = "窗口上现在真的是这个值"。
///
/// **两者都不等于"合成器按 HDR 处理"** —— 那要看 SurfaceFlinger 的图层 dump
/// （`tools/probe_layer_dataspace.py`）。三层分开记，别互相顶替。
struct DataSpaceReadback {
    bool ok = false;
    std::int32_t value = 0;
    /// 为什么没读到。`ok == true` 时为空。
    std::string reason;
};

/// tone mapping 的实现状态（P12-E4）。
///
/// **这个结构存在的目的是"不假装支持"**：它让我们在证据里明确写出
/// "本轮没有做色调映射"，而不是让读者从一堆颜色字段里自己猜。
struct TonemapFacts {
    /// 本架构**始终为 false**：Surface 直出没有像素访问权。
    /// 真要做必须换 GL 路径，那是另一张卡（见 `41-p12e4-design.md` §2.3）。
    bool implemented = false;
    /// 为什么没做 / 为什么不需要。空串会被读成"没查"，所以一律给值。
    std::string reason;
};

/// 一次播放的 HDR 交付事实（P12-E2）。
///
/// 三段分开记，因为它们三个是**互相独立**的事实：
/// 1. 片源被判成什么（`kind` + `kind_reason`）；
/// 2. 容器里有没有静态元数据（`static_info_bytes`，-1 = 没有）；
/// 3. 我们有没有把 dataspace 设到窗口上（`dataspace`）。
///
/// 混成一个 bool 会让"识别到了但没设上"和"根本没识别"看起来一样。
struct HdrDeliveryFacts {
    HdrKind kind = HdrKind::kSdr;
    /// 判成这个 kind 的依据，或判不出来的原因。`kSdr` 也必须有值 ——
    /// 空的 reason 会被读成"没查"，而这张卡要的正是"查过并说得出依据"。
    std::string kind_reason;
    /// 轨道格式里 `hdr-static-info` 的字节数；-1 表示这个键不存在。
    /// 注意：SEI 里有、容器盒里没有是常态，见 `37-p12e2-design.md` §一。
    std::int32_t static_info_bytes = -1;
    DataSpaceOutcome dataspace;
    /// 窗口回读（P12-E3）。`dataspace.applied == true` 时才有意义；
    /// SDR 源我们不动窗口，回读值就是窗口原来的值 —— 也照记，免得读者以为没测。
    DataSpaceReadback dataspace_readback;
    /// 色调映射状态（P12-E4）。`implemented` 恒为 false —— 这里是事实，不是开关。
    TonemapFacts tonemap;
};

}  // namespace kivo::video::mediacodec
