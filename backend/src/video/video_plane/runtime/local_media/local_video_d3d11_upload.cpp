#include "video/video_plane/runtime/local_media/local_video_d3d11_upload.hpp"

#include <chrono>
#include <cstdlib>
#include <utility>

#include "kivo/cinema_engine/video_upload/d3d11_device_context.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_texture_upload.hpp"
#include "kivo/cinema_engine/video_upload/yuv_rgb_conversion.hpp"

namespace kivo::video::video_plane::runtime::local_media {
namespace {

[[nodiscard]] std::uint64_t elapsed_ms_since(
    const std::chrono::steady_clock::time_point anchor) noexcept {
    const auto elapsed = std::chrono::steady_clock::now() - anchor;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}

[[nodiscard]] std::string upload_pixel_format(const std::string& pixel_format) {
    if (pixel_format == "rgba") {
        return "rgba32";
    }
    if (pixel_format == "bgra") {
        return "bgra32";
    }
    return pixel_format;
}

[[nodiscard]] bool upload_enabled() noexcept {
    const char* const value = std::getenv("KIVO_P5_D3D11_UPLOAD");
    return value != nullptr && std::string(value) == "1";
}

}  // namespace

struct LocalVideoD3d11Upload::Impl {
    kivo::cinema_engine::D3d11DeviceContext device;
    kivo::cinema_engine::D3d11TextureUpload uploader;
    kivo::cinema_engine::D3D11TextureHandle texture{nullptr};
};

LocalVideoD3d11Upload::LocalVideoD3d11Upload()
    : impl_(std::make_unique<Impl>()) {}

LocalVideoD3d11Upload::~LocalVideoD3d11Upload() {
    close();
}

bool LocalVideoD3d11Upload::ensure_initialized() noexcept {
    if (stats_.initialized && impl_->uploader.is_ready()) {
        return true;
    }
    if (!impl_->device.is_ready() && !impl_->device.initialize()) {
        stats_.available = false;
        stats_.initialized = false;
        stats_.error = "d3d11 device unavailable";
        return false;
    }
    if (!impl_->uploader.initialize(&impl_->device)) {
        stats_.available = false;
        stats_.initialized = false;
        stats_.error = "d3d11 texture uploader unavailable";
        return false;
    }
    stats_.available = true;
    stats_.initialized = true;
    stats_.device_type = impl_->device.device_type();
    return true;
}

bool LocalVideoD3d11Upload::ensure_texture(
    const kivo::cinema_engine::DecodedFrame& frame) noexcept {
    const std::string pixel_format = upload_pixel_format(frame.pixel_format);
    if (impl_->texture != nullptr && texture_width_ == frame.width &&
        texture_height_ == frame.height && texture_pixel_format_ == pixel_format) {
        return true;
    }

    reset_texture();
    impl_->texture = impl_->uploader.create_texture(frame.width, frame.height, pixel_format);
    if (impl_->texture == nullptr) {
        stats_.error = "d3d11 texture creation failed";
        return false;
    }
    texture_width_ = frame.width;
    texture_height_ = frame.height;
    texture_pixel_format_ = pixel_format;
    return true;
}

bool LocalVideoD3d11Upload::upload(
    const kivo::cinema_engine::DecodedFrame& frame) noexcept {
    const auto upload_start = std::chrono::steady_clock::now();
    if (!upload_enabled()) {
        stats_.available = false;
        stats_.initialized = false;
        stats_.uploaded = false;
        stats_.error = "d3d11 upload disabled";
        return false;
    }
    if (frame.width <= 0 || frame.height <= 0 || frame.frame_data.empty()) {
        ++stats_.upload_failures;
        stats_.error = "decoded frame is not uploadable";
        return false;
    }
    if (!ensure_initialized() || !ensure_texture(frame)) {
        ++stats_.upload_failures;
        return false;
    }

    const kivo::cinema_engine::YuvRgbConversion conversion{};
    if (!impl_->uploader.upload_frame(impl_->texture, frame, conversion)) {
        ++stats_.upload_failures;
        stats_.error = "d3d11 frame upload failed";
        return false;
    }

    const auto elapsed = elapsed_ms_since(upload_start);
    ++stats_.frames_uploaded;
    stats_.uploaded = true;
    stats_.last_upload_elapsed_ms = elapsed;
    if (elapsed > stats_.max_upload_elapsed_ms) {
        stats_.max_upload_elapsed_ms = elapsed;
    }
    stats_.error.clear();
    return true;
}

void LocalVideoD3d11Upload::reset_texture() noexcept {
    if (impl_ == nullptr || impl_->texture == nullptr) {
        texture_width_ = 0;
        texture_height_ = 0;
        texture_pixel_format_.clear();
        return;
    }
    impl_->uploader.release_texture(impl_->texture);
    impl_->texture = nullptr;
    texture_width_ = 0;
    texture_height_ = 0;
    texture_pixel_format_.clear();
}

void LocalVideoD3d11Upload::close() noexcept {
    if (impl_ != nullptr) {
        reset_texture();
        impl_->uploader.release();
        impl_->device.release();
    }
    stats_ = {};
}

}  // namespace kivo::video::video_plane::runtime::local_media
