#include "video/video_plane/probe/decode_probe.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "kivo/cinema_engine/video_plane_gpu/decode/hardware_decode_backend.hpp"
#include "video/video_plane/backends/d3d11/device/d3d11_video_device.hpp"
#include "video/video_plane/backends/d3d11/hwaccel/ffmpeg_d3d11va_decoder.hpp"
#include "video/video_plane/probe/media_source.hpp"
#include "video/video_plane/probe/probe_clock.hpp"

namespace kivo::video::video_plane::probe {
namespace {

namespace gpu = kivo::cinema_engine::video_plane_gpu;
namespace device_ns = kivo::video::video_plane::backends::d3d11::device;
namespace hwaccel_ns = kivo::video::video_plane::backends::d3d11::hwaccel;

[[nodiscard]] auto format_name(const gpu::GpuPixelFormat format) -> std::string {
    switch (format) {
        case gpu::GpuPixelFormat::Nv12: return "NV12";
        case gpu::GpuPixelFormat::P010: return "P010";
        case gpu::GpuPixelFormat::Rgba8: return "RGBA8";
        default: return "unknown";
    }
}

}  // namespace

auto HardwareDecodeProbe::run() -> ProbeResult {
    ProbeResult result{};
    DecodeMetrics& metrics = result.metrics;
    metrics.input_path = options_.input_path;
    metrics.backend = "d3d11va";

    MediaSource source;
    std::string error;
    if (!source.open(options_.input_path, error)) {
        metrics.failure_reason = error;
        result.error = std::move(error);
        return result;
    }

    const VideoStreamInfo& info = source.video_stream();
    metrics.codec = info.codec_name;
    metrics.width = info.width;
    metrics.height = info.height;
    metrics.bit_depth = info.bit_depth;

    if (info.time_base_num <= 0 || info.time_base_den <= 0) {
        metrics.failure_reason = "stream has an invalid time base";
        result.error = metrics.failure_reason;
        return result;
    }

    device_ns::D3d11VideoDevice device;
    if (!device.initialize()) {
        metrics.failure_reason = "no hardware video device available";
        result.error = metrics.failure_reason;
        return result;
    }
    metrics.device_name = device.contract().adapter.name;

    hwaccel_ns::FfmpegD3d11vaDecoder decoder;
    gpu::HardwareDecodeConfig config{};
    config.codec_name = info.codec_name;
    config.coded_width = info.width;
    config.coded_height = info.height;
    config.preferred_format = gpu::GpuPixelFormat::P010;
    config.extradata = info.extradata;
    config.extradata_size = info.extradata_size;

    if (!decoder.configure(config, device.device())) {
        metrics.failure_reason = decoder.last_error().empty()
                                     ? "decoder configure failed"
                                     : decoder.last_error();
        result.error = metrics.failure_reason;
        return result;
    }

    std::vector<double> step_ms;
    step_ms.reserve(static_cast<std::size_t>(options_.max_frames) + 16);

    std::int32_t frames = 0;
    std::int32_t packets = 0;
    bool saw_frame = false;
    bool all_frames_hardware = true;

    const double cpu_before = process_cpu_seconds();
    const unsigned long long cycles_before = process_cycle_count();
    const double wall_before = steady_seconds();

    bool source_exhausted = false;
    bool eos_submitted = false;
    bool has_pending_packet = false;
    bool failed = false;
    bool step_active = false;
    double step_start = 0.0;
    int idle_spins = 0;

    while (!failed && frames < options_.max_frames) {
        if (!has_pending_packet && !source_exhausted) {
            const ReadStatus read = source.read_next(error);
            if (read == ReadStatus::Failed) {
                metrics.failure_reason = error;
                result.error = error;
                failed = true;
                break;
            }
            if (read == ReadStatus::EndOfStream) {
                source_exhausted = true;
            } else {
                has_pending_packet = true;
            }
        }

        if (has_pending_packet) {
            const SourcePacketView& packet = source.current_packet();
            const gpu::SubmitOutcome submit = decoder.submit_packet(packet.data,
                                                                   packet.size,
                                                                   packet.pts,
                                                                   packet.dts,
                                                                   info.time_base_num,
                                                                   info.time_base_den);
            switch (submit.status) {
                case gpu::SubmitStatus::Accepted:
                    has_pending_packet = false;
                    ++packets;
                    if (!step_active) {
                        step_active = true;
                        step_start = steady_seconds();
                    }
                    idle_spins = 0;
                    break;
                case gpu::SubmitStatus::NeedsDrain:
                    // The packet was NOT consumed: drain first, then retry it.
                    break;
                case gpu::SubmitStatus::EndOfStream:
                    has_pending_packet = false;
                    source_exhausted = true;
                    break;
                default:
                    metrics.failure_reason =
                        submit.error.empty() ? "packet submission failed" : submit.error;
                    result.error = metrics.failure_reason;
                    failed = true;
                    break;
            }
            if (failed) {
                break;
            }
        } else if (source_exhausted && !eos_submitted) {
            const gpu::SubmitOutcome eos = decoder.submit_end_of_stream();
            if (eos.status == gpu::SubmitStatus::Accepted ||
                eos.status == gpu::SubmitStatus::EndOfStream ||
                eos.status == gpu::SubmitStatus::NeedsDrain) {
                eos_submitted = true;
            } else {
                metrics.failure_reason =
                    eos.error.empty() ? "end of stream rejected" : eos.error;
                result.error = metrics.failure_reason;
                failed = true;
                break;
            }
        }

        gpu::DecodeOutcome outcome = decoder.receive_frame();
        if (outcome.status == gpu::DecodeStatus::Frame) {
            if (!saw_frame) {
                saw_frame = true;
                metrics.pixel_format = format_name(outcome.frame.surface.desc.format);
                metrics.ten_bit = outcome.frame.surface.desc.format == gpu::GpuPixelFormat::P010;
                metrics.width = outcome.frame.surface.desc.width;
                metrics.height = outcome.frame.surface.desc.height;
            }
            if (!outcome.frame.hardware_decoded) {
                all_frames_hardware = false;
            }
            ++frames;
            decoder.release_frame(outcome.frame.frame_token);
            idle_spins = 0;
            continue;
        }

        if (outcome.status == gpu::DecodeStatus::NeedsMoreInput) {
            if (step_active) {
                step_ms.push_back((steady_seconds() - step_start) * 1000.0);
                step_active = false;
            }
            // Stall guard: input is gone, end of stream was accepted, and the
            // decoder still asks for more. That is a defect, not a wait state.
            if (source_exhausted && eos_submitted && ++idle_spins > 1000) {
                metrics.failure_reason = "decoder stalled after end of stream";
                result.error = metrics.failure_reason;
                failed = true;
            }
            continue;
        }

        if (outcome.status == gpu::DecodeStatus::EndOfStream) {
            break;
        }

        metrics.failure_reason =
            outcome.error.empty() ? "decoder reported a hard failure" : outcome.error;
        result.error = metrics.failure_reason;
        failed = true;
    }

    const double wall_after = steady_seconds();
    const double cpu_after = process_cpu_seconds();
    const unsigned long long cycles_after = process_cycle_count();

    metrics.decoded_frames = frames;
    metrics.submitted_packets = packets;
    metrics.elapsed_ms = (wall_after - wall_before) * 1000.0;
    metrics.wall_time_ms = metrics.elapsed_ms;
    metrics.cpu_time_ms = (cpu_after - cpu_before) * 1000.0;
    metrics.cpu_cycles = static_cast<double>(cycles_after - cycles_before);
    metrics.fps = metrics.elapsed_ms > 0.0
                      ? static_cast<double>(frames) * 1000.0 / metrics.elapsed_ms
                      : 0.0;
    metrics.hardware_decoded = saw_frame && all_frames_hardware;

    if (!step_ms.empty()) {
        std::sort(step_ms.begin(), step_ms.end());
        double sum = 0.0;
        for (const double value : step_ms) {
            sum += value;
        }
        metrics.step_ms_avg = sum / static_cast<double>(step_ms.size());
        metrics.step_ms_p50 = percentile(step_ms, 50.0);
        metrics.step_ms_p99 = percentile(step_ms, 99.0);
        metrics.step_ms_max = step_ms.back();
    }

    // A run that produced nothing is a failure even if no error was raised:
    // silently reporting "0 frames, no error" is exactly the false-green pattern
    // this probe exists to prevent.
    if (frames == 0 && metrics.failure_reason.empty()) {
        metrics.failure_reason = "decoder produced no frames";
        result.error = metrics.failure_reason;
    }

    result.ok = metrics.failure_reason.empty() && frames > 0 && metrics.hardware_decoded;
    return result;
}

}  // namespace kivo::video::video_plane::probe
