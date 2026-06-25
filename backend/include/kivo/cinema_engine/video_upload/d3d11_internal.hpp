#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d11.h>
#include <dxgi.h>
#endif

#include <cstdint>
#include <string>

namespace kivo::cinema_engine {

// Internal D3D11 device/context state, hidden behind opaque handle.
struct D3d11DeviceContextImpl {
#ifdef _WIN32
    ID3D11Device* device{nullptr};
    ID3D11DeviceContext* device_context{nullptr};
    IDXGIAdapter* adapter{nullptr};
    D3D_DRIVER_TYPE driver_type{D3D_DRIVER_TYPE_HARDWARE};
#endif
    std::string device_type;
    std::string adapter_description;
};

// Internal D3D11 texture state, hidden behind opaque handle.
struct D3d11TextureImpl {
#ifdef _WIN32
    ID3D11Texture2D* texture{nullptr};
    ID3D11ShaderResourceView* srv{nullptr};
#endif
    int32_t width{0};
    int32_t height{0};
    std::string pixel_format;
};

}  // namespace kivo::cinema_engine
