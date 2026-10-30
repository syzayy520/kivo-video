#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "kivo/cinema_engine/video_plane_gpu/device/gpu_decode_capability.hpp"
#include "kivo/cinema_engine/video_plane_gpu/device/gpu_device_contract.hpp"
#include "kivo/cinema_engine/video_plane_gpu/frame/gpu_frame_contract.hpp"
#include "kivo/cinema_engine/video_plane_gpu/surface/gpu_surface_contract.hpp"

namespace kivo::cinema_engine::video_plane_gpu {

// Configuration for one hardware decode session.
struct HardwareDecodeConfig {
    std::string codec_name;              // FFmpeg codec name, e.g. "hevc"
    std::int32_t stream_index{0};
    std::int32_t coded_width{0};
    std::int32_t coded_height{0};
    GpuPixelFormat preferred_format{GpuPixelFormat::P010};
    // Extra decoder threads. Zero means let the decoder decide.
    std::int32_t thread_count{0};
    // Codec extradata from the container (HEVC hvcC, H.264 avcC, ...).
    // MP4/MOV cannot be decoded without it: the parameter sets live here, not in
    // the bitstream. The backend copies it, so the caller keeps ownership.
    const std::uint8_t* extradata{nullptr};
    std::size_t extradata_size{0};
};

// Outcome of a packet submission.
// Submit is separated from receive because every real hardware decoder
// (D3D11VA, MediaCodec, VideoToolbox) works that way: submission can be
// temporarily refused while the decoder still holds frames, and a caller that
// treats that refusal as "done" silently drops packets and corrupts the
// reference chain.
enum class SubmitStatus : std::uint8_t {
    Accepted = 0,        // the packet was consumed
    NeedsDrain,          // refused: receive frames first, then submit again
    EndOfStream,         // no more input will be accepted
    HardwareUnavailable, // no usable hardware path
    Error,
};

struct SubmitOutcome {
    SubmitStatus status{SubmitStatus::Error};
    std::string error;
};

// Outcome of a single decode step.
enum class DecodeStatus : std::uint8_t {
    Frame = 0,          // a GPU frame is available
    NeedsMoreInput,     // no frame ready; decoder wants more packets
    EndOfStream,        // decoder drained
    HardwareUnavailable,// no usable hardware path; caller must decide on fallback
    Error,
};

struct DecodeOutcome {
    DecodeStatus status{DecodeStatus::Error};
    GpuFrameContract frame{};
    std::string error;
};

// Abstract hardware video decode backend.
// The concrete implementation is FFmpeg/D3D11VA today; nothing in this header
// mentions FFmpeg, D3D11 or DXGI.
class HardwareDecodeBackend {
public:
    virtual ~HardwareDecodeBackend() = default;

    // Bind to a decoder configuration and an already-created GPU device.
    // device_handle must come from the same adapter that produced the capability report.
    [[nodiscard]] virtual auto configure(const HardwareDecodeConfig& config,
                                         GpuDeviceHandle device_handle) -> bool = 0;

    // Report what this backend instance actually negotiated.
    [[nodiscard]] virtual auto capability() const -> GpuDecodeCapability = 0;

    // Feed one demuxed packet. `data` is the raw packet payload, `pts`/`dts` are in
    // stream time base units, `time_base_num`/`den` describe that base.
    // When the result is NeedsDrain the packet was NOT consumed: the caller must
    // drain with receive_frame() and submit the same packet again. Dropping it
    // instead produces "could not find reference" errors a few frames later.
    [[nodiscard]] virtual auto submit_packet(const std::uint8_t* data,
                                             std::size_t size,
                                             std::int64_t pts,
                                             std::int64_t dts,
                                             std::int64_t time_base_num,
                                             std::int64_t time_base_den) -> SubmitOutcome = 0;

    // Signal that no further packets will arrive, so remaining frames can drain.
    [[nodiscard]] virtual auto submit_end_of_stream() -> SubmitOutcome = 0;

    // Pull one already-decoded frame without submitting new input.
    // A single packet legitimately yields zero, one or several frames (B-frame
    // reordering), so a correct decode loop must drain with this call until it
    // reports NeedsMoreInput. Without it, frame accounting is silently wrong.
    [[nodiscard]] virtual auto receive_frame() -> DecodeOutcome = 0;

    // Return a frame token obtained from DecodeOutcome::frame.
    virtual void release_frame(std::uint64_t frame_token) noexcept = 0;

    // Drop all buffered state and prepare for a seek to the given timestamp.
    virtual void reset(std::int64_t seek_pts_us) = 0;
};

}  // namespace kivo::cinema_engine::video_plane_gpu
