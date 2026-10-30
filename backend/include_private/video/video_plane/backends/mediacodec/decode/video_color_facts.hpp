#pragma once

#include <cstdint>
#include <string>

#include "video/video_plane/backends/mediacodec/decode/hdr_kind.hpp"

namespace kivo::video::mediacodec {

/// 一次播放的"颜色事实"：源侧是什么、我们下发了什么、平台给了什么。
///
/// 全是整数，**不含 `AMediaFormat`** —— 取值在 `decode/` 里做，判定在这里做。
/// 这样判定就能在桌面上被单测，而不用起一个解码器。
///
/// 三段字段按发生顺序排：`source_*`（configure 前，来自轨道格式）→
/// `delivered_*`（configure 时我们写进去的）→ 输出侧（`start()` 之后平台报的）。
/// 三段必须分开存：源有标记而我们没发下去、发了平台不认、平台自己改了，是三件事。
struct VideoColorFacts {
    std::string mime;                  ///< "video/hevc" / "video/avc" / 其他
    /// 轨道格式里的 `profile`（`AMEDIAFORMAT_KEY_PROFILE` 的字面量形式）。
    /// 拿不到写 -1 —— 未知与"等于 0"必须分开。
    std::int32_t source_profile = -1;

    // ---- 源侧（P12-E2）：轨道格式里带的颜色键。拿不到一律 -1 ----
    std::int32_t source_color_standard = -1;
    std::int32_t source_color_transfer = -1;
    std::int32_t source_color_range = -1;
    /// 轨道格式里 `hdr-static-info` 的字节数；-1 表示这个键不存在。
    /// 存在不等于"我们下发了" —— 下发结果看 `delivered_*`。
    std::int32_t hdr_static_info_bytes = -1;

    // ---- 下发事实（P12-E2）：我们写进 configure 格式里的值 ----
    /// -1 表示这个键**没有写**（源里没有，且我们不补默认值）。
    std::int32_t delivered_color_standard = -1;
    std::int32_t delivered_color_transfer = -1;
    std::int32_t delivered_color_range = -1;
    /// true = configure 用的是"复制再写"的格式；false = 原样传的轨道格式。
    bool configure_format_copied = false;
    /// 复制格式 configure 失败、回退到原格式时的原因。空 = 没有回退。
    std::string configure_fallback_reason;

    // ---- 输出侧：`start()` 之后平台报的 ----
    /// `AMediaCodec_getOutputFormat()` 里的 `color-format`。拿不到写 -1。
    std::int32_t negotiated_format = -1;
    std::int32_t color_standard = -1;  ///< `color-standard`，HDR 判据（E3 用）
    std::int32_t color_transfer = -1;  ///< `color-transfer`
    std::int32_t color_range = -1;     ///< `color-range`
};

enum class ColorVerdict : std::int32_t {
    /// 观测到 10 bit 输出（P010）。
    kTenBit = 0,
    /// 观测到 8 bit 输出（I420 / NV12）。
    kEightBit = 1,
    /// **观测不到位深**（Surface / Flexible / 未知格式）。不是"没问题"，是"这条路径看不见"。
    kIndeterminate = 2,
};

[[nodiscard]] const char* color_verdict_name(ColorVerdict verdict) noexcept;

struct ColorAssessment {
    /// 源是不是 10 bit。只有 mime+profile 都在已知表里时才可信，见 `source_bit_depth_known`。
    bool source_is_10bit = false;
    /// 源位深是否可判定。mime 不在 HEVC/AVC 表里、或 profile 缺失时为 false ——
    /// 这时 `source_is_10bit` 没有意义，报告里应当写 `null` 而不是 `false`。
    bool source_bit_depth_known = false;
    ColorVerdict verdict = ColorVerdict::kIndeterminate;
    /// **需要解释时必填，无需解释时为空。**
    /// 空串只有一种情况：观测到的位深与源匹配（10bit 源→10bit 输出，或 8bit 源→8bit 输出）。
    std::string reason;
};

/// 判定"这一轮到底拿到了什么位深，以及要不要解释"。
///
/// 卡片 P12-E1 的那句"绝不静默受 NV12"就落在这个函数的三条分支上：
/// - 10bit 源 + 8bit 输出 → `kEightBit` + 非空 reason（降级必须被看见）
/// - 10bit 源 + 看不到位深 → `kIndeterminate` + 非空 reason（看不见也必须被看见）
/// - 8bit 源 + 看不到位深 → `kIndeterminate` + **非空** reason（说明为什么不影响判定）
[[nodiscard]] ColorAssessment assess_color(const VideoColorFacts& facts);

// ---------------------------------------------------------------------------
// P12-E2：HDR 分类
// ---------------------------------------------------------------------------

/// HDR 分类结果。`kind` 与 `reason` **成对出现**：
/// `reason` 不是"出问题时才写"的东西，而是"依据是什么"的记录 ——
/// 判成 SDR 也必须说清依据（哪两个键、各是什么值），否则读者分不清
/// "查了、不是 HDR" 与 "根本没查"。
struct HdrAssessment {
    HdrKind kind = HdrKind::kSdr;
    std::string reason;
};

/// 按轨道格式里的色彩键判定 HDR 类型。
///
/// 判据（数值来源见 `decode/color_key_names.hpp`）：
/// - `transfer == 6 (ST2084/PQ)` 且 `standard == 6 (BT2020)` → `kHdr10`
/// - `transfer == 7 (HLG)` 且 `standard == 6` → `kHlg`
/// - 其余 → `kSdr` + 依据
///
/// **为什么不用 profile**：`profile == 4096`（HEVCProfileMain10HDR10）是编码侧声明，
/// 而这里要判的是"显示侧该按什么处理"。两者不一致时以色彩键为准，
/// profile 只在证据里并列记录供人对照（见 `37-p12e2-design.md` §2.1）。
[[nodiscard]] HdrAssessment classify_hdr(const VideoColorFacts& facts);

}  // namespace kivo::video::mediacodec
