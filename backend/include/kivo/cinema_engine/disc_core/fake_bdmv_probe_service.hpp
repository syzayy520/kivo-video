#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/disc_core/bdmv_probe_result.hpp"

namespace kivo::cinema_engine {

// Build a successful BDMV folder probe result.
BdmvProbeResult build_successful_bdmv_probe(const std::string& probe_id, const std::string& folder_path);

// Build a failed BDMV probe result.
BdmvProbeResult build_failed_bdmv_probe(const std::string& probe_id, const std::string& error_message);

// Build a mounted ISO probe result.
BdmvProbeResult build_mounted_iso_probe(const std::string& probe_id, const std::string& folder_path);

// Build an empty BDMV folder probe result (no playlists).
BdmvProbeResult build_empty_bdmv_probe(const std::string& probe_id, const std::string& folder_path);

// Build a BDMV with specific playlists.
BdmvProbeResult build_bdmv_probe_with_playlists(
    const std::string& probe_id,
    const std::string& folder_path,
    const std::vector<std::string>& playlists);

}  // namespace kivo::cinema_engine
