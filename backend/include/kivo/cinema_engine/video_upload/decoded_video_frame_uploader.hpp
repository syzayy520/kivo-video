#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kivo/cinema_engine/decode_core/decoded_frame.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Opaque handle to D3D11 texture. Actual type stays inside adapter boundary.
using D3D11TextureHandle = void*;

// DecodedVideoFrameUploader is the interface for uploading decoded video frames
// to GPU texture (D3D11 hardware or WARP). Actual D3D11 calls stay inside
// adapter boundary. This interface is platform-neutral.
struct DecodedVideoFrameUploader {
    ContractMetadata metadata;

    // Upload a decoded video frame to a GPU texture.
    // Returns a handle to the uploaded texture (D3D11TextureHandle).
    // The caller must not free the handle; it is managed by the uploader.
    // Returns nullptr on failure.
    virtual D3D11TextureHandle upload(const DecodedFrame& frame) = 0;

    // Check if uploader is initialized and ready.
    virtual bool is_ready() const = 0;

    // Get the last error message (empty if no error).
    virtual std::string last_error() const = 0;

    // Release all resources.
    virtual void release() = 0;

    virtual ~DecodedVideoFrameUploader() = default;
};

// UploadResult contains the result of a frame upload.
struct UploadResult {
    bool success{false};
    D3D11TextureHandle texture{nullptr};
    int32_t width{0};
    int32_t height{0};
    std::string pixel_format;
    std::string error_message;
    bool is_warp{false};  // true if WARP adapter used
};

}  // namespace kivo::cinema_engine