// passthrough_claim_grade.hpp — P6A passthrough: PassthroughClaimGrade (§8.9)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::passthrough {
enum class PassthroughClaimGrade {
    NotClaimable = 0,
    Eligible = 1,
    Active = 2,
    BrokenAfterStart = 3,
    FailClosed = 4,
};
}  // namespace kivo::video::audio_plane::passthrough
