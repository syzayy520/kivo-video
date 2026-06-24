#pragma once

#include "bdmv_playlist_duplicate_request.hpp"
#include "bdmv_playlist_duplicate_result.hpp"

namespace kivo::cinema_engine {

// Provides deterministic BDMV playlist duplicate policy results for testing
class FakeBdmvPlaylistDuplicateService {
public:
    // No duplicates found
    static BdmvPlaylistDuplicateResult build_no_duplicates_policy();
    
    // Identical playlists collapsed
    static BdmvPlaylistDuplicateResult build_identical_playlists_policy();
    
    // Subset playlists collapsed
    static BdmvPlaylistDuplicateResult build_subset_playlists_policy();
    
    // Superset playlists collapsed
    static BdmvPlaylistDuplicateResult build_superset_playlists_policy();
    
    // Similar playlists collapsed
    static BdmvPlaylistDuplicateResult build_similar_playlists_policy();
};

} // namespace kivo::cinema_engine
