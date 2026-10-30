#pragma once

#include <dxgi1_2.h>

namespace kivo::video::video_plane::backends::d3d11::device {

// Result of adapter selection. `adapter` is owned by the caller and must be
// released with Release(); it is null when no usable hardware adapter exists.
struct AdapterChoice {
    IDXGIAdapter1* adapter{nullptr};
    DXGI_ADAPTER_DESC1 desc{};

    [[nodiscard]] auto is_valid() const noexcept -> bool { return adapter != nullptr; }
};

// Enumerate the adapters behind `factory` and pick the one most likely to
// hardware-decode video: software adapters are skipped, the remaining adapter
// with the largest dedicated video memory wins.
// Does not create a device and does not retain the factory.
[[nodiscard]] auto select_preferred_adapter(IDXGIFactory1* factory) noexcept -> AdapterChoice;

}  // namespace kivo::video::video_plane::backends::d3d11::device
