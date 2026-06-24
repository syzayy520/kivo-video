#include "kivo/cinema_engine/disc_core/fake_bdmv_probe_service.hpp"

namespace kivo::cinema_engine {

BdmvProbeResult build_successful_bdmv_probe(const std::string& probe_id, const std::string& folder_path) {
    BdmvProbeResult result;
    result.metadata.schema_version = "1.1";
    result.metadata.trace_id = "fake-bdmv-probe";
    result.probe_id = probe_id;
    result.success = true;
    result.is_bdmv_folder = true;
    result.is_mounted_iso = false;
    result.playlist_files = {"00001.mpls", "00002.mpls", "00003.mpls"};
    result.stream_files = {"00001.m2ts", "00002.m2ts"};
    result.total_size_bytes = 50LL * 1024 * 1024 * 1024;  // 50 GB
    return result;
}

BdmvProbeResult build_failed_bdmv_probe(const std::string& probe_id, const std::string& error_message) {
    BdmvProbeResult result;
    result.metadata.schema_version = "1.1";
    result.metadata.trace_id = "fake-bdmv-probe";
    result.probe_id = probe_id;
    result.success = false;
    result.is_bdmv_folder = false;
    result.is_mounted_iso = false;
    result.error_message = error_message;
    return result;
}

BdmvProbeResult build_mounted_iso_probe(const std::string& probe_id, const std::string& folder_path) {
    BdmvProbeResult result;
    result.metadata.schema_version = "1.1";
    result.metadata.trace_id = "fake-bdmv-probe";
    result.probe_id = probe_id;
    result.success = true;
    result.is_bdmv_folder = true;
    result.is_mounted_iso = true;
    result.playlist_files = {"00001.mpls", "00002.mpls"};
    result.stream_files = {"00001.m2ts", "00002.m2ts", "00003.m2ts"};
    result.total_size_bytes = 45LL * 1024 * 1024 * 1024;  // 45 GB
    return result;
}

BdmvProbeResult build_empty_bdmv_probe(const std::string& probe_id, const std::string& folder_path) {
    BdmvProbeResult result;
    result.metadata.schema_version = "1.1";
    result.metadata.trace_id = "fake-bdmv-probe";
    result.probe_id = probe_id;
    result.success = true;
    result.is_bdmv_folder = true;
    result.is_mounted_iso = false;
    result.playlist_files = {};
    result.stream_files = {};
    result.total_size_bytes = 0;
    return result;
}

BdmvProbeResult build_bdmv_probe_with_playlists(
    const std::string& probe_id,
    const std::string& folder_path,
    const std::vector<std::string>& playlists) {
    BdmvProbeResult result;
    result.metadata.schema_version = "1.1";
    result.metadata.trace_id = "fake-bdmv-probe";
    result.probe_id = probe_id;
    result.success = true;
    result.is_bdmv_folder = true;
    result.is_mounted_iso = false;
    result.playlist_files = playlists;
    result.stream_files = {"00001.m2ts"};
    result.total_size_bytes = 30LL * 1024 * 1024 * 1024;  // 30 GB
    return result;
}

}  // namespace kivo::cinema_engine
