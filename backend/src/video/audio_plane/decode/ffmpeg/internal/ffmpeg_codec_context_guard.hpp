// ffmpeg_codec_context_guard.hpp — P6C internal: RAII guard for AVCodecContext
// INTERNAL header: includes FFmpeg headers (allowed under backend/src/.../internal/).
// avformat/demux is FORBIDDEN — avcodec layer only.
#pragma once

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavcodec/packet.h>

namespace kivo::video::audio_plane::backend::decode::ffmpeg::internal {

// RAII guard for AVCodecContext.
//
// Acquires the context in the constructor via avcodec_alloc_context3(codec),
// and releases it in the destructor via avcodec_free_context. Move-only:
// copy is deleted to prevent double-free of the underlying FFmpeg resource.
//
// The raw pointer is exposed via get() for internal use by sibling P6C
// translation units. Callers must NOT retain the raw pointer beyond the
// lifetime of this guard.
class AVCodecContextGuard {
public:
    // Default constructor: no context owned. Use take() to assume ownership
    // of an already-allocated context.
    AVCodecContextGuard() noexcept = default;

    // Allocate a new AVCodecContext for the given decoder codec.
    // If `codec` is null, avcodec_alloc_context3 returns null and is_valid()
    // will report false. The guard is still safe to destroy.
    explicit AVCodecContextGuard(const AVCodec* codec) noexcept
        : ctx_(avcodec_alloc_context3(codec)) {}

    // Assume ownership of an already-allocated AVCodecContext. Releases any
    // currently-held context first.
    void take(AVCodecContext* ctx) noexcept {
        release();
        ctx_ = ctx;
    }

    // Release the context if still owned.
    ~AVCodecContextGuard() {
        if (ctx_ != nullptr) {
            AVCodecContext* tmp = ctx_;
            ctx_ = nullptr;
            avcodec_free_context(&tmp);
        }
    }

    // Move constructor: steals the context, leaves the source empty.
    AVCodecContextGuard(AVCodecContextGuard&& other) noexcept
        : ctx_(other.ctx_) {
        other.ctx_ = nullptr;
    }

    // Move assignment: releases any currently-held context, then steals
    // the source's context.
    AVCodecContextGuard& operator=(AVCodecContextGuard&& other) noexcept {
        if (this != &other) {
            release();
            ctx_ = other.ctx_;
            other.ctx_ = nullptr;
        }
        return *this;
    }

    AVCodecContextGuard(const AVCodecContextGuard&) = delete;
    AVCodecContextGuard& operator=(const AVCodecContextGuard&) = delete;

    // Raw pointer access for internal FFmpeg calls. Never nullifies ownership.
    AVCodecContext* get() noexcept { return ctx_; }
    const AVCodecContext* get() const noexcept { return ctx_; }

    // True when the context was successfully allocated and is still owned.
    bool is_valid() const noexcept { return ctx_ != nullptr; }

    // Release the currently-held context if any. The guard becomes empty.
    void reset() noexcept { release(); }

private:
    void release() noexcept {
        if (ctx_ != nullptr) {
            AVCodecContext* tmp = ctx_;
            ctx_ = nullptr;
            avcodec_free_context(&tmp);
        }
    }

    AVCodecContext* ctx_{nullptr};
};

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg::internal
