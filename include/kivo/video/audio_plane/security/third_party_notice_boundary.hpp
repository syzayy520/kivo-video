// third_party_notice_boundary.hpp — P6A security: third party notice (§security)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::security {

// ThirdPartyNoticeBoundary
// Boundary for third party license notices. Compile-only skeleton.
struct ThirdPartyNoticeBoundary {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool license_manifest_required{true};
    bool codec_feature_to_license_mapping{true};

    bool operator==(const ThirdPartyNoticeBoundary&) const = default;
};

}  // namespace kivo::video::audio_plane::security
