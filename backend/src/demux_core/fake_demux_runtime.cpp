#include "kivo/cinema_engine/demux_core/fake_demux_runtime.hpp"

namespace kivo::cinema_engine {

DemuxResult build_successful_demux_result() {
    DemuxResult result;
    result.metadata.schema_version = "1.1";
    result.packet.metadata.schema_version = "1.1";
    result.packet.packet_id = "packet-001";
    result.packet.stream_index = 0;
    result.packet.pts = 1000;
    result.packet.dts = 1000;
    result.packet.duration = 40;
    result.packet.is_key_frame = true;
    result.packet.is_corrupt = false;
    result.packet.data_size = 1024;
    result.packet.data_ref = "adapter://packet-001";
    result.success = true;
    result.eof = false;
    return result;
}

DemuxResult build_eof_demux_result() {
    DemuxResult result;
    result.metadata.schema_version = "1.1";
    result.success = false;
    result.eof = true;
    result.error_message = "End of file";
    return result;
}

DemuxResult build_failed_demux_result(const std::string& error_message) {
    DemuxResult result;
    result.metadata.schema_version = "1.1";
    result.success = false;
    result.eof = false;
    result.error_message = error_message;
    return result;
}

DemuxResult build_demux_result_with_packet(const KivoPacket& packet) {
    DemuxResult result;
    result.metadata.schema_version = "1.1";
    result.packet = packet;
    result.success = true;
    result.eof = false;
    return result;
}

}  // namespace kivo::cinema_engine