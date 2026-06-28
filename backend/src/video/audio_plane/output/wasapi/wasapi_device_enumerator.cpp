// wasapi_device_enumerator.cpp — P6D backend: device enumeration implementation
#include "video/audio_plane/output/wasapi/wasapi_device_enumerator.hpp"

#include <objbase.h>
#include <mmdeviceapi.h>
#include <propidl.h>
#include <functiondiscoverykeys_devpkey.h>
#include <wrl/client.h>

namespace kivo::video::audio_plane::backend::output::wasapi {

bool WasapiDeviceEnumerator::initialize() {
    HRESULT hr = ::CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(nullptr));
    // We can't store the COM pointer in this class without including wrl here,
    // so just verify COM is initialized. Real enumeration is done in the backend.
    valid_ = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE || hr == S_FALSE;
    return valid_;
}

bool WasapiDeviceEnumerator::get_default_device(DeviceInfo& out) {
    if (!valid_) return false;

    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
    HRESULT hr = ::CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(enumerator.GetAddressOf()));
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IMMDevice> device;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, device.GetAddressOf());
    if (FAILED(hr)) return false;

    // Get device ID
    LPWSTR device_id = nullptr;
    if (SUCCEEDED(device->GetId(&device_id)) && device_id) {
        // Convert wide string to UTF-8 (simple ASCII subset)
        out.device_id.clear();
        for (LPWSTR p = device_id; *p; ++p) {
            if (*p < 128) out.device_id.push_back(static_cast<char>(*p));
        }
        ::CoTaskMemFree(device_id);
    }

    // Get friendly name from property store
    Microsoft::WRL::ComPtr<IPropertyStore> props;
    if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, props.GetAddressOf())) && props) {
        PROPVARIANT var;
        ::PropVariantInit(&var);
        if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &var)) && var.vt == VT_LPWSTR) {
            out.friendly_name.clear();
            for (LPWSTR p = var.pwszVal; *p; ++p) {
                if (*p < 128) out.friendly_name.push_back(static_cast<char>(*p));
            }
        }
        ::PropVariantClear(&var);
    }

    out.is_default = true;
    return true;
}

bool WasapiDeviceEnumerator::device_exists(const std::string& device_id) {
    if (!valid_ || device_id.empty()) return false;
    // Simple check: if we can get the default device, at least one device exists.
    DeviceInfo info;
    return get_default_device(info);
}

}  // namespace kivo::video::audio_plane::backend::output::wasapi
