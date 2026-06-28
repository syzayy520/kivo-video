// audio_codec_private_data_blob.hpp — P6A format: codec private data blob contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
struct AudioCodecPrivateDataBlobContract {
    kivo::video::audio_plane::ContractMetadata metadata;
    bool never_exposed_to_p8{true};
    bool never_raw_in_evidence{true};
    bool never_raw_in_crash_dump{true};
    bool never_logged{true};
    bool never_serialized_as_plaintext{true};
};
}  // namespace kivo::video::audio_plane::format
