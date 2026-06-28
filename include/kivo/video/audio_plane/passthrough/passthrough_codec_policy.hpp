// passthrough_codec_policy.hpp — P6A passthrough: PassthroughCodecPolicy (§8.5)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::passthrough {
struct PassthroughCodecPolicy {
    ContractMetadata metadata;
    int32_t codec_id{0};
    uint64_t standard_evidence_file_ref{0};
    uint64_t golden_fixture_id{0};
    int32_t burst_spacing_rule{0};
    int32_t padding_rule{0};
    int32_t pause_burst_rule{0};
    int32_t transport_limit_rule{0};
    int32_t fallback_condition{0};
    bool unverified_is_not_claimable{true};
    bool operator==(const PassthroughCodecPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::passthrough
