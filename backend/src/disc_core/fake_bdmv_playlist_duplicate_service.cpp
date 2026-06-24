#include "kivo/cinema_engine/disc_core/fake_bdmv_playlist_duplicate_service.hpp"

namespace kivo::cinema_engine {

BdmvPlaylistDuplicateResult FakeBdmvPlaylistDuplicateService::build_no_duplicates_policy() {
    BdmvPlaylistDuplicateResult result;
    result.request_id = "no-duplicates-001";
    result.success = true;
    result.unique_playlists = {"00001.mpls", "00002.mpls", "00003.mpls"};
    result.collapsed_playlists = {};
    result.collapse_reason = "No duplicates found";
    return result;
}

BdmvPlaylistDuplicateResult FakeBdmvPlaylistDuplicateService::build_identical_playlists_policy() {
    BdmvPlaylistDuplicateResult result;
    result.request_id = "identical-playlists-001";
    result.success = true;
    result.unique_playlists = {"00001.mpls"};
    result.collapsed_playlists = {"00002.mpls", "00003.mpls"};
    result.collapse_reason = "Identical stream sequences";
    return result;
}

BdmvPlaylistDuplicateResult FakeBdmvPlaylistDuplicateService::build_subset_playlists_policy() {
    BdmvPlaylistDuplicateResult result;
    result.request_id = "subset-playlists-001";
    result.success = true;
    result.unique_playlists = {"00001.mpls"};
    result.collapsed_playlists = {"00002.mpls"};
    result.collapse_reason = "Subset of longer playlist";
    return result;
}

BdmvPlaylistDuplicateResult FakeBdmvPlaylistDuplicateService::build_superset_playlists_policy() {
    BdmvPlaylistDuplicateResult result;
    result.request_id = "superset-playlists-001";
    result.success = true;
    result.unique_playlists = {"00001.mpls"};
    result.collapsed_playlists = {"00002.mpls", "00003.mpls"};
    result.collapse_reason = "Superset contains all streams";
    return result;
}

BdmvPlaylistDuplicateResult FakeBdmvPlaylistDuplicateService::build_similar_playlists_policy() {
    BdmvPlaylistDuplicateResult result;
    result.request_id = "similar-playlists-001";
    result.success = true;
    result.unique_playlists = {"00001.mpls"};
    result.collapsed_playlists = {"00002.mpls"};
    result.collapse_reason = "Similar stream sequences (>90% match)";
    return result;
}

} // namespace kivo::cinema_engine
