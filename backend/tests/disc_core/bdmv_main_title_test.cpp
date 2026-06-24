#include "kivo/cinema_engine/disc_core/fake_bdmv_main_title_service.hpp"
#include <cassert>
#include <iostream>

using namespace kivo::cinema_engine;

void test_longest_duration_selection() {
    auto result = FakeBdmvMainTitleService::build_longest_duration_selection();
    assert(result.request_id == "longest-duration-001");
    assert(result.success == true);
    assert(result.selected_playlist == "00001.mpls");
    assert(result.selection_reason == "Longest duration (2h 15m)");
    assert(result.all_playlists.size() == 3);
    std::cout << "PASS: test_longest_duration_selection" << std::endl;
}

void test_most_chapters_selection() {
    auto result = FakeBdmvMainTitleService::build_most_chapters_selection();
    assert(result.request_id == "most-chapters-001");
    assert(result.success == true);
    assert(result.selected_playlist == "00002.mpls");
    assert(result.selection_reason == "Most chapters (32 chapters)");
    assert(result.all_playlists.size() == 3);
    std::cout << "PASS: test_most_chapters_selection" << std::endl;
}

void test_largest_size_selection() {
    auto result = FakeBdmvMainTitleService::build_largest_size_selection();
    assert(result.request_id == "largest-size-001");
    assert(result.success == true);
    assert(result.selected_playlist == "00003.mpls");
    assert(result.selection_reason == "Largest file size (45.2 GB)");
    assert(result.all_playlists.size() == 3);
    std::cout << "PASS: test_largest_size_selection" << std::endl;
}

void test_lowest_number_selection() {
    auto result = FakeBdmvMainTitleService::build_lowest_number_selection();
    assert(result.request_id == "lowest-number-001");
    assert(result.success == true);
    assert(result.selected_playlist == "00001.mpls");
    assert(result.selection_reason == "Lowest playlist number (00001)");
    assert(result.all_playlists.size() == 3);
    std::cout << "PASS: test_lowest_number_selection" << std::endl;
}

void test_user_override_selection() {
    auto result = FakeBdmvMainTitleService::build_user_override_selection();
    assert(result.request_id == "user-override-001");
    assert(result.success == true);
    assert(result.selected_playlist == "00002.mpls");
    assert(result.selection_reason == "User override selection");
    assert(result.all_playlists.size() == 3);
    std::cout << "PASS: test_user_override_selection" << std::endl;
}

int main() {
    test_longest_duration_selection();
    test_most_chapters_selection();
    test_largest_size_selection();
    test_lowest_number_selection();
    test_user_override_selection();
    
    std::cout << "All BDMV main title selection tests passed!" << std::endl;
    return 0;
}
