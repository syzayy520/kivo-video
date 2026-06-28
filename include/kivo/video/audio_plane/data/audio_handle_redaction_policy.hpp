// audio_handle_redaction_policy.hpp — P6A §8/§33: handle redaction
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::data {
struct AudioHandleRedactionPolicy {
    ContractMetadata metadata;
    bool evidence_contains_handle_id{true};
    bool evidence_contains_size{true};
    bool evidence_contains_salted_hash_if_policy_allows{true};
    bool evidence_must_not_contain_raw_bytes{true};
    bool crash_dump_must_not_contain_raw_bytes{true};
    bool crash_dump_must_not_contain_codec_private_data{true};
};
}  // namespace kivo::video::audio_plane::data
