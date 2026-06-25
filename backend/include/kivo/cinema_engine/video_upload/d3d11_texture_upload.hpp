#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kivo/cinema_engine/decode_core/decoded_frame.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_device_context.hpp"
#include "kivo/cinema_engine/video_upload/yuv_rgb_conversion.hpp"

namespace kivo::cinema_engine {

// Opaque handle to D3D11 texture. Actual type stays inside adapter boundary.
using D3D11TextureHandle = void*;

// TextureUploadResult contains the result of a texture upload.
struct TextureUploadResult {
    bool success{false};
    D3D11TextureHandle texture{nullptr};
    int32_t width{0};
    int32_t height{0};
    std::string pixel_format;
    std::string error_message;
    bool is_warp{false};
    uint64_t upload_time_us{0};  // microseconds
};

// D3d11TextureUpload manages texture creation and upload.
// Actual D3D11 API calls stay inside adapter boundary.
struct D3d11TextureUpload {
    ContractMetadata metadata;

    // Create a texture for the given frame dimensions and format.
    // Returns a handle to the texture (nullptr on failure).
    D3D11TextureHandle create_texture(int32_t width, int32_t height,
                                      const std::string& pixel_format);

    // Upload frame data to an existing texture.
    // Returns true if successful.
    bool upload_frame(D3D11TextureHandle texture, const DecodedFrame& frame,
                      const YuvRgbConversion& conversion);

    // Get texture dimensions.
    void get_texture_dimensions(D3D11TextureHandle texture,
                                int32_t& width, int32_t& height) const;

    // Release a texture.
    void release_texture(D3D11TextureHandle texture);

    // Release all resources.
    void release();

    // Check if ready.
    bool is_ready() const;

private:
    D3d11DeviceContext* device_context_{nullptr};
    bool initialized_{false};
};

}  // namespace kivo::cinema_engine