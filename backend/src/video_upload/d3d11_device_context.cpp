#include "kivo/cinema_engine/video_upload/d3d11_device_context.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_internal.hpp"

#include <iostream>
#include <cstring>

namespace kivo::cinema_engine {

bool D3d11DeviceContext::initialize() {
#ifdef _WIN32
    auto* impl = new D3d11DeviceContextImpl();

    const UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    // Try hardware adapter first, then WARP fallback.
    const D3D_DRIVER_TYPE driver_types[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
    };

    for (auto dt : driver_types) {
        HRESULT hr = D3D11CreateDevice(
            nullptr,                    // default adapter
            dt,                         // driver type
            nullptr,                    // software module
            create_flags,               // flags
            nullptr, 0,                 // feature levels
            D3D11_SDK_VERSION,
            &impl->device,
            nullptr,                    // feature level out
            &impl->device_context);

        if (SUCCEEDED(hr)) {
            impl->driver_type = dt;
            impl->device_type = (dt == D3D_DRIVER_TYPE_HARDWARE) ? "hardware" : "warp";

            // Query adapter description via DXGI.
            IDXGIDevice* dxgi_device = nullptr;
            if (SUCCEEDED(impl->device->QueryInterface(__uuidof(IDXGIDevice),
                                                       reinterpret_cast<void**>(&dxgi_device)))) {
                if (SUCCEEDED(dxgi_device->GetAdapter(&impl->adapter))) {
                    DXGI_ADAPTER_DESC desc{};
                    if (SUCCEEDED(impl->adapter->GetDesc(&desc))) {
                        char name[128]{};
                        wcstombs(name, desc.Description, sizeof(name) - 1);
                        impl->adapter_description = name;
                    }
                }
                dxgi_device->Release();
            }

            if (impl->adapter_description.empty()) {
                impl->adapter_description = (dt == D3D_DRIVER_TYPE_HARDWARE)
                    ? "Hardware D3D11 Adapter"
                    : "Microsoft Basic Render Driver (WARP)";
            }

            impl_ = static_cast<D3D11DeviceContextHandle>(impl);
            device_type_ = impl->device_type;
            adapter_description_ = impl->adapter_description;
            initialized_ = true;

            std::cout << "[D3d11DeviceContext] Initialized: " << device_type_
                      << " (" << adapter_description_ << ")\n";
            return true;
        }
    }

    // All driver types failed.
    delete impl;
    std::cerr << "[D3d11DeviceContext] D3D11CreateDevice failed for all driver types\n";
    return false;
#else
    std::cerr << "[D3d11DeviceContext] D3D11 not available on this platform\n";
    return false;
#endif
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
#ifdef _WIN32
    if (impl_) {
        auto* ctx = static_cast<D3d11DeviceContextImpl*>(impl_);
        if (ctx->device_context) {
            ctx->device_context->Release();
            ctx->device_context = nullptr;
        }
        if (ctx->device) {
            ctx->device->Release();
            ctx->device = nullptr;
        }
        if (ctx->adapter) {
            ctx->adapter->Release();
            ctx->adapter = nullptr;
        }
        delete ctx;
        impl_ = nullptr;
    }
#endif
    device_type_.clear();
    adapter_description_.clear();
    initialized_ = false;
}

D3D11DeviceContextHandle D3d11DeviceContext::handle() const {
    return impl_;
}

}  // namespace kivo::cinema_engine
