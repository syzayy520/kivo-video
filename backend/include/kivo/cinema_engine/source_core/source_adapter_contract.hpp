#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Source adapter boundary contract describing a concrete source backend.
// Each adapter (local file, WebDAV, AList, Emby) produces one of these
// to declare its identity, capabilities, and boundary validity.
struct SourceAdapterContract {
    ContractMetadata metadata;
    std::string adapter_id;
    std::string adapter_kind;  // "local", "webdav", "alist", "emby"
    std::vector<std::string> supported_operations;
    bool boundary_valid{true};
};

}  // namespace kivo::cinema_engine
