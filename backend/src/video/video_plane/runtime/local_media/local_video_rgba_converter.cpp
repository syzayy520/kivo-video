#include "video/video_plane/runtime/local_media/local_video_rgba_converter.hpp"

#ifdef KIVO_ENABLE_FFMPEG
#include "kivo/cinema_engine/decode_core/decoded_frame.hpp"

extern "C" {
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}
#endif

namespace kivo::video::video_plane::runtime::local_media {

#ifndef KIVO_ENABLE_FFMPEG

RgbaFrameBuffer convert_decoded_frame_to_rgba8888(
    const kivo::cinema_engine::DecodedFrame& frame) noexcept {
    (void)frame;
    RgbaFrameBuffer buffer{};
    buffer.error = "ffmpeg disabled";
    return buffer;
}

#else

namespace {

struct ThreadLocalSwsCache {
    SwsContext* context{nullptr};

    ~ThreadLocalSwsCache() {
        if (context != nullptr) {
            sws_freeContext(context);
            context = nullptr;
        }
    }

    [[nodiscard]] SwsContext* get(
        const int width,
        const int height,
        const int source_format) noexcept {
        context = sws_getCachedContext(
            context,
            width,
            height,
            static_cast<AVPixelFormat>(source_format),
            width,
            height,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr);
        return context;
    }
};

[[nodiscard]] int pixel_format_to_av(const std::string& pixel_format) noexcept {
    if (pixel_format == "yuv420p") {
        return AV_PIX_FMT_YUV420P;
    }
    if (pixel_format == "yuv420p10le") {
        return AV_PIX_FMT_YUV420P10LE;
    }
    if (pixel_format == "nv12") {
        return AV_PIX_FMT_NV12;
    }
    return AV_PIX_FMT_NONE;
}

[[nodiscard]] const std::uint8_t* plane_ptr(
    const std::vector<std::uint8_t>& data,
    const std::size_t offset) noexcept {
    return offset < data.size() ? data.data() + offset : nullptr;
}

}  // namespace

RgbaFrameBuffer convert_decoded_frame_to_rgba8888(
    const kivo::cinema_engine::DecodedFrame& frame) noexcept {
    RgbaFrameBuffer buffer{};
    if (!convert_decoded_frame_to_rgba8888_into(frame, buffer.rgba8888, buffer.error)) {
        return buffer;
    }

    buffer.valid = true;
    buffer.width = frame.width;
    buffer.height = frame.height;
    return buffer;
}

bool convert_decoded_frame_to_rgba8888_into(
    const kivo::cinema_engine::DecodedFrame& frame,
    std::vector<std::uint8_t>& rgba8888,
    std::string& error) noexcept {
    error.clear();
    if (frame.width <= 0 || frame.height <= 0 || frame.frame_data.empty()) {
        error = "empty decoded frame";
        return false;
    }

    const int source_format = pixel_format_to_av(frame.pixel_format);
    if (source_format == AV_PIX_FMT_NONE) {
        error = "unsupported pixel format: " + frame.pixel_format;
        return false;
    }

    const int y_stride = frame.linesize[0] > 0 ? frame.linesize[0] : frame.width;
    const int u_stride = frame.linesize[1] > 0 ? frame.linesize[1] : frame.width / 2;
    const int v_stride = frame.linesize[2] > 0 ? frame.linesize[2] : frame.width / 2;
    const std::size_t y_bytes =
        static_cast<std::size_t>(frame.height) * static_cast<std::size_t>(y_stride);
    const std::size_t u_bytes =
        static_cast<std::size_t>((frame.height + 1) / 2) * static_cast<std::size_t>(u_stride);

    const std::uint8_t* planes[4] = {
        plane_ptr(frame.frame_data, 0),
        plane_ptr(frame.frame_data, y_bytes),
        plane_ptr(frame.frame_data, y_bytes + u_bytes),
        nullptr,
    };
    const int strides[4] = {y_stride, u_stride, v_stride, 0};

    thread_local ThreadLocalSwsCache sws_cache;
    SwsContext* const context = sws_cache.get(frame.width, frame.height, source_format);
    if (context == nullptr) {
        error = "swscale context failed";
        return false;
    }

    rgba8888.resize(static_cast<std::size_t>(frame.width) * static_cast<std::size_t>(frame.height) * 4);
    std::uint8_t* dst_planes[4] = {rgba8888.data(), nullptr, nullptr, nullptr};
    const int dst_strides[4] = {frame.width * 4, 0, 0, 0};
    const int scaled_rows = sws_scale(
        context,
        planes,
        strides,
        0,
        frame.height,
        dst_planes,
        dst_strides);

    if (scaled_rows <= 0) {
        error = "swscale failed";
        rgba8888.clear();
        return false;
    }

    return true;
}

#endif

}  // namespace kivo::video::video_plane::runtime::local_media
