#include "kivo/cinema_engine/disc_core/fake_bdmv_playback_gate_service.hpp"

namespace kivo::cinema_engine {

BdmvPlaybackGateResult FakeBdmvPlaybackGateService::build_valid_bdmv_gate() {
    BdmvPlaybackGateResult result;
    result.request_id = "valid-bdmv-001";
    result.success = true;
    result.playback_allowed = true;
    result.gate_reason = "Valid BDMV folder with main title playlist";
    result.failure_reason = "";
    return result;
}

BdmvPlaybackGateResult FakeBdmvPlaybackGateService::build_mounted_iso_gate() {
    BdmvPlaybackGateResult result;
    result.request_id = "mounted-iso-001";
    result.success = true;
    result.playback_allowed = true;
    result.gate_reason = "Mounted ISO with valid BDMV structure";
    result.failure_reason = "";
    return result;
}

BdmvPlaybackGateResult FakeBdmvPlaybackGateService::build_encrypted_disc_gate() {
    BdmvPlaybackGateResult result;
    result.request_id = "encrypted-disc-001";
    result.success = true;
    result.playback_allowed = false;
    result.gate_reason = "Encrypted disc detected";
    result.failure_reason = "Encrypted discs are not supported (fail-closed)";
    return result;
}

BdmvPlaybackGateResult FakeBdmvPlaybackGateService::build_missing_streams_gate() {
    BdmvPlaybackGateResult result;
    result.request_id = "missing-streams-001";
    result.success = true;
    result.playback_allowed = false;
    result.gate_reason = "Missing stream files detected";
    result.failure_reason = "Required stream files are missing";
    return result;
}

BdmvPlaybackGateResult FakeBdmvPlaybackGateService::build_invalid_playlist_gate() {
    BdmvPlaybackGateResult result;
    result.request_id = "invalid-playlist-001";
    result.success = true;
    result.playback_allowed = false;
    result.gate_reason = "Invalid playlist detected";
    result.failure_reason = "Playlist file is corrupted or invalid";
    return result;
}

} // namespace kivo::cinema_engine
