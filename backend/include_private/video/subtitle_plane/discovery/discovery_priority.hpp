#pragma once

#include <cstdint>

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

uint64_t discovery_priority_key(uint32_t flags,
                                uint32_t candidate_flags,
                                uint32_t priority_score,
                                uint32_t source_order,
                                KivoRedactedSourceIdentity identity) noexcept;

}  // namespace kivo::video::subtitle_plane::runtime
