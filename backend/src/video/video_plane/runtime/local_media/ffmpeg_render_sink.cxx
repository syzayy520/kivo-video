#include "video/video_plane/runtime/local_media/ffmpeg_render_sink.hpp"

#include <cstring>

namespace kivo::video::video_plane::runtime::local_media {

FfmpegWindowSink::FfmpegWindowSink(ANativeWindow* window) : window_(window) {
    if (window_ != nullptr) {
        ANativeWindow_acquire(window_);
    }
}

FfmpegWindowSink::~FfmpegWindowSink() {
    if (window_ != nullptr) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }
}

bool FfmpegWindowSink::ensure_geometry(int width, int height, std::string& error) {
    if (configured_width_ == width && configured_height_ == height) {
        return true;
    }
    const std::int32_t status = ANativeWindow_setBuffersGeometry(
        window_, width, height, WINDOW_FORMAT_RGBA_8888);
    if (status != 0) {
        error = "ANativeWindow_setBuffersGeometry(" + std::to_string(width) + "x" +
                std::to_string(height) + ") failed with " + std::to_string(status);
        return false;
    }
    configured_width_ = width;
    configured_height_ = height;
    return true;
}

bool FfmpegWindowSink::render(const kivo::cinema_engine::DecodedFrame& frame,
                              std::string& error) {
    if (window_ == nullptr) {
        error = "window is null";
        return false;
    }
    // 只吃 rgba：decode_video_rgba 的输出保证 4 字节/像素打包格式。
    // 其他 pixel_format 到这里说明上游接错了线，如实报错而不是猜布局。
    if (frame.pixel_format != "rgba") {
        error = "sink expects rgba frames, got " + frame.pixel_format;
        return false;
    }
    if (!ensure_geometry(frame.width, frame.height, error)) {
        return false;
    }

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window_, &buffer, nullptr) != 0) {
        error = "ANativeWindow_lock failed";
        return false;
    }
    // 按行拷：窗口 stride 与帧 linesize 都按字节算，逐行搬运，
    // 宽度不一致（缩放交给 geometry）或 stride 不齐都在这里兜住。
    const std::uint8_t* src = frame.frame_data.data();
    const std::int32_t src_stride =
        frame.linesize[0] > 0 ? frame.linesize[0] : frame.width * 4;
    auto* dst = static_cast<std::uint8_t*>(buffer.bits);
    const std::int32_t dst_stride = buffer.stride * 4;
    const std::size_t copy_bytes =
        static_cast<std::size_t>(frame.width) * 4 < static_cast<std::size_t>(buffer.width) * 4
            ? static_cast<std::size_t>(frame.width) * 4
            : static_cast<std::size_t>(buffer.width) * 4;
    const int rows = frame.height < buffer.height ? frame.height : buffer.height;
    for (int row = 0; row < rows; ++row) {
        std::memcpy(dst + static_cast<std::size_t>(row) * dst_stride,
                    src + static_cast<std::size_t>(row) * src_stride, copy_bytes);
    }
    ANativeWindow_unlockAndPost(window_);
    ++rendered_;
    return true;
}

}  // namespace kivo::video::video_plane::runtime::local_media
