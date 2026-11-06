#pragma once

#include <cstdint>

#include <media/NdkMediaFormat.h>

#include "video/video_plane/backends/mediacodec/decode/source_color_tags.hpp"

namespace kivo::video::mediacodec {

/// 轨道格式颜色的**读写层**（P12-E2）。
///
/// 取值的**含义**不在这里解释：那是 `decode/color_key_names.hpp` 与
/// `classify_hdr()` 的事。这里只负责"从 `AMediaFormat` 里读出来 / 写进去"——
/// 唯一碰平台类型的一层，所以它无法在桌面上单测（`AMediaFormat` 只在 Android 上存在）。
/// 这也是 `37-p12e2-design.md` §六 那条"复制逻辑单测"的实测更正：
/// 改为由设备端探针 `kivo_hdr_probe` 的 A/B 两次 configure 证明，见
/// `38-p12e2-status.md`。
/// 读轨道格式里的颜色标记。读不到的键保持 -1。
[[nodiscard]] SourceColorTags read_source_color_tags(const AMediaFormat* track_format);

/// 造一份"复制再写"的 configure 格式。**调用方负责 `AMediaFormat_delete`。**
///
/// 为什么不用 `AMediaFormat_copy`：那是 API 28 才导出的符号，而工程 minSdk = 26
/// （`probe/color_probe_main.cxx` 已按同样理由手写副本）。
///
/// 抄什么：`mime` / `width` / `height` / `csd-0` / `csd-1` / `max-input-size`，
/// 再加 `tags` 里存在的颜色键。**不补默认值** —— 源没有的键就不写，
/// 让平台按它自己的规则处理；补一个默认值等于替片源撒谎（见设计 §2.2）。
[[nodiscard]] AMediaFormat* make_configure_format(const AMediaFormat* track_format,
                                                 const SourceColorTags& tags);

/// 同 `make_configure_format`，但 mime 用调用方给的值（210 DV 透传）。
/// `mime_override` 非空时，重建格式里不抄轨道的 mime —— 8.x 的 BL 要以
/// `video/hevc` 的身份进 HEVC 解码器；其余键的取舍规则一字不差
/// （csd-2 与 DV 专有键本来就不在抄写清单里，正好是透传需要的干净格式）。
[[nodiscard]] AMediaFormat* make_configure_format_with_mime(const AMediaFormat* track_format,
                                                           const SourceColorTags& tags,
                                                           const char* mime_override);

/// 在"复制再写"的格式上加一个 `color-transfer-request`（P12-E4）。
///
/// **这条键与 `color-format` 是两件独立的事**：E1 已证这颗 OMX 组件在 configure 阶段
/// 根本不校验 `color-format`（连"请求 Surface 却不给窗口"都能成功），
/// 但 E2 证它**认** `color-standard/transfer/range` 三键（输出报 6/6/2 而非默认 1/3/2）。
/// 所以"能不能让它转 SDR"必须单独测，不能从 E1 的结论外推。
///
/// 键名字面量：`AMEDIAFORMAT_KEY_COLOR_TRANSFER_REQUEST` 是 API 28 才导出的常量。
[[nodiscard]] AMediaFormat* make_configure_format_with_transfer_request(
    const AMediaFormat* track_format, const SourceColorTags& tags,
    std::int32_t requested_transfer);

}  // namespace kivo::video::mediacodec
