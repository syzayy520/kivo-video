#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace kivo::p3::drm {

/// DrmSystem enumerates DRM systems (metadata only, no license/decryption).
enum class DrmSystem
{
    fairplay,
    widevine,
    playready,
    clearkey,
    hls_aes_128,
    none
};

/// DrmMetadata identifies DRM metadata without decrypting.
/// schema_version: p3.drm.metadata.v1
struct DrmMetadata
{
    std::string schema_version{"p3.drm.metadata.v1"};
    DrmSystem system{DrmSystem::none};
    std::string redacted_kid{};
    std::string redacted_license_uri{};
    std::string redacted_key_uri{};
    bool has_clear_fallback{false};
    bool partial_drm_blocked{false};

    bool is_playable() const noexcept { return system == DrmSystem::none || has_clear_fallback; }
};

/// DrmTrackDecision defines DRM track-level decision.
struct DrmTrackDecision
{
    std::string schema_version{"p3.drm.track_decision.v1"};
    bool is_drm_track{false};
    bool clear_fallback_available{false};
    bool track_blocked{false};
    std::string reason{};

    bool is_track_playable() const noexcept { return !is_drm_track || clear_fallback_available; }
};

} // namespace kivo::p3::drm
