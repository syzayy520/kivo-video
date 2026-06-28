// codec_private_data_redaction_policy.hpp — P6A security: codec private data (§security)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::security {

// CodecPrivateDataRedactionPolicy
// Redaction policy for codec private data. Compile-only skeleton.
struct CodecPrivateDataRedactionPolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool summary_allowed{true};
    bool raw_forbidden{true};

    bool operator==(const CodecPrivateDataRedactionPolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::security
