// ffmpeg_packet_guard.hpp — P6C internal: RAII guard for AVPacket
// INTERNAL header: includes FFmpeg headers (allowed under backend/src/.../internal/).
// avformat/demux is FORBIDDEN — avcodec layer only.
#pragma once

#include <cstdint>
#include <cstring>

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavcodec/packet.h>

namespace kivo::video::audio_plane::backend::decode::ffmpeg::internal {

// RAII guard for AVPacket.
//
// Allocates the packet in the constructor via av_packet_alloc, and frees it in
// the destructor via av_packet_free. Move-only: copy is deleted to prevent
// double-free of the underlying FFmpeg resource.
//
// set_data() copies the supplied byte range into the packet's own buffer so
// the caller's source buffer does not need to outlive the packet. reset()
// unreferences any referenced data while keeping the AVPacket allocation.
class AVPacketGuard {
public:
    AVPacketGuard() noexcept : pkt_(av_packet_alloc()) {}

    // Release the packet if still owned.
    ~AVPacketGuard() {
        if (pkt_ != nullptr) {
            AVPacket* tmp = pkt_;
            pkt_ = nullptr;
            av_packet_free(&tmp);
        }
    }

    // Move constructor: steals the packet, leaves the source empty.
    AVPacketGuard(AVPacketGuard&& other) noexcept : pkt_(other.pkt_) {
        other.pkt_ = nullptr;
    }

    // Move assignment: releases any currently-held packet, then steals the
    // source's packet.
    AVPacketGuard& operator=(AVPacketGuard&& other) noexcept {
        if (this != &other) {
            release();
            pkt_ = other.pkt_;
            other.pkt_ = nullptr;
        }
        return *this;
    }

    AVPacketGuard(const AVPacketGuard&) = delete;
    AVPacketGuard& operator=(const AVPacketGuard&) = delete;

    // Raw pointer access for internal FFmpeg calls. Never nullifies ownership.
    AVPacket* get() noexcept { return pkt_; }
    const AVPacket* get() const noexcept { return pkt_; }

    // True when the packet was successfully allocated and is still owned.
    bool is_valid() const noexcept { return pkt_ != nullptr; }

    // Copy `size` bytes from `data` into the packet's own buffer. Any
    // previously-referenced data is released first. A null `data` with a
    // positive `size` is treated as a no-op; size <= 0 unreferences the
    // packet without copying.
    void set_data(const uint8_t* data, int size) noexcept {
        if (pkt_ == nullptr) {
            return;
        }
        // Always start from a clean packet.
        av_packet_unref(pkt_);
        if (data == nullptr || size <= 0) {
            return;
        }
        // Allocate a writable buffer owned by the packet and copy the bytes.
        // av_packet_alloc leaves buf as null; we use av_new_packet to give the
        // packet its own backing buffer of the requested size.
        if (av_new_packet(pkt_, size) < 0) {
            // Allocation failed: leave the packet unreference'd.
            av_packet_unref(pkt_);
            return;
        }
        std::memcpy(pkt_->data, data, static_cast<size_t>(size));
        pkt_->size = size;
    }

    // Unreference any data referenced by the packet, but keep the AVPacket
    // allocation itself. The guard remains valid and reusable.
    void reset() noexcept {
        if (pkt_ != nullptr) {
            av_packet_unref(pkt_);
        }
    }

private:
    void release() noexcept {
        if (pkt_ != nullptr) {
            AVPacket* tmp = pkt_;
            pkt_ = nullptr;
            av_packet_free(&tmp);
        }
    }

    AVPacket* pkt_{nullptr};
};

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg::internal
