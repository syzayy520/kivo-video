// link_rules.hpp — GATE-026: P5A link rules (10 rules from §2)
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video_plane::architecture {

struct LinkRule {
    uint32_t rule_id{0};
    std::string rule_description;
    std::string check_method;               // how to verify this rule
};

struct LinkRules {
    // 10 link rules from §2:
    // 1. contracts links NOTHING
    // 2. contracts has NO link to FFmpeg/D3D11/DXGI/Qt/WASAPI
    // 3. core links contracts only, NO FFmpeg/D3D11
    // 4. ffmpeg_demux MAY link FFmpeg, ONLY in ffmpeg_demux/ dir
    // 5. d3d11_gpu MAY link D3D11, ONLY in d3d11_gpu/ dir
    // 6. Public headers contain NO raw third-party types
    // 7. Binary exports contain NO forbidden symbols
    // 8. Future backends NOT in default include path
    // 9. Future backends NOT linked into baseline binary
    // 10. Core target exposes NO FFmpeg/D3D11 public ABI
    std::vector<LinkRule> rules;
    bool operator==(LinkRules const&) const = default;
};

}  // namespace kivo::video_plane::architecture
