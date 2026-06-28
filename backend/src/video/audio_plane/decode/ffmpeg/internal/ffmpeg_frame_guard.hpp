// ffmpeg_frame_guard.hpp — P6C internal: RAII guard for AVFrame
// INTERNAL header: includes FFmpeg headers (allowed under backend/src/.../internal/).
// avformat/demux is FORBIDDEN — avcodec layer only.
#pragma once

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavcodec/packet.h>

namespace kivo::video::audio_plane::backend::decode::ffmpeg::internal {

// RAII guard for AVFrame.
//
// Allocates the frame in the constructor via av_frame_alloc, and frees it in
// the destructor via av_frame_free. Move-only: copy is deleted to prevent
// double-free of the underlying FFmpeg resource.
//
// reset() unreferences any currently-referenced data buffers while keeping
// the AVFrame allocation itself alive — the guard remains valid after reset()
// and can be reused for the next decode call.
class AVFrameGuard {
public:
    AVFrameGuard() noexcept : frame_(av_frame_alloc()) {}

    // Release the frame if still owned.
    ~AVFrameGuard() {
        if (frame_ != nullptr) {
            AVFrame* tmp = frame_;
            frame_ = nullptr;
            av_frame_free(&tmp);
        }
    }

    // Move constructor: steals the frame, leaves the source empty.
    AVFrameGuard(AVFrameGuard&& other) noexcept : frame_(other.frame_) {
        other.frame_ = nullptr;
    }

    // Move assignment: releases any currently-held frame, then steals the
    // source's frame.
    AVFrameGuard& operator=(AVFrameGuard&& other) noexcept {
        if (this != &other) {
            release();
            frame_ = other.frame_;
            other.frame_ = nullptr;
        }
        return *this;
    }

    AVFrameGuard(const AVFrameGuard&) = delete;
    AVFrameGuard& operator=(const AVFrameGuard&) = delete;

    // Raw pointer access for internal FFmpeg calls. Never nullifies ownership.
    AVFrame* get() noexcept { return frame_; }
    const AVFrame* get() const noexcept { return frame_; }

    // True when the frame was successfully allocated and is still owned.
    bool is_valid() const noexcept { return frame_ != nullptr; }

    // Unreference any data buffers referenced by the frame, but keep the
    // AVFrame allocation itself. The guard remains valid and reusable.
    void reset() noexcept {
        if (frame_ != nullptr) {
            av_frame_unref(frame_);
        }
    }

private:
    void release() noexcept {
        if (frame_ != nullptr) {
            AVFrame* tmp = frame_;
            frame_ = nullptr;
            av_frame_free(&tmp);
        }
    }

    AVFrame* frame_{nullptr};
};

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg::internal
