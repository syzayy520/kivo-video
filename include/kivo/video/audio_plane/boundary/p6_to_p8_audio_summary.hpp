// p6_to_p8_audio_summary.hpp — P6A boundary: P6→P8 audio summary (§7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::boundary {

// P6ToP8AudioSummary
// Summary of the P6 audio media plane output state reported to the P8
// observability / claim layer. output_mode and claim_grade are enum-as-int
// placeholders. claim_generation ties the summary to a specific claim epoch.
struct P6ToP8AudioSummary {
    kivo::video::audio_plane::ContractMetadata metadata{};
    int32_t output_mode{0};
    int32_t claim_grade{0};
    bool bit_perfect_active{false};
    bool passthrough_active{false};
    uint64_t claim_generation{0};

    bool operator==(const P6ToP8AudioSummary&) const = default;
};

}  // namespace kivo::video::audio_plane::boundary
