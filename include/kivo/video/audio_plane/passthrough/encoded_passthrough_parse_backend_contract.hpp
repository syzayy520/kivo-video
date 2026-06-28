// encoded_passthrough_parse_backend_contract.hpp — P6A passthrough: EncodedPassthroughParseBackendContract (§8.1)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::passthrough {
struct EncodedPassthroughParseBackendContract {
    ContractMetadata metadata;
    bool separates_from_decode_backend{true};
    bool must_not_depend_on_decoded_pcm{true};
    bool operator==(const EncodedPassthroughParseBackendContract&) const = default;
};
}  // namespace kivo::video::audio_plane::passthrough
