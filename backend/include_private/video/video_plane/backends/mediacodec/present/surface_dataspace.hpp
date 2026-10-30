#pragma once

#include <cstdint>
#include <string>

#include "video/video_plane/backends/mediacodec/decode/hdr_delivery_facts.hpp"
#include "video/video_plane/backends/mediacodec/decode/hdr_kind.hpp"

struct ANativeWindow;

namespace kivo::video::mediacodec {

// `HdrKind` 与 `DataSpaceOutcome` 已下沉到 `decode/`（P12-E2）：
// 它们的产生者是颜色判定，`present/` 只是消费者。定义留在本文件会让
// `decode/` 反向依赖上屏层。这里 include 回来，调用方写法不变。

/// 按 HDR 类型设置窗口的 buffer dataspace。
///
/// SDR 源**不动**窗口（返回 `applied=false` + reason）：给 SDR 内容硬设 BT2020/PQ
/// 会让画面变得又灰又暗，那不是"支持 HDR"，那是把 SDR 弄坏。
///
/// 实现细节：`ANativeWindow_setBuffersDataSpace` 是 API 28 才有的符号，
/// 而工程 `minSdk = 26`，所以运行时用 `dlsym` 取 —— 取不到就退回 `applied=false`，
/// 不影响播放。
[[nodiscard]] DataSpaceOutcome apply_surface_dataspace(ANativeWindow* window, HdrKind kind);

/// 回读窗口当前的 buffer dataspace（P12-E3）。
///
/// `ANativeWindow_getBuffersDataSpace` 是 API 29 才有的符号，同样走
/// `dlopen("libnativewindow.so")`（与 set 同一条解析逻辑，位置也一样）。
///
/// **它证明的是"窗口上现在真是这个值"**，不证明合成器认账 —— 后者看
/// `tools/probe_layer_dataspace.py` 抓的 SurfaceFlinger 图层 dump。
[[nodiscard]] DataSpaceReadback read_surface_dataspace(ANativeWindow* window);

}  // namespace kivo::video::mediacodec
