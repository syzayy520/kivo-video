// wasapi_device_enumerator.hpp — P6D backend: device enumeration + notification
#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::audio_plane::backend::output::wasapi {

struct DeviceInfo {
    std::string device_id;
    std::string friendly_name;
    bool is_default{false};
};

// WasapiDeviceEnumerator
// Enumerates WASAPI render endpoints. Real implementation uses IMMDeviceEnumerator.
class WasapiDeviceEnumerator {
public:
    WasapiDeviceEnumerator() = default;

    // Initialize COM + create enumerator. Returns false on failure.
    bool initialize();

    // Get default render endpoint.
    bool get_default_device(DeviceInfo& out);

    // Check if a device with the given ID exists.
    bool device_exists(const std::string& device_id);

    bool is_valid() const noexcept { return valid_; }

private:
    bool valid_{false};
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi
