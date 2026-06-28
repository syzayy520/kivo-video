// wasapi_device_enumerator_guard.hpp — P6D internal: RAII guard for IMMDeviceEnumerator
// INTERNAL header: includes WASAPI headers (allowed under backend/src/.../internal/).
#pragma once

#include <mmdeviceapi.h>
#include <wrl/client.h>

namespace kivo::video::audio_plane::backend::output::wasapi::internal {

// RAII guard for IMMDeviceEnumerator.
// Creates the enumerator via CoCreateInstance and releases it on destruction.
class DeviceEnumeratorGuard {
public:
    DeviceEnumeratorGuard() noexcept = default;

    HRESULT initialize() noexcept {
        return ::CoCreateInstance(
            __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            reinterpret_cast<void**>(enumerator_.GetAddressOf()));
    }

    IMMDeviceEnumerator* get() noexcept { return enumerator_.Get(); }
    const IMMDeviceEnumerator* get() const noexcept { return enumerator_.Get(); }
    bool is_valid() const noexcept { return enumerator_ != nullptr; }

private:
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator_;
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi::internal
