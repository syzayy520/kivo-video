// audio_buffer_redaction_policy.hpp — P6A security: buffer redaction (§security)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::security {

// AudioBufferRedactionPolicy
// Redaction policy for audio buffers in diagnostics. Compile-only skeleton.
struct AudioBufferRedactionPolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool raw_bytes_redacted{true};
    bool codec_private_data_redacted{true};

    bool operator==(const AudioBufferRedactionPolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::security
