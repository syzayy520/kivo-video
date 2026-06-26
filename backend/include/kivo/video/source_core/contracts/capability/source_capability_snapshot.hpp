#pragma once
#include "kivo/video/source_core/contracts/capability/range_proof_kind.hpp"
#include <cstdint>
#include <optional>
#include <string>
namespace kivo::video::source_core {
enum class CapabilityState : std::uint8_t { unknown=0, supported=1, unsupported=2 };
struct SourceCapabilitySnapshot {
    CapabilityState seek{CapabilityState::unknown};
    CapabilityState range_read{CapabilityState::unknown};
    CapabilityState direct_play_candidate{CapabilityState::unknown};
    CapabilityState direct_stream_candidate{CapabilityState::unknown};
    CapabilityState can_range_request{CapabilityState::unknown};
    std::optional<std::uint64_t> content_length_bytes;
    std::optional<std::string> mime_type;
    std::optional<std::string> container_hint;
    RangeProofKind range_proof_kind{RangeProofKind::none};
    ValidatorStrengthLevel validator_strength{ValidatorStrengthLevel::none};
    SourceAccessEligibility eligibility{SourceAccessEligibility::ineligible_no_range};
    std::uint32_t capability_generation{0};
    std::uint32_t contract_schema_version{1};
};
}  // namespace kivo::video::source_core
