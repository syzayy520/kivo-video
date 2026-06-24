#pragma once

#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct ProbeRequest {
    ContractMetadata metadata;
    std::string source_uri;
    std::optional<std::string> format_hint;
    bool fast_probe{true};
};

}  // namespace kivo::cinema_engine