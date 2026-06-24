#include "kivo/cinema_engine/demux_core/fake_probe_service.hpp"

namespace kivo::cinema_engine {

ProbeResult build_successful_probe_result() {
    ProbeResult result;
    result.metadata.schema_version = "1.1";
    result.inventory.metadata.schema_version = "1.1";
    result.inventory.inventory_id = "fake-inventory-001";
    result.inventory.container_format = "mp4";
    result.inventory.duration = "02:30:00";
    result.inventory.bitrate_estimate = "8000 kbps";
    result.inventory.video_streams = {"h264:1920x1080@24fps"};
    result.inventory.audio_streams = {"aac:2ch:48000Hz"};
    result.inventory.subtitle_streams = {"srt:eng"};
    result.success = true;
    return result;
}

ProbeResult build_failed_probe_result(const std::string& error_message) {
    ProbeResult result;
    result.metadata.schema_version = "1.1";
    result.success = false;
    result.error_message = error_message;
    return result;
}

ProbeResult build_probe_result_with_streams(
    const std::string& container_format,
    const std::vector<std::string>& video_streams,
    const std::vector<std::string>& audio_streams,
    const std::vector<std::string>& subtitle_streams
) {
    ProbeResult result;
    result.metadata.schema_version = "1.1";
    result.inventory.metadata.schema_version = "1.1";
    result.inventory.inventory_id = "fake-inventory-custom";
    result.inventory.container_format = container_format;
    result.inventory.video_streams = video_streams;
    result.inventory.audio_streams = audio_streams;
    result.inventory.subtitle_streams = subtitle_streams;
    result.success = true;
    return result;
}

}  // namespace kivo::cinema_engine