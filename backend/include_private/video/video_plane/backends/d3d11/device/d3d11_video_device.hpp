#pragma once

#include <d3d11_4.h>
#include <dxgi1_2.h>

#include "kivo/cinema_engine/video_plane_gpu/device/gpu_device_contract.hpp"

namespace kivo::video::video_plane::backends::d3d11::device {

// Owns the graphics device used for hardware video decode.
// Single responsibility: create a video-capable device and report what it is.
// It does not decode, does not present, and does not own any media state.
class D3d11VideoDevice {
public:
    D3d11VideoDevice() = default;
    ~D3d11VideoDevice();

    D3d11VideoDevice(const D3d11VideoDevice&) = delete;
    auto operator=(const D3d11VideoDevice&) -> D3d11VideoDevice& = delete;
    D3d11VideoDevice(D3d11VideoDevice&&) = delete;
    auto operator=(D3d11VideoDevice&&) -> D3d11VideoDevice& = delete;

    // Create the device on the preferred hardware adapter.
    // Returns false when no hardware adapter can host a video device;
    // callers must treat that as "hardware unavailable", never as a soft success.
    [[nodiscard]] auto initialize() noexcept -> bool;

    void release() noexcept;

    [[nodiscard]] auto is_ready() const noexcept -> bool { return device_ != nullptr; }
    [[nodiscard]] auto device() const noexcept -> ID3D11Device* { return device_; }
    [[nodiscard]] auto device_context() const noexcept -> ID3D11DeviceContext* { return context_; }
    [[nodiscard]] auto video_device() const noexcept -> ID3D11VideoDevice* { return video_device_; }
    [[nodiscard]] auto contract() const noexcept
        -> const kivo::cinema_engine::video_plane_gpu::GpuDeviceContract& {
        return contract_;
    }

private:
    IDXGIFactory1* factory_{nullptr};
    ID3D11Device* device_{nullptr};
    ID3D11DeviceContext* context_{nullptr};
    ID3D11VideoDevice* video_device_{nullptr};
    ID3D11Multithread* multithread_{nullptr};
    kivo::cinema_engine::video_plane_gpu::GpuDeviceContract contract_{};
};

}  // namespace kivo::video::video_plane::backends::d3d11::device
