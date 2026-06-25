#include "kivo/cinema_engine/video_upload/d3d11_texture_upload.hpp"

#include <iostream>

namespace kivo::cinema_engine {

D3D11TextureHandle D3d11TextureUpload::create_texture(int32_t width, int32_t height,
                                                      const std::string& pixel_format) {
    // TODO: Real D3D11 texture creation
    // Simulate texture creation
    std::cout << "[D3d11TextureUpload] Created texture " << width << "x" << height
              << " (" << pixel_format << ")\n";
    return reinterpret_cast<D3D11TextureHandle>(new bool(true));
}

bool D3d11TextureUpload::upload_frame(D3D11TextureHandle texture, const DecodedFrame& frame,
                                      const YuvRgbConversion& conversion) {
    if (!texture) return false;
    // TODO: Real frame upload
    std::cout << "[D3d11TextureUpload] Uploaded frame " << frame.width << "x" << frame.height
              << " (" << frame.pixel_format << ")\n";
    return true;
}

void D3d11TextureUpload::get_texture_dimensions(D3D11TextureHandle texture,
                                                int32_t& width, int32_t& height) const {
    if (texture) {
        // TODO: Real dimension query
        width = 320;
        height = 240;
    }
}

void D3d11TextureUpload::release_texture(D3D11TextureHandle texture) {
    if (texture) {
        delete static_cast<bool*>(texture);
        std::cout << "[D3d11TextureUpload] Released texture\n";
    }
}

void D3d11TextureUpload::release() {
    // TODO: Release all textures
    std::cout << "[D3d11TextureUpload] Released all resources\n";
}

bool D3d11TextureUpload::is_ready() const {
    return initialized_;
}

}  // namespace kivo::cinema_engine