#include "video/video_plane/backends/d3d11/device/d3d11_video_device.hpp"

#include <windows.h>

#include <cstddef>
#include <string>

#include "video/video_plane/backends/d3d11/device/dxgi_adapter_selector.hpp"

namespace kivo::video::video_plane::backends::d3d11::device {
namespace {

[[nodiscard]] auto wide_to_utf8(const wchar_t* text) -> std::string {
    if (text == nullptr) {
        return {};
    }
    const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) {
        return {};
    }
    std::string buffer(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer.data(), required, nullptr, nullptr);
    buffer.resize(static_cast<std::size_t>(required - 1));
    return buffer;
}

constexpr D3D_FEATURE_LEVEL kRequestedFeatureLevels[] = {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
};

constexpr UINT kDeviceCreationFlags =
    D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT;

}  // namespace

D3d11VideoDevice::~D3d11VideoDevice() { release(); }

auto D3d11VideoDevice::initialize() noexcept -> bool {
    release();

    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                                 reinterpret_cast<void**>(&factory_)))) {
        factory_ = nullptr;
        return false;
    }

    const AdapterChoice choice = select_preferred_adapter(factory_);
    if (!choice.is_valid()) {
        release();
        return false;
    }

    D3D_FEATURE_LEVEL obtained_level = D3D_FEATURE_LEVEL_10_0;
    const HRESULT create_result = D3D11CreateDevice(
        choice.adapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        kDeviceCreationFlags,
        kRequestedFeatureLevels,
        ARRAYSIZE(kRequestedFeatureLevels),
        D3D11_SDK_VERSION,
        &device_,
        &obtained_level,
        &context_);

    if (FAILED(create_result)) {
        choice.adapter->Release();
        release();
        return false;
    }

    if (FAILED(device_->QueryInterface(__uuidof(ID3D11VideoDevice),
                                       reinterpret_cast<void**>(&video_device_)))) {
        video_device_ = nullptr;
    }

    choice.adapter->Release();

    if (video_device_ == nullptr) {
        // A device without ID3D11VideoDevice cannot decode anything. Report the
        // truth and fail instead of pretending the hardware path is available.
        release();
        return false;
    }

    if (SUCCEEDED(device_->QueryInterface(__uuidof(ID3D11Multithread),
                                          reinterpret_cast<void**>(&multithread_)))) {
        multithread_->SetMultithreadProtected(TRUE);
        contract_.multithread_protected = true;
    }

    contract_.kind = kivo::cinema_engine::video_plane_gpu::GpuDeviceKind::Hardware;
    contract_.adapter.name = wide_to_utf8(choice.desc.Description);
    contract_.adapter.vendor_id = choice.desc.VendorId;
    contract_.adapter.device_id = choice.desc.DeviceId;
    contract_.adapter.dedicated_video_memory_bytes =
        static_cast<std::uint64_t>(choice.desc.DedicatedVideoMemory);
    contract_.adapter.is_software = false;
    contract_.video_decode_capable = true;

    return true;
}

void D3d11VideoDevice::release() noexcept {
    if (multithread_ != nullptr) {
        multithread_->Release();
        multithread_ = nullptr;
    }
    if (video_device_ != nullptr) {
        video_device_->Release();
        video_device_ = nullptr;
    }
    if (context_ != nullptr) {
        context_->Release();
        context_ = nullptr;
    }
    if (device_ != nullptr) {
        device_->Release();
        device_ = nullptr;
    }
    if (factory_ != nullptr) {
        factory_->Release();
        factory_ = nullptr;
    }
    contract_ = {};
}

}  // namespace kivo::video::video_plane::backends::d3d11::device
