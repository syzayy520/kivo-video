#pragma once

#include "kivo/cinema_engine/demux_core/demux_result.hpp"

namespace kivo::cinema_engine {

DemuxResult build_successful_demux_result();
DemuxResult build_eof_demux_result();
DemuxResult build_failed_demux_result(const std::string& error_message);
DemuxResult build_demux_result_with_packet(const KivoPacket& packet);

}  // namespace kivo::cinema_engine