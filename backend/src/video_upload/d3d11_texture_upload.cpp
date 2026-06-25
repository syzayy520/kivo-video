#include "kivo/cinema_engine/video_upload/d3d11_texture_upload.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_internal.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_device_context.hpp"

#include <iostream>
#include <cstring>
#include <chrono>

namespace kivo::cinema_engine {

bool D3d11TextureUpload::initialize(D3d11DeviceContext* device_context) {
    device_context_ = device_context;
    initialized_ = (device_context != nullptr && device_context->is_ready());
    return initialized_;
}

// Determine D3D11 DXGI format from pixel_format string.
#ifdef _WIN32
static DXGI_FORMAT pixel_format_to_dxgi(const std::string& pixel_format) {
    if (pixel_format == "nv12")       return DXGI_FORMAT_NV12;
    if (pixel_format == "yuv420p")    return DXGI_FORMAT_NV12;  // DXGI has no YUV420P; NV12 is closest GPU-native
    if (pixel_format == "yuv422p")    return DXGI_FORMAT_YUY2;
    if (pixel_format == "rgb24")      return DXGI_FORMAT_R8G8B8A8_UNORM;
    if (pixel_format == "bgr24")      return DXGI_FORMAT_B8G8R8A8_UNORM;
    if (pixel_format == "rgba32")     return DXGI_FORMAT_R8G8B8A8_UNORM;
    if (pixel_format == "bgra32")     return DXGI_FORMAT_B8G8R8A8_UNORM;
    return DXGI_FORMAT_UNKNOWN;
}
#endif

D3D11TextureHandle D3d11TextureUpload::create_texture(int32_t width, int32_t height,
                                                      const std::string& pixel_format) {
#ifdef _WIN32
    if (!device_context_ || !device_context_->is_ready() || !device_context_->handle()) {
        std::cerr << "[D3d11TextureUpload] Device context not ready\n";
        return nullptr;
    }

    auto* ctx_impl = static_cast<D3d11DeviceContextImpl*>(device_context_->handle());
    if (!ctx_impl->device) {
        std::cerr << "[D3d11TextureUpload] No D3D11 device\n";
        return nullptr;
    }

    DXGI_FORMAT dxgi_format = pixel_format_to_dxgi(pixel_format);
    if (dxgi_format == DXGI_FORMAT_UNKNOWN) {
        std::cerr << "[D3d11TextureUpload] Unknown pixel format: " << pixel_format << "\n";
        return nullptr;
    }

    // For YUV420P, create NV12 texture (width x height/2 packed).
    // For other formats, create R8G8B8A8_UNORM or matching format.
    D3D11_TEXTURE2D_DESC tex_desc{};
    tex_desc.Width = static_cast<UINT>(width);
    tex_desc.Height = static_cast<UINT>(height);
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = dxgi_format;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.MiscFlags = 0;

    if (pixel_format == "yuv420p") {
        // NV12: width x height/2 with interleaved UV
        tex_desc.Height = static_cast<UINT>((height + 1) / 2 * 2);  // ensure even
        tex_desc.Width = static_cast<UINT>(width);
    }

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = ctx_impl->device->CreateTexture2D(&tex_desc, nullptr, &texture);
    if (FAILED(hr) || !texture) {
        std::cerr << "[D3d11TextureUpload] CreateTexture2D failed: 0x"
                  << std::hex << hr << std::dec << "\n";
        return nullptr;
    }

    auto* tex_impl = new D3d11TextureImpl();
    tex_impl->texture = texture;
    tex_impl->width = width;
    tex_impl->height = height;
    tex_impl->pixel_format = pixel_format;

    initialized_ = true;

    std::cout << "[D3d11TextureUpload] Created real texture " << width << "x" << height
              << " (" << pixel_format << ", DXGI=" << dxgi_format << ")\n";

    return static_cast<D3D11TextureHandle>(tex_impl);
#else
    (void)width; (void)height; (void)pixel_format;
    return nullptr;
#endif
}

bool D3d11TextureUpload::upload_frame(D3D11TextureHandle texture, const DecodedFrame& frame,
                                      const YuvRgbConversion& conversion) {
#ifdef _WIN32
    if (!texture) return false;

    auto* tex_impl = static_cast<D3d11TextureImpl*>(texture);
    if (!tex_impl->texture) return false;

    if (!device_context_ || !device_context_->is_ready() || !device_context_->handle()) {
        std::cerr << "[D3d11TextureUpload] Device context not ready for upload\n";
        return false;
    }

    auto* ctx_impl = static_cast<D3d11DeviceContextImpl*>(device_context_->handle());
    if (!ctx_impl->device_context) return false;

    // Real upload: copy frame_data into the GPU texture via UpdateSubresource.
    if (frame.frame_data.empty()) {
        std::cerr << "[D3d11TextureUpload] No frame data to upload\n";
        return false;
    }

    // Calculate expected data size for validation.
    size_t expected_size = 0;
    if (frame.pixel_format == "yuv420p") {
        // Y plane: width * height, UV planes: width * height/2
        expected_size = static_cast<size_t>(frame.width) * frame.height * 3 / 2;
    } else if (frame.pixel_format == "nv12") {
        expected_size = static_cast<size_t>(frame.width) * frame.height * 3 / 2;
    } else if (frame.pixel_format == "rgb24" || frame.pixel_format == "bgr24") {
        expected_size = static_cast<size_t>(frame.width) * frame.height * 3;
    } else {
        expected_size = static_cast<size_t>(frame.width) * frame.height * 4;
    }

    if (frame.frame_data.size() < expected_size) {
        std::cerr << "[D3d11TextureUpload] Frame data too small: "
                  << frame.frame_data.size() << " < " << expected_size << "\n";
        return false;
    }

    // Use Map/Unmap for efficient GPU upload.
    ID3D11DeviceContext* dctx = ctx_impl->device_context;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = dctx->Map(tex_impl->texture, 0, D3D11_MAP_WRITE, 0, &mapped);
    if (SUCCEEDED(hr)) {
        // Copy row by row (GPU row pitch may differ from source).
        const size_t src_pitch = (frame.pixel_format == "yuv420p" || frame.pixel_format == "nv12")
            ? static_cast<size_t>(frame.width)
            : static_cast<size_t>(frame.width) * 4;

        const size_t dst_pitch = mapped.RowPitch;

        const uint8_t* src = frame.frame_data.data();
        for (UINT row = 0; row < tex_impl->height; ++row) {
            memcpy(static_cast<uint8_t*>(mapped.pData) + row * dst_pitch,
                   src + row * src_pitch,
                   src_pitch);
        }

        dctx->Unmap(tex_impl->texture, 0);

        std::cout << "[D3d11TextureUpload] Uploaded " << frame.frame_data.size()
                  << " bytes to GPU texture " << tex_impl->width << "x" << tex_impl->height << "\n";
        return true;
    }

    // Fallback: UpdateSubresource (slower but always works).
    // Note: UpdateSubresource returns void, not HRESULT.
    dctx->UpdateSubresource(tex_impl->texture, 0, nullptr,
                            frame.frame_data.data(),
                            static_cast<UINT>(frame.pixel_format == "yuv420p" ? frame.width : frame.width * 4),
                            0);
    std::cout << "[D3d11TextureUpload] Uploaded via UpdateSubresource: "
              << frame.frame_data.size() << " bytes\n";
    return true;
#else
    (void)texture; (void)frame; (void)conversion;
    return false;
#endif
}

void D3d11TextureUpload::get_texture_dimensions(D3D11TextureHandle texture,
                                                int32_t& width, int32_t& height) const {
    if (texture) {
#ifdef _WIN32
        auto* tex_impl = static_cast<D3d11TextureImpl*>(texture);
        width = tex_impl->width;
        height = tex_impl->height;
        return;
#else
        (void)texture;
#endif
    }
    width = 0;
    height = 0;
}

void D3d11TextureUpload::release_texture(D3D11TextureHandle texture) {
#ifdef _WIN32
    if (texture) {
        auto* tex_impl = static_cast<D3d11TextureImpl*>(texture);
        if (tex_impl->srv) {
            tex_impl->srv->Release();
            tex_impl->srv = nullptr;
        }
        if (tex_impl->texture) {
            tex_impl->texture->Release();
            tex_impl->texture = nullptr;
        }
        delete tex_impl;
    }
#else
    (void)texture;
#endif
}

void D3d11TextureUpload::release() {
    // Note: Individual textures must be released via release_texture() before this.
    device_context_ = nullptr;
    initialized_ = false;
}

bool D3d11TextureUpload::is_ready() const {
    return initialized_;
}

}  // namespace kivo::cinema_engine
