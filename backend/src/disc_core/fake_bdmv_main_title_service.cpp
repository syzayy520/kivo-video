#include "kivo/cinema_engine/disc_core/fake_bdmv_main_title_service.hpp"

namespace kivo::cinema_engine {

BdmvMainTitleResult FakeBdmvMainTitleService::build_longest_duration_selection() {
    BdmvMainTitleResult result;
    result.request_id = "longest-duration-001";
    result.success = true;
    result.selected_playlist = "00001.mpls";
    result.selection_reason = "Longest duration (2h 15m)";
    result.all_playlists = {"00001.mpls", "00002.mpls", "00003.mpls"};
    return result;
}

BdmvMainTitleResult FakeBdmvMainTitleService::build_most_chapters_selection() {
    BdmvMainTitleResult result;
    result.request_id = "most-chapters-001";
    result.success = true;
    result.selected_playlist = "00002.mpls";
    result.selection_reason = "Most chapters (32 chapters)";
    result.all_playlists = {"00001.mpls", "00002.mpls", "00003.mpls"};
    return result;
}

BdmvMainTitleResult FakeBdmvMainTitleService::build_largest_size_selection() {
    BdmvMainTitleResult result;
    result.request_id = "largest-size-001";
    result.success = true;
    result.selected_playlist = "00003.mpls";
    result.selection_reason = "Largest file size (45.2 GB)";
    result.all_playlists = {"00001.mpls", "00002.mpls", "00003.mpls"};
    return result;
}

BdmvMainTitleResult FakeBdmvMainTitleService::build_lowest_number_selection() {
    BdmvMainTitleResult result;
    result.request_id = "lowest-number-001";
    result.success = true;
    result.selected_playlist = "00001.mpls";
    result.selection_reason = "Lowest playlist number (00001)";
    result.all_playlists = {"00001.mpls", "00002.mpls", "00003.mpls"};
    return result;
}

BdmvMainTitleResult FakeBdmvMainTitleService::build_user_override_selection() {
    BdmvMainTitleResult result;
    result.request_id = "user-override-001";
    result.success = true;
    result.selected_playlist = "00002.mpls";
    result.selection_reason = "User override selection";
    result.all_playlists = {"00001.mpls", "00002.mpls", "00003.mpls"};
    return result;
}

} // namespace kivo::cinema_engine
