#include "kivo/cinema_engine/disc_core/fake_bdmv_playback_gate_service.hpp"
#include <cassert>
#include <iostream>

using namespace kivo::cinema_engine;

void test_valid_bdmv_gate() {
    auto result = FakeBdmvPlaybackGateService::build_valid_bdmv_gate();
    assert(result.request_id == "valid-bdmv-001");
    assert(result.success == true);
    assert(result.playback_allowed == true);
    assert(result.gate_reason == "Valid BDMV folder with main title playlist");
    assert(result.failure_reason.empty());
    std::cout << "PASS: test_valid_bdmv_gate" << std::endl;
}

void test_mounted_iso_gate() {
    auto result = FakeBdmvPlaybackGateService::build_mounted_iso_gate();
    assert(result.request_id == "mounted-iso-001");
    assert(result.success == true);
    assert(result.playback_allowed == true);
    assert(result.gate_reason == "Mounted ISO with valid BDMV structure");
    assert(result.failure_reason.empty());
    std::cout << "PASS: test_mounted_iso_gate" << std::endl;
}

void test_encrypted_disc_gate() {
    auto result = FakeBdmvPlaybackGateService::build_encrypted_disc_gate();
    assert(result.request_id == "encrypted-disc-001");
    assert(result.success == true);
    assert(result.playback_allowed == false);
    assert(result.gate_reason == "Encrypted disc detected");
    assert(result.failure_reason == "Encrypted discs are not supported (fail-closed)");
    std::cout << "PASS: test_encrypted_disc_gate" << std::endl;
}

void test_missing_streams_gate() {
    auto result = FakeBdmvPlaybackGateService::build_missing_streams_gate();
    assert(result.request_id == "missing-streams-001");
    assert(result.success == true);
    assert(result.playback_allowed == false);
    assert(result.gate_reason == "Missing stream files detected");
    assert(result.failure_reason == "Required stream files are missing");
    std::cout << "PASS: test_missing_streams_gate" << std::endl;
}

void test_invalid_playlist_gate() {
    auto result = FakeBdmvPlaybackGateService::build_invalid_playlist_gate();
    assert(result.request_id == "invalid-playlist-001");
    assert(result.success == true);
    assert(result.playback_allowed == false);
    assert(result.gate_reason == "Invalid playlist detected");
    assert(result.failure_reason == "Playlist file is corrupted or invalid");
    std::cout << "PASS: test_invalid_playlist_gate" << std::endl;
}

int main() {
    test_valid_bdmv_gate();
    test_mounted_iso_gate();
    test_encrypted_disc_gate();
    test_missing_streams_gate();
    test_invalid_playlist_gate();
    
    std::cout << "All BDMV playback gate tests passed!" << std::endl;
    return 0;
}
