// audio_lock_rank.hpp — P6A threading: lock rank (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioLockRank
// Rank of a lock to enforce acquisition order. Compile-only enum.
enum class AudioLockRank {
    None = 0,
    EvidenceRing = 1,
    ControlQueue = 2,
    RecoveryQueue = 3
};

}  // namespace kivo::video::audio_plane::threading
