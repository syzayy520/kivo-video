#include "video/video_plane/backends/d3d11/device/dxgi_adapter_selector.hpp"

namespace kivo::video::video_plane::backends::d3d11::device {
namespace {

[[nodiscard]] auto is_software_adapter(const DXGI_ADAPTER_DESC1& desc) noexcept -> bool {
    return (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
}

// Returns the better of two candidates by dedicated video memory.
[[nodiscard]] auto is_better_candidate(const DXGI_ADAPTER_DESC1& candidate,
                                       const DXGI_ADAPTER_DESC1& incumbent) noexcept -> bool {
    return candidate.DedicatedVideoMemory > incumbent.DedicatedVideoMemory;
}

}  // namespace

auto select_preferred_adapter(IDXGIFactory1* factory) noexcept -> AdapterChoice {
    AdapterChoice choice{};
    if (factory == nullptr) {
        return choice;
    }

    for (UINT index = 0;; ++index) {
        IDXGIAdapter1* candidate = nullptr;
        if (factory->EnumAdapters1(index, &candidate) == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (candidate == nullptr) {
            continue;
        }

        DXGI_ADAPTER_DESC1 desc{};
        if (FAILED(candidate->GetDesc1(&desc)) || is_software_adapter(desc)) {
            candidate->Release();
            continue;
        }

        if (choice.is_valid() && !is_better_candidate(desc, choice.desc)) {
            candidate->Release();
            continue;
        }

        if (choice.is_valid()) {
            choice.adapter->Release();
        }
        choice.adapter = candidate;
        choice.desc = desc;
    }

    return choice;
}

}  // namespace kivo::video::video_plane::backends::d3d11::device
