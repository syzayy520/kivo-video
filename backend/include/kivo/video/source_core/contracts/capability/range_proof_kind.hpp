#pragma once
#include <cstdint>
namespace kivo::video::source_core {
enum class RangeProofKind : std::uint8_t { none=0, seekable_known_length=1, seekable_empty_source=2,
    range_observed_unknown_length_not_seekable=3, rejected_no_range=4, failed_protocol_error=5,
    failed_nonconformant_response=6 };
enum class SourceAccessEligibility : std::uint8_t { eligible_seekable=0, eligible_empty_source=1,
    ineligible_no_range=2, ineligible_unknown_length=3, ineligible_nonconformant=4,
    ineligible_source_changed=5, ineligible_session_closed=6, ineligible_stale_reference=7,
    ineligible_auth_failed=8 };
enum class ValidatorStrengthLevel : std::uint8_t { strong_etag=0, last_modified_second_precision=1,
    weak_etag_hint=2, length_only_hint=3, none=4 };
enum class RemoteFallbackCandidateKind : std::uint8_t { no_range=0, unknown_length=1, tiny_probe=2,
    extended_probe=3, vendor_quirk=4, none=5 };
}  // namespace kivo::video::source_core
