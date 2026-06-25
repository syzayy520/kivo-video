#include "kivo/cinema_engine/video_upload/d3d11_device_context.hpp"

#include <iostream>

namespace kivo::cinema_engine {

bool D3d11DeviceContext::initialize() {
    // TODO: Real D3D11 device initialization
    // For now, simulate success with WARP fallback
    device_type_ = "warp";
    adapter_description_ = "Microsoft Basic Render Driver (WARP)";
    initialized_ = true;
    std::cout << "[D3d11DeviceContext] Initialized with WARP adapter\n";
    return true;
}

bool D3d11DeviceContext::is_ready() const {
    return initialized_;
}

std::string D3d11DeviceContext::device_type() const {
    return device_type_;
}

std::string D3d11DeviceContext::adapter_description() const {
    return adapter_description_;
}

void D3d11DeviceContext::release() {
    if (impl_) {
        // TODO: Real D3D11 release
        impl_ = nullptr;
    }
    initialized_ = false;
    std::cout << "[D3d11DeviceContext] Released\n";
}

D3D11DeviceContextHandle D3d11DeviceContext::handle() const {
    return impl_;
}

}  // namespace kivo::cinema_engine