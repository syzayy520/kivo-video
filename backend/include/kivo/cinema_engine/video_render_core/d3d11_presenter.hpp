#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// PresenterRuntimeContract is the contract for platform-specific video presentation.
// Actual platform types (swap chain, device, surface) stay inside the adapter boundary.
struct PresenterRuntimeContract {
    ContractMetadata metadata;
    std::string presenter_id;
    std::string presenter_backend;  // "d3d11_flip", "d3d11_bitblt"
    bool is_initialized{false};
    bool is_fullscreen{false};
    int32_t width{0};
    int32_t height{0};
    std::string swapchain_format;  // "rgba8", "rgba16f", "nv12"
    bool hdr_enabled{false};
    bool zero_copy{false};
    std::string copy_back_reason;
    int64_t frames_presented{0};
};

}  // namespace kivo::cinema_engine
