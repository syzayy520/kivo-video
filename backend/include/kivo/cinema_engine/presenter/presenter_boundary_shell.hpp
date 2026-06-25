#pragma once

#include <cstdint>
#include <string>
#include <chrono>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_texture_upload.hpp"

namespace kivo::cinema_engine {

// PresenterBoundaryShell is the presenter boundary that receives uploaded texture
// and records first-frame proof. This is the interface between video upload and
// actual presentation (D3D11 flip/bitblt).
struct PresenterBoundaryShell {
    ContractMetadata metadata;

    // Initialize the presenter boundary.
    bool initialize();

    // Receive an uploaded texture and present it.
    // Returns true if successful.
    bool present(D3D11TextureHandle texture, int32_t width, int32_t height);

    // Get the first frame presentation time (microseconds since epoch).
    // Returns 0 if no frame presented yet.
    int64_t first_frame_time_us() const;

    // Get the total frames presented.
    int64_t frames_presented() const;

    // Check if first frame has been presented.
    bool has_first_frame() const;

    // Get presenter status.
    std::string status() const;

    // Release all resources.
    void release();

    // Check if ready.
    bool is_ready() const;

private:
    bool initialized_{false};
    int64_t first_frame_time_us_{0};
    int64_t frames_presented_{0};
    bool has_first_frame_{false};
};

}  // namespace kivo::cinema_engine