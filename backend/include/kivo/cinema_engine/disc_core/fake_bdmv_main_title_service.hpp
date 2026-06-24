#pragma once

#include "bdmv_main_title_request.hpp"
#include "bdmv_main_title_result.hpp"

namespace kivo::cinema_engine {

// Provides deterministic BDMV main title selection results for testing
class FakeBdmvMainTitleService {
public:
    // Select by longest duration
    static BdmvMainTitleResult build_longest_duration_selection();
    
    // Select by most chapters
    static BdmvMainTitleResult build_most_chapters_selection();
    
    // Select by largest file size
    static BdmvMainTitleResult build_largest_size_selection();
    
    // Select by lowest playlist number
    static BdmvMainTitleResult build_lowest_number_selection();
    
    // Select user override playlist
    static BdmvMainTitleResult build_user_override_selection();
};

} // namespace kivo::cinema_engine
