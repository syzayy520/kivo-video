#pragma once

#include "kivo/cinema_engine/demux_core/kivo_packet.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct DecodeRequest {
    ContractMetadata metadata;
    KivoPacket packet;
    bool force_software{true};
};

}  // namespace kivo::cinema_engine