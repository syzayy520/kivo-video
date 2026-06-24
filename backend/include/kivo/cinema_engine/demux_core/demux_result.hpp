#pragma once

#include <string>

#include "kivo/cinema_engine/demux_core/kivo_packet.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct DemuxResult {
    ContractMetadata metadata;
    KivoPacket packet;
    bool success{false};
    bool eof{false};
    std::string error_message;
};

}  // namespace kivo::cinema_engine