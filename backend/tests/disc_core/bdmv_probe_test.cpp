#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/disc_core/fake_bdmv_probe_service.hpp"

using namespace kivo::cinema_engine;

static void test_successful_bdmv_probe() {
    auto result = build_successful_bdmv_probe("probe-001", "/media/disc/BDMV");
    assert(result.metadata.schema_version == "1.1");
    assert(result.probe_id == "probe-001");
    assert(result.success == true);
    assert(result.is_bdmv_folder == true);
    assert(result.is_mounted_iso == false);
    assert(result.playlist_files.size() == 3);
    assert(result.stream_files.size() == 2);
    assert(result.total_size_bytes > 0);
    assert(result.error_message.empty());
    std::cout << "  PASS: successful_bdmv_probe\n";
}

static void test_failed_bdmv_probe() {
    auto result = build_failed_bdmv_probe("probe-002", "Not a BDMV folder");
    assert(result.probe_id == "probe-002");
    assert(result.success == false);
    assert(result.is_bdmv_folder == false);
    assert(result.is_mounted_iso == false);
    assert(result.error_message == "Not a BDMV folder");
    assert(result.playlist_files.empty());
    assert(result.stream_files.empty());
    std::cout << "  PASS: failed_bdmv_probe\n";
}

static void test_mounted_iso_probe() {
    auto result = build_mounted_iso_probe("probe-003", "/mnt/iso/BDMV");
    assert(result.probe_id == "probe-003");
    assert(result.success == true);
    assert(result.is_bdmv_folder == true);
    assert(result.is_mounted_iso == true);
    assert(result.playlist_files.size() == 2);
    assert(result.stream_files.size() == 3);
    assert(result.total_size_bytes > 0);
    std::cout << "  PASS: mounted_iso_probe\n";
}

static void test_empty_bdmv_probe() {
    auto result = build_empty_bdmv_probe("probe-004", "/media/disc/BDMV");
    assert(result.probe_id == "probe-004");
    assert(result.success == true);
    assert(result.is_bdmv_folder == true);
    assert(result.is_mounted_iso == false);
    assert(result.playlist_files.empty());
    assert(result.stream_files.empty());
    assert(result.total_size_bytes == 0);
    std::cout << "  PASS: empty_bdmv_probe\n";
}

static void test_bdmv_with_playlists() {
    std::vector<std::string> playlists = {"00001.mpls", "00002.mpls", "00003.mpls", "00004.mpls"};
    auto result = build_bdmv_probe_with_playlists("probe-005", "/media/disc/BDMV", playlists);
    assert(result.probe_id == "probe-005");
    assert(result.success == true);
    assert(result.is_bdmv_folder == true);
    assert(result.is_mounted_iso == false);
    assert(result.playlist_files.size() == 4);
    assert(result.playlist_files[0] == "00001.mpls");
    assert(result.playlist_files[3] == "00004.mpls");
    assert(result.stream_files.size() == 1);
    assert(result.total_size_bytes > 0);
    std::cout << "  PASS: bdmv_with_playlists\n";
}

int main() {
    std::cout << "bdmv_probe_test:\n";
    test_successful_bdmv_probe();
    test_failed_bdmv_probe();
    test_mounted_iso_probe();
    test_empty_bdmv_probe();
    test_bdmv_with_playlists();
    std::cout << "ALL 5 TESTS PASSED\n";
    return 0;
}
