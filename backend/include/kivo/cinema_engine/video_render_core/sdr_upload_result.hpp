#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct SdrUploadResult {
    ContractMetadata metadata;
    std::string upload_id;
    bool success{false};
    std::string error_message;
    int32_t frames_processed{0};
    int32_t frames_skipped{0};
};

}  // namespace kivo::cinema_engine