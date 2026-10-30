#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

// FFmpeg public headers carry no extern "C" guard in this SDK; the project
// convention is to wrap them explicitly (see audio_plane/decode/ffmpeg).
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/rational.h>
}

#include "kivo/cinema_engine/video_plane_gpu/decode/hardware_decode_backend.hpp"
#include "video/video_plane/backends/d3d11/hwaccel/ffmpeg_hwdevice_binding.hpp"

namespace kivo::video::video_plane::backends::d3d11::hwaccel {

// D3D11VA hardware video decoder.
// Single responsibility: turn demuxed packets into GPU-resident frames.
// It never copies pixels to CPU memory and never falls back to software decode.
class FfmpegD3d11vaDecoder final
    : public kivo::cinema_engine::video_plane_gpu::HardwareDecodeBackend {
public:
    FfmpegD3d11vaDecoder() = default;
    ~FfmpegD3d11vaDecoder() override;

    FfmpegD3d11vaDecoder(const FfmpegD3d11vaDecoder&) = delete;
    auto operator=(const FfmpegD3d11vaDecoder&) -> FfmpegD3d11vaDecoder& = delete;

    [[nodiscard]] auto configure(
        const kivo::cinema_engine::video_plane_gpu::HardwareDecodeConfig& config,
        kivo::cinema_engine::video_plane_gpu::GpuDeviceHandle device_handle) -> bool override;

    [[nodiscard]] auto capability() const
        -> kivo::cinema_engine::video_plane_gpu::GpuDecodeCapability override;

    [[nodiscard]] auto submit_packet(const std::uint8_t* data,
                                     std::size_t size,
                                     std::int64_t pts,
                                     std::int64_t dts,
                                     std::int64_t time_base_num,
                                     std::int64_t time_base_den)
        -> kivo::cinema_engine::video_plane_gpu::SubmitOutcome override;

    [[nodiscard]] auto submit_end_of_stream()
        -> kivo::cinema_engine::video_plane_gpu::SubmitOutcome override;

    // Drain one more frame without submitting input. A single packet can yield
    // zero, one or several frames, so a correct loop must drain until
    // NeedsMoreInput; otherwise frame accounting is silently wrong.
    [[nodiscard]] auto receive_frame()
        -> kivo::cinema_engine::video_plane_gpu::DecodeOutcome override;

    void release_frame(std::uint64_t frame_token) noexcept override;

    void reset(std::int64_t seek_pts_us) override;

    [[nodiscard]] auto last_error() const noexcept -> const std::string& { return last_error_; }

private:
    [[nodiscard]] auto receive_into_outcome()
        -> kivo::cinema_engine::video_plane_gpu::DecodeOutcome;
    [[nodiscard]] auto make_frame_contract(std::size_t slot_index)
        -> kivo::cinema_engine::video_plane_gpu::GpuFrameContract;

    void release_decoder() noexcept;
    void release_live_frames() noexcept;
    [[nodiscard]] auto acquire_staging(std::size_t required) -> AVBufferRef*;

    FfmpegHwDeviceBinding binding_;
    AVCodecContext* codec_ctx_{nullptr};
    AVRational time_base_{0, 1};
    std::string codec_name_;
    std::string last_error_;

    // Stable packet staging area.
    // A single shared buffer is NOT safe: after submit the decoder still holds a
    // reference to the previous packet, so overwriting it corrupts data that has
    // not been decoded yet (observed as AVERROR_INVALIDDATA mid-stream). Several
    // slots are kept and only a writable one is reused, which keeps the hot path
    // allocation-free while remaining correct.
    struct StagingSlot {
        AVBufferRef* buffer{nullptr};
        std::size_t capacity{0};
    };
    static constexpr std::size_t kStagingSlots = 24;
    std::array<StagingSlot, kStagingSlots> staging_{};
    AVPacket* scratch_packet_{nullptr};

    // Fixed-capacity table of live frames.
    // The hash map it replaces allocated a node per frame and cost a lookup on
    // every release; measured CPU was 11.3% of software instead of the 10% goal.
    // A slab keeps the AVFrame objects alive across the stream, so the steady
    // state performs no allocation at all and release is an index write.
    // The token is (slot index + 1) so that 0 keeps meaning "invalid".
    struct LiveFrameSlot {
        AVFrame* frame{nullptr};
        bool in_use{false};
    };
    static constexpr std::size_t kMaxLiveFrames = 64;
    std::array<LiveFrameSlot, kMaxLiveFrames> live_frames_{};

    [[nodiscard]] auto acquire_live_slot() noexcept -> std::size_t;
    // Set once a P010 surface is observed: this is the only truthful evidence
    // that ten-bit decode actually happened on this GPU.
    bool saw_ten_bit_{false};
};

}  // namespace kivo::video::video_plane::backends::d3d11::hwaccel
