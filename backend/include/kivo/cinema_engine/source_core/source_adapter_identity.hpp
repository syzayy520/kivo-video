#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Identifies a source adapter by protocol, host, and credential scope.
struct SourceAdapterIdentity {
    ContractMetadata metadata;
    std::string adapter_id;
    std::string adapter_kind;   // "local", "webdav", "alist", "emby"
    std::string protocol;       // "file", "http", "https", "dav"
    std::string host;
    bool requires_auth{false};
    std::string credential_scope;
};

}  // namespace kivo::cinema_engine
