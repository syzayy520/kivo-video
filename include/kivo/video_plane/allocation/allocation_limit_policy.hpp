// allocation_limit_policy.hpp — GATE-005: P5A allocation limit policy (12 constexpr values)
#pragma once
#include <cstdint>

namespace kivo::video_plane::allocation {

namespace AllocationDefaults {
    constexpr uint64_t kMaxSingleMediaAllocation = 64ULL * 1024ULL * 1024ULL;       // 64 MiB
    constexpr uint64_t kMaxSingleAttachmentDescriptorBytes = 16ULL * 1024ULL * 1024ULL;
    constexpr uint64_t kMaxSingleAttachmentBytes = 64ULL * 1024ULL * 1024ULL;
    constexpr uint64_t kMaxTotalAttachmentBytes = 256ULL * 1024ULL * 1024ULL;
    constexpr uint32_t kMaxTrackCount = 256;
    constexpr uint32_t kMaxChapterCount = 10000;
    constexpr uint64_t kMaxMetadataEntryBytes = 1ULL * 1024ULL * 1024ULL;
    constexpr uint64_t kMaxTotalMetadataBytes = 32ULL * 1024ULL * 1024ULL;
    constexpr uint64_t kMaxProbeBytes = 10ULL * 1024ULL * 1024ULL;                  // policy-capped
    constexpr uint64_t kMaxSubtitlePacketBytes = 8ULL * 1024ULL * 1024ULL;
    constexpr uint64_t kMaxSubtitleQueueBytes = 64ULL * 1024ULL * 1024ULL;
    constexpr uint64_t kMaxIpcPayloadBytes = 16ULL * 1024ULL * 1024ULL;
}  // namespace AllocationDefaults

}  // namespace kivo::video_plane::allocation
