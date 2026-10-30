#pragma once

extern "C" {
#include <libavutil/buffer.h>
}

struct ID3D11Device;

namespace kivo::video::video_plane::backends::d3d11::hwaccel {

// Binds an existing D3D11 device to an FFmpeg AVHWDeviceContext of type D3D11VA.
// Single responsibility: own that binding and hand out the buffer reference.
// It does not create the D3D11 device and does not decode.
class FfmpegHwDeviceBinding {
public:
    FfmpegHwDeviceBinding() = default;
    ~FfmpegHwDeviceBinding();

    FfmpegHwDeviceBinding(const FfmpegHwDeviceBinding&) = delete;
    auto operator=(const FfmpegHwDeviceBinding&) -> FfmpegHwDeviceBinding& = delete;

    // Create and initialise the hardware device context around `device`.
    // `device` must stay alive for as long as the binding is used. The binding
    // takes its own reference; FFmpeg releases that reference on teardown.
    [[nodiscard]] auto bind(ID3D11Device* device) noexcept -> bool;

    void release() noexcept;

    [[nodiscard]] auto is_bound() const noexcept -> bool { return ref_ != nullptr; }
    [[nodiscard]] auto ref() const noexcept -> AVBufferRef* { return ref_; }
    [[nodiscard]] auto last_error() const noexcept -> const char* { return last_error_; }

private:
    AVBufferRef* ref_{nullptr};
    const char* last_error_{nullptr};
};

}  // namespace kivo::video::video_plane::backends::d3d11::hwaccel
