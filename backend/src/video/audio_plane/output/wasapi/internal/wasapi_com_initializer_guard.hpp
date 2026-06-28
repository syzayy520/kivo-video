// wasapi_com_initializer_guard.hpp — P6D internal: RAII guard for COM initialization
// INTERNAL header: includes Windows headers (allowed under backend/src/.../internal/).
#pragma once

#include <objbase.h>

namespace kivo::video::audio_plane::backend::output::wasapi::internal {

// RAII guard for CoInitializeEx / CoUninitialize.
// COM init is per-thread. Each thread that uses WASAPI must initialize COM.
// This guard ensures CoUninitialize is always called on thread exit.
class ComInitializerGuard {
public:
    explicit ComInitializerGuard(DWORD dwCoInit = COINIT_MULTITHREADED) noexcept
        : hr_(::CoInitializeEx(nullptr, dwCoInit)) {}

    ~ComInitializerGuard() {
        if (SUCCEEDED(hr_)) {
            ::CoUninitialize();
        }
    }

    ComInitializerGuard(const ComInitializerGuard&) = delete;
    ComInitializerGuard& operator=(const ComInitializerGuard&) = delete;
    ComInitializerGuard(ComInitializerGuard&&) = delete;
    ComInitializerGuard& operator=(ComInitializerGuard&&) = delete;

    bool is_valid() const noexcept { return SUCCEEDED(hr_); }
    HRESULT result() const noexcept { return hr_; }

private:
    HRESULT hr_;
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi::internal
