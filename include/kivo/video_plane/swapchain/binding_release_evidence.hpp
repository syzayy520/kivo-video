// binding_release_evidence.hpp — GATE-017: P5A binding release evidence
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::swapchain {

struct BindingReleaseEvidence {
    uint64_t backbuffer_handle{0};          // backbuffer that was released
    uint32_t views_released{0};             // number of views released
    uint32_t refs_released{0};              // number of refs released
    std::string evidence_id;
    uint64_t timestamp{0};
    bool operator==(BindingReleaseEvidence const&) const = default;
};

}  // namespace kivo::video_plane::swapchain
