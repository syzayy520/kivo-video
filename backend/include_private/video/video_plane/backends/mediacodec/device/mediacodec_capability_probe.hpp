#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video::mediacodec {

/// 探测结果。code == 0 表示证据已成功落盘。
struct CapabilityProbeOutcome {
    std::int32_t code = 0;
    std::string message;
    std::size_t total_records = 0;
    std::size_t decoder_records = 0;
    std::size_t hardware_decoder_records = 0;
    std::size_t decoder_10bit_records = 0;
};

/// 把 Java 侧摊平的记录整理成结构化能力证据并写入 output_path。
///
/// 只做编排：聚合 → 渲染 → 落盘。解析、命名、位深、JSON 细节都在各自的单元里。
/// 不做能力协商（协商在 decode 阶段做）。
///
/// 失败时 outcome.code 非 0 且 message 非空 —— 符合 G9「不许静默降级」。
[[nodiscard]] CapabilityProbeOutcome probe_capabilities(
    const std::vector<std::string>& raw_records,
    const std::string& output_path);

}  // namespace kivo::video::mediacodec
