#pragma once

#include <string>

#include "kivo/cinema_engine/decode_core/decoded_frame.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct DecodeResult {
    ContractMetadata metadata;
    DecodedFrame frame;
    bool success{false};
    bool eof{false};
    bool needs_more_input{false};
    std::string error_message;
};

}  // namespace kivo::cinema_engine