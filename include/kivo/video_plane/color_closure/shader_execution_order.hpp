// shader_execution_order.hpp -- P5H H4: shader execution order
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video_plane::color_closure {

enum class ShaderStage {
    Decode,
    ChromaUpsample,
    ColorMatrix,
    ToneMap,
    TransferFunction,
    DisplayMapping,
    Composite
};

struct ShaderExecutionStep {
    ShaderStage stage{ShaderStage::Decode};
    uint32_t order{0};
    std::string name;
};

class FakeShaderExecutionPlanner {
public:
    // Standard SDR pipeline order
    std::vector<ShaderExecutionStep> plan_sdr_pipeline() {
        return {
            {ShaderStage::Decode, 0, "decode"},
            {ShaderStage::ChromaUpsample, 1, "chroma_upsample"},
            {ShaderStage::ColorMatrix, 2, "color_matrix"},
            {ShaderStage::TransferFunction, 3, "transfer_function"},
            {ShaderStage::Composite, 4, "composite"},
        };
    }

    // HDR pipeline with tone mapping
    std::vector<ShaderExecutionStep> plan_hdr_pipeline() {
        return {
            {ShaderStage::Decode, 0, "decode"},
            {ShaderStage::ChromaUpsample, 1, "chroma_upsample"},
            {ShaderStage::ColorMatrix, 2, "color_matrix"},
            {ShaderStage::ToneMap, 3, "tone_map"},
            {ShaderStage::TransferFunction, 4, "transfer_function"},
            {ShaderStage::DisplayMapping, 5, "display_mapping"},
            {ShaderStage::Composite, 6, "composite"},
        };
    }

    // Verify order is monotonically increasing
    bool verify_order(std::vector<ShaderExecutionStep> const& steps) const {
        for (size_t i = 1; i < steps.size(); ++i) {
            if (steps[i].order <= steps[i-1].order) return false;
        }
        return true;
    }

    // Verify tone map comes before transfer function in HDR
    bool verify_hdr_order(std::vector<ShaderExecutionStep> const& steps) const {
        int tone_map_idx = -1;
        int transfer_idx = -1;
        for (size_t i = 0; i < steps.size(); ++i) {
            if (steps[i].stage == ShaderStage::ToneMap) tone_map_idx = static_cast<int>(i);
            if (steps[i].stage == ShaderStage::TransferFunction) transfer_idx = static_cast<int>(i);
        }
        if (tone_map_idx < 0 || transfer_idx < 0) return false;
        return tone_map_idx < transfer_idx;
    }
};

}  // namespace kivo::video_plane::color_closure
