#include "kivo/cinema_engine/disc_core/fake_bdmv_playlist_duplicate_service.hpp"
#include <cassert>
#include <iostream>

using namespace kivo::cinema_engine;

void test_no_duplicates_policy() {
    auto result = FakeBdmvPlaylistDuplicateService::build_no_duplicates_policy();
    assert(result.request_id == "no-duplicates-001");
    assert(result.success == true);
    assert(result.unique_playlists.size() == 3);
    assert(result.collapsed_playlists.empty());
    assert(result.collapse_reason == "No duplicates found");
    std::cout << "PASS: test_no_duplicates_policy" << std::endl;
}

void test_identical_playlists_policy() {
    auto result = FakeBdmvPlaylistDuplicateService::build_identical_playlists_policy();
    assert(result.request_id == "identical-playlists-001");
    assert(result.success == true);
    assert(result.unique_playlists.size() == 1);
    assert(result.collapsed_playlists.size() == 2);
    assert(result.collapse_reason == "Identical stream sequences");
    std::cout << "PASS: test_identical_playlists_policy" << std::endl;
}

void test_subset_playlists_policy() {
    auto result = FakeBdmvPlaylistDuplicateService::build_subset_playlists_policy();
    assert(result.request_id == "subset-playlists-001");
    assert(result.success == true);
    assert(result.unique_playlists.size() == 1);
    assert(result.collapsed_playlists.size() == 1);
    assert(result.collapse_reason == "Subset of longer playlist");
    std::cout << "PASS: test_subset_playlists_policy" << std::endl;
}

void test_superset_playlists_policy() {
    auto result = FakeBdmvPlaylistDuplicateService::build_superset_playlists_policy();
    assert(result.request_id == "superset-playlists-001");
    assert(result.success == true);
    assert(result.unique_playlists.size() == 1);
    assert(result.collapsed_playlists.size() == 2);
    assert(result.collapse_reason == "Superset contains all streams");
    std::cout << "PASS: test_superset_playlists_policy" << std::endl;
}

void test_similar_playlists_policy() {
    auto result = FakeBdmvPlaylistDuplicateService::build_similar_playlists_policy();
    assert(result.request_id == "similar-playlists-001");
    assert(result.success == true);
    assert(result.unique_playlists.size() == 1);
    assert(result.collapsed_playlists.size() == 1);
    assert(result.collapse_reason == "Similar stream sequences (>90% match)");
    std::cout << "PASS: test_similar_playlists_policy" << std::endl;
}

int main() {
    test_no_duplicates_policy();
    test_identical_playlists_policy();
    test_subset_playlists_policy();
    test_superset_playlists_policy();
    test_similar_playlists_policy();
    
    std::cout << "All BDMV playlist duplicate tests passed!" << std::endl;
    return 0;
}
