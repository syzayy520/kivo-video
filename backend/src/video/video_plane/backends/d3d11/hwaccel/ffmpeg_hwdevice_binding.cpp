#include "video/video_plane/backends/d3d11/hwaccel/ffmpeg_hwdevice_binding.hpp"

// Must precede the FFmpeg C block: libavutil/hwcontext_d3d11va.h includes
// <d3d11.h> itself, and D3D11 declares C++ operator overloads that cannot live
// inside extern "C".
#include <d3d11.h>

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_d3d11va.h>
}

namespace kivo::video::video_plane::backends::d3d11::hwaccel {

FfmpegHwDeviceBinding::~FfmpegHwDeviceBinding() { release(); }

auto FfmpegHwDeviceBinding::bind(ID3D11Device* device) noexcept -> bool {
    release();

    if (device == nullptr) {
        last_error_ = "null d3d11 device";
        return false;
    }

    ref_ = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_D3D11VA);
    if (ref_ == nullptr) {
        last_error_ = "av_hwdevice_ctx_alloc failed";
        return false;
    }

    auto* device_ctx = reinterpret_cast<AVHWDeviceContext*>(ref_->data);
    auto* d3d11_ctx = reinterpret_cast<AVD3D11VADeviceContext*>(device_ctx->hwctx);

    // FFmpeg's d3d11va teardown releases this device pointer, so the reference
    // added here is the one FFmpeg will drop. The caller keeps its own.
    device->AddRef();
    d3d11_ctx->device = device;

    if (av_hwdevice_ctx_init(ref_) < 0) {
        last_error_ = "av_hwdevice_ctx_init failed";
        release();
        return false;
    }

    last_error_ = nullptr;
    return true;
}

void FfmpegHwDeviceBinding::release() noexcept {
    if (ref_ != nullptr) {
        av_buffer_unref(&ref_);
        ref_ = nullptr;
    }
}

}  // namespace kivo::video::video_plane::backends::d3d11::hwaccel
