#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// AList API operation capabilities.
struct AListApiBehavior {
    ContractMetadata metadata;
    std::string behavior_id;
    bool supports_list{false};                // Can list directory contents
    bool supports_get{false};                 // Can get file/directory info
    bool supports_search{false};              // Can search files
    bool supports_rename{false};              // Can rename files
    bool supports_move{false};                // Can move files
    bool supports_copy{false};                // Can copy files
    bool supports_delete{false};              // Can delete files
    bool supports_mkdir{false};               // Can create directories
    bool supports_link_generation{false};     // Can generate direct download links
};

}  // namespace kivo::cinema_engine
