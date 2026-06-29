#include "video/subtitle_plane/discovery/discovery_priority.hpp"

namespace kivo::video::subtitle_plane::runtime {

uint64_t discovery_priority_key(const uint32_t flags,
                                const uint32_t candidate_flags,
                                const uint32_t priority_score,
                                const uint32_t source_order,
                                const KivoRedactedSourceIdentity identity) noexcept {
    const bool user_prefers_external = (flags & (1u << 4u)) != 0u;
    const bool candidate_external = (candidate_flags & 1u) != 0u;
    const uint64_t external_rank = user_prefers_external
        ? (candidate_external ? 0ull : 1ull)
        : (candidate_external ? 1ull : 0ull);
    return (external_rank << 56u) |
           ((0xffffffffull - static_cast<uint64_t>(priority_score)) << 24u) |
           (static_cast<uint64_t>(source_order) << 8u) |
           (identity.source_identity_hash & 0xffu);
}

}  // namespace kivo::video::subtitle_plane::runtime
