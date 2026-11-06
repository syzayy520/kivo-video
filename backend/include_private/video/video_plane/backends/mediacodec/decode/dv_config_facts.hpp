#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace kivo::video::mediacodec {

// ---- 210 · DV-A：识别 + 透传 -------------------------------------------------
//
// 判定层（纯逻辑，桌面可单测）。平台读数（`AMediaFormat`）在
// `decode/mediaformat_dv_io.hpp`，与颜色家族的 `mediaformat_color_io` 同款分工：
// 读键的不解释含义，解释含义的不碰平台类型。
//
// 实测依据（K40 / Android 13，2026-11-06，`docs/evidence/p210/p210-track-dump-*.json`）：
// - 平台对带 `dvcC` 的 HEVC 轨直接报 `mime = video/dolby-vision`（路 A，零容器解析）；
// - "profile" 键是位标志编码：profile 8 → 256 = 1<<8，profile 5 → 32 = 1<<5；
// - "level" 键同为位标志：level 1 → 1 = 1<<(1-1)，level 6 → 32 = 1<<(6-1)；
// - "bl_compatibility_id" 键在 profile 8 上 = 1，在 profile 5 上整键缺席（bl_present=0）；
// - K40 没有 dolby-vision 解码器：探针三次 `createDecoderByType` 全部 null
//   —— 所以 8.x 透传必须重写 mime 交给 HEVC 解码器（BL 本来就是标准 HEVC Main10）。

/// 平台轨道格式里的 DV 键（`mediaformat_dv_io` 填；-1 = 键缺席）。
struct DvTrackKeys {
    std::int32_t profile_key = -1;
    std::int32_t level_key = -1;
    std::int32_t bl_compatibility_id = -1;
};

enum class DvVerdict : std::int32_t {
    /// mime 不是 dolby-vision：不是 DV 片源，一切照旧。
    kNotDolbyVision = 0,
    /// profile 8.x：BL 是 HDR10 兼容流，按 HDR10 透传（重写 mime，交给 HEVC 解码器）。
    kPassthroughHdr10 = 1,
    /// DV 片源但 A 档不收（profile 5 / 未知 profile / BL 编码不明）。必须拒绝且 reason 非空。
    kUnsupported = 2,
};

[[nodiscard]] const char* dv_verdict_name(DvVerdict verdict) noexcept;

/// mime 是否声明 DV。
[[nodiscard]] bool is_dolby_vision_mime(std::string_view mime);

/// 平台 profile 键 → DV profile 号（`1<<p` → `p`）。不是 2 的幂 → -1（**不猜**）。
[[nodiscard]] std::int32_t dv_profile_from_key(std::int32_t profile_key);

/// 平台 level 键 → DV level 号（`1<<(l-1)` → `l`）。不是 2 的幂 → -1（**不猜**）。
[[nodiscard]] std::int32_t dv_level_from_key(std::int32_t level_key);

/// BL（基础层）编码嗅探：mime 只说 dolby-vision，不说底下是 HEVC 还是 AVC；
/// profile 8.x 的 BL 是标准流，从 csd-0 第一个 NAL 的头判家族。
enum class BlCodec : std::int32_t {
    kUnknown = 0,
    kHevc = 1,  ///< 第一个 NAL 是 VPS（type 32）或 SPS（type 33）
    kAvc = 2,   ///< 第一个 NAL 是 AVC SPS（type 7）
};

[[nodiscard]] BlCodec sniff_bl_codec(const std::uint8_t* csd0, std::size_t size);

/// BL 编码对应的 mime；Unknown 给空串（调用方据此拒绝，不猜）。
[[nodiscard]] const char* bl_codec_mime(BlCodec codec) noexcept;

/// 一次 DV 判定的结果。`reason` **永远非空**（判成"不是 DV"也要说依据 —— E2 的成对铁律）。
struct DvAssessment {
    DvVerdict verdict = DvVerdict::kNotDolbyVision;
    std::int32_t dv_profile = -1;  ///< 解出的 profile 号；-1 = 没解出来
    std::int32_t dv_level = -1;    ///< 同上
    std::string bl_mime;           ///< 透传用的 BL mime；非透传为空
    std::string reason;
};

[[nodiscard]] DvAssessment assess_dv(const std::string& mime, const DvTrackKeys& keys,
                                     BlCodec bl_codec);

/// 证据出版用：判定结果 + 透传动作（后两格由会话在接线处补齐）。
struct DvFacts {
    DvVerdict verdict = DvVerdict::kNotDolbyVision;
    std::int32_t dv_profile = -1;
    std::int32_t dv_level = -1;
    std::int32_t bl_compatibility_id = -1;  ///< 平台键原值；-1 = 键缺席（profile 5 上就是缺席）
    std::string bl_mime;                    ///< 实际给解码器的 mime；空 = 没重写
    bool configure_format_rebuilt = false;  ///< true = 用了重建的 configure 格式
    std::string reason;                     ///< 永远非空
};

}  // namespace kivo::video::mediacodec
