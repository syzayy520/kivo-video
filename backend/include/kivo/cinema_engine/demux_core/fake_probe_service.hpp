#pragma once

#include "kivo/cinema_engine/demux_core/probe_result.hpp"

namespace kivo::cinema_engine {

ProbeResult build_successful_probe_result();
ProbeResult build_failed_probe_result(const std::string& error_message);
ProbeResult build_probe_result_with_streams(
    const std::string& container_format,
    const std::vector<std::string>& video_streams,
    const std::vector<std::string>& audio_streams,
    const std::vector<std::string>& subtitle_streams
);

}  // namespace kivo::cinema_engine