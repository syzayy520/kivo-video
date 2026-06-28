// audio_crash_dump_policy.hpp — P6A security: crash dump policy (§security)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::security {

// AudioCrashDumpPolicy
// Crash dump redaction policy. Compile-only skeleton.
struct AudioCrashDumpPolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool no_raw_bytes{true};
    bool no_codec_private_data{true};
    bool redacted_json{true};

    bool operator==(const AudioCrashDumpPolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::security
