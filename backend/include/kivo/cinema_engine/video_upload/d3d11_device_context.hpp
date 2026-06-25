#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Opaque handle to D3D11 device context. Actual type stays inside adapter boundary.
using D3D11DeviceContextHandle = void*;

// D3d11DeviceContext manages D3D11 device/context lifecycle.
// Actual D3D11 API calls stay inside adapter boundary.
struct D3d11DeviceContext {
    ContractMetadata metadata;

    // Initialize D3D11 device (hardware or WARP fallback).
    // Returns true if successful.
    bool initialize();

    // Check if device is initialized and ready.
    bool is_ready() const;

    // Get device type ("hardware" or "warp").
    std::string device_type() const;

    // Get adapter description.
    std::string adapter_description() const;

    // Release all resources.
    void release();

    // Get the underlying handle (for adapter boundary use).
    D3D11DeviceContextHandle handle() const;

private:
    D3D11DeviceContextHandle impl_{nullptr};
    std::string device_type_;
    std::string adapter_description_;
    bool initialized_{false};
};

}  // namespace kivo::cinema_engine