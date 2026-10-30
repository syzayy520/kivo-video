#include "video/audio_plane/runtime/local_pcm/local_pcm_audio_runtime.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <utility>

#include "kivo/cinema_engine/audio_core/audio_endpoint_contract.hpp"
#include "kivo/cinema_engine/audio_output/decoded_audio_frame_converter.hpp"
#include "kivo/cinema_engine/audio_output/wasapi_shared_pcm_writer.hpp"
#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/demux_core/kivo_packet.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"

namespace kivo::video::audio_plane::runtime::local_pcm {
namespace {

[[nodiscard]] std::string default_sample_format(const std::int32_t bit_depth) {
    return bit_depth == 32 ? "flt" : "s16";
}

[[nodiscard]] std::string lower_ascii(std::string value) {
    for (char& ch : value) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return value;
}

[[nodiscard]] bool requires_passthrough_or_fallback(const std::string& codec_name) {
    const std::string codec = lower_ascii(codec_name);
    return codec == "truehd" || codec == "mlp";
}

}  // namespace

struct LocalPcmAudioRuntime::Impl {
    kivo::cinema_engine::RealSoftwareDecodeRuntime decoder;
    kivo::cinema_engine::DecodedAudioFrameConverter converter;
    kivo::cinema_engine::WasapiSharedPcmWriter writer;
    kivo::cinema_engine::PcmFormat pcm_target;
    std::mutex io_mutex;
};

LocalPcmAudioRuntime::LocalPcmAudioRuntime() = default;

LocalPcmAudioRuntime::~LocalPcmAudioRuntime() {
    close();
}

bool LocalPcmAudioRuntime::open(LocalPcmAudioOpenRequest request) noexcept {
    close();
    {
        std::lock_guard lock(mutex_);
        stats_ = {};
        stats_.stream_index = request.stream_index;
        stats_.codec_name = request.codec_name;
    }

    if (request.codec_context == nullptr || request.stream_index < 0) {
        set_error("invalid audio open request");
        return false;
    }

    if (requires_passthrough_or_fallback(request.codec_name)) {
        kivo::cinema_engine::FfmpegCodecHandle handle{request.codec_context};
        kivo::cinema_engine::FfmpegAdapter::free_codec_context(handle);
        set_error("audio codec requires passthrough or compatible fallback");
        return false;
    }

    impl_ = new (std::nothrow) Impl();
    if (impl_ == nullptr) {
        kivo::cinema_engine::FfmpegCodecHandle handle{request.codec_context};
        kivo::cinema_engine::FfmpegAdapter::free_codec_context(handle);
        set_error("audio runtime allocation failed");
        return false;
    }

    if (!impl_->decoder.adopt_codec_context(
            request.codec_context, request.codec_name, "p10_audio_decode")) {
        const std::string error = impl_->decoder.inspector_hint().empty()
                                      ? "audio decoder adopt failed"
                                      : impl_->decoder.inspector_hint();
        delete impl_;
        impl_ = nullptr;
        set_error(error);
        return false;
    }

    kivo::cinema_engine::AudioEndpointRuntimeContract endpoint;
    endpoint.endpoint_id = "default";
    endpoint.endpoint_name = "Default Audio Device";
    endpoint.output_mode = "shared_pcm";
    endpoint.sample_rate = 48000;
    endpoint.channels = 2;
    endpoint.bit_depth = 16;
    if (!impl_->writer.initialize(endpoint)) {
        const std::string error = impl_->writer.last_error().empty()
                                      ? "wasapi writer initialize failed"
                                      : impl_->writer.last_error();
        impl_->decoder.close();
        delete impl_;
        impl_ = nullptr;
        set_error(error);
        return false;
    }

    {
        std::lock_guard lock(mutex_);
        stats_.decoder_open = true;
        stats_.output_initialized = true;
        stats_.output_sample_rate = impl_->writer.sample_rate();
        stats_.output_channel_count = impl_->writer.channel_count();
        stats_.output_bit_depth = impl_->writer.bit_depth();
        stats_.output_sample_format = impl_->writer.sample_format();
    }

    impl_->pcm_target.sample_rate =
        stats().output_sample_rate > 0 ? stats().output_sample_rate : 48000;
    impl_->pcm_target.channels =
        stats().output_channel_count > 0 ? stats().output_channel_count : 2;
    impl_->pcm_target.bit_depth =
        stats().output_bit_depth > 0 ? stats().output_bit_depth : 16;
    impl_->pcm_target.sample_format =
        stats().output_sample_format.empty()
            ? default_sample_format(impl_->pcm_target.bit_depth)
            : stats().output_sample_format;
    impl_->pcm_target.is_planar = false;

    {
        std::lock_guard lock(mutex_);
        accepting_packets_ = true;
        stop_requested_ = false;
    }
    try {
        worker_ = std::thread([this]() { worker_loop(); });
    } catch (...) {
        {
            std::lock_guard lock(mutex_);
            accepting_packets_ = false;
            stop_requested_ = true;
        }
        impl_->writer.release();
        impl_->decoder.close();
        delete impl_;
        impl_ = nullptr;
        set_error("audio worker start failed");
        return false;
    }
    return true;
}

bool LocalPcmAudioRuntime::submit_packet(LocalPcmAudioPacket packet) noexcept {
    if (packet.data.empty()) {
        return true;
    }
    {
        std::lock_guard lock(mutex_);
        if (impl_ == nullptr || !stats_.decoder_open || !accepting_packets_) {
            return false;
        }
        if (packet.stream_index != stats_.stream_index) {
            return true;
        }
        if (queue_.size() >= kMaxQueuedPackets) {
            ++stats_.packets_dropped;
            return true;
        }
        ++stats_.packets_submitted;
        stats_.decode_started = true;
        queue_.push_back(std::move(packet));
    }
    queue_changed_.notify_one();
    return true;
}

bool LocalPcmAudioRuntime::process_packet(LocalPcmAudioPacket packet) noexcept {
    Impl* impl = impl_;
    if (impl == nullptr) {
        return false;
    }
    std::uint64_t packet_flush_generation = 0;
    {
        std::lock_guard lock(mutex_);
        packet_flush_generation = flush_generation_;
    }

    kivo::cinema_engine::KivoPacket kivo_packet;
    kivo_packet.stream_index = packet.stream_index;
    kivo_packet.pts = packet.pts;
    kivo_packet.dts = packet.dts;
    kivo_packet.duration = packet.duration;
    kivo_packet.is_key_frame = packet.is_key_frame;
    kivo_packet.data = std::move(packet.data);
    kivo_packet.side_data = std::move(packet.side_data);
    kivo_packet.packet_id = "audio_pkt_" + std::to_string(packet.stream_index) +
                            "_" + std::to_string(packet.pts);

    std::uint64_t packet_index = 0;
    {
        std::lock_guard lock(mutex_);
        packet_index = stats_.packets_submitted;
    }

    const auto decoded = [&]() {
        std::lock_guard io_lock(impl->io_mutex);
        return impl->decoder.decode(
            kivo_packet, "p10_audio_decode_" + std::to_string(packet_index));
    }();
    {
        std::lock_guard lock(mutex_);
        if (packet_flush_generation != flush_generation_) {
            ++stats_.stale_packets_discarded;
            return true;
        }
    }
    if (!decoded.success) {
        set_error(decoded.error_message.empty() ? "audio decode failed"
                                                : decoded.error_message);
        return false;
    }
    if (decoded.needs_more_input || decoded.eof ||
        decoded.frame.stream_kind != "audio" || decoded.frame.frame_data.empty()) {
        return true;
    }

    {
        std::lock_guard lock(mutex_);
        ++stats_.frames_decoded;
        stats_.pcm_conversion_started = true;
    }
    const auto pcm = [&]() {
        std::lock_guard io_lock(impl->io_mutex);
        return impl->converter.convert(decoded.frame, impl->pcm_target);
    }();
    {
        std::lock_guard lock(mutex_);
        if (packet_flush_generation != flush_generation_) {
            ++stats_.stale_packets_discarded;
            return true;
        }
    }
    if (!pcm.success || pcm.pcm_data.empty()) {
        set_error(pcm.error_message.empty() ? "audio pcm conversion failed"
                                            : pcm.error_message);
        return false;
    }

    {
        std::lock_guard lock(mutex_);
        ++stats_.frames_converted;
    }
    return write_pcm_buffer(pcm.pcm_data, packet_flush_generation);
}

void LocalPcmAudioRuntime::worker_loop() noexcept {
    while (true) {
        LocalPcmAudioPacket packet;
        {
            std::unique_lock lock(mutex_);
            queue_changed_.wait(lock, [this]() noexcept {
                return stop_requested_ || !queue_.empty();
            });
            if (stop_requested_) {
                break;
            }
            packet = std::move(queue_.front());
            queue_.pop_front();
        }
        if (!process_packet(std::move(packet))) {
            std::lock_guard lock(mutex_);
            accepting_packets_ = false;
            queue_.clear();
        }
    }
}

void LocalPcmAudioRuntime::pause() noexcept {
    Impl* impl = impl_;
    if (impl != nullptr) {
        std::lock_guard io_lock(impl->io_mutex);
        (void)impl->writer.stop();
    }
    {
        std::lock_guard lock(mutex_);
        stats_.output_started = false;
    }
}

void LocalPcmAudioRuntime::resume() noexcept {
    Impl* impl = impl_;
    if (impl == nullptr) {
        return;
    }
    bool should_start = false;
    {
        std::lock_guard lock(mutex_);
        should_start = stats_.pcm_write_started && !stats_.output_started;
    }
    if (!should_start) {
        return;
    }
    std::lock_guard io_lock(impl->io_mutex);
    const bool started = impl->writer.start();
    {
        std::lock_guard lock(mutex_);
        stats_.output_started = started;
    }
}

void LocalPcmAudioRuntime::flush() noexcept {
    {
        std::lock_guard lock(mutex_);
        ++flush_generation_;
        ++stats_.flush_count;
        queue_.clear();
    }
    Impl* impl = impl_;
    if (impl != nullptr && stats().decoder_open) {
        std::lock_guard io_lock(impl->io_mutex);
        if (impl->writer.reset()) {
            std::lock_guard lock(mutex_);
            stats_.output_started = false;
            ++stats_.writer_reset_count;
        }
        (void)impl->decoder.flush("p10_audio_flush");
    }
    queue_changed_.notify_all();
}

void LocalPcmAudioRuntime::close() noexcept {
    {
        std::lock_guard lock(mutex_);
        accepting_packets_ = false;
        stop_requested_ = true;
        queue_.clear();
    }
    queue_changed_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }

    Impl* impl = impl_;
    if (impl != nullptr) {
        {
            std::lock_guard io_lock(impl->io_mutex);
            (void)impl->writer.stop();
            impl->writer.release();
            impl->decoder.close();
        }
        delete impl;
        impl_ = nullptr;
    }
    {
        std::lock_guard lock(mutex_);
        stats_ = {};
        accepting_packets_ = false;
        stop_requested_ = false;
    }
}

bool LocalPcmAudioRuntime::is_active() const noexcept {
    std::lock_guard lock(mutex_);
    return stats_.decoder_open;
}

LocalPcmAudioStats LocalPcmAudioRuntime::stats() const noexcept {
    std::lock_guard lock(mutex_);
    return stats_;
}

bool LocalPcmAudioRuntime::write_pcm_buffer(
    const std::vector<std::uint8_t>& pcm_data,
    const std::uint64_t flush_generation) noexcept {
    Impl* impl = impl_;
    if (impl == nullptr || pcm_data.empty()) {
        return false;
    }

    const std::int32_t bytes_per_frame =
        impl->pcm_target.channels * (impl->pcm_target.bit_depth / 8);
    if (bytes_per_frame <= 0) {
        set_error("invalid audio pcm target format");
        return false;
    }

    std::size_t offset = 0;
    while (offset < pcm_data.size()) {
        {
            std::lock_guard lock(mutex_);
            if (flush_generation != flush_generation_) {
                ++stats_.stale_packets_discarded;
                return true;
            }
            if (stop_requested_) {
                return false;
            }
        }

        const auto remaining =
            static_cast<std::int32_t>(pcm_data.size() - offset);
        kivo::cinema_engine::WriteResult write;
        {
            std::lock_guard io_lock(impl->io_mutex);
            write = impl->writer.write(pcm_data.data() + offset, remaining);
        }

        if (!write.success) {
            if (write.is_buffer_full) {
                std::unique_lock lock(mutex_);
                queue_changed_.wait_for(lock, std::chrono::milliseconds(2), [this]() noexcept {
                    return stop_requested_;
                });
                continue;
            }
            set_error(write.error_message.empty() ? "audio pcm write failed"
                                                  : write.error_message);
            return false;
        }

        const std::int32_t bytes_written = write.frames_written * bytes_per_frame;
        if (bytes_written <= 0) {
            std::unique_lock lock(mutex_);
            queue_changed_.wait_for(lock, std::chrono::milliseconds(2), [this]() noexcept {
                return stop_requested_;
            });
            continue;
        }

        const std::size_t safe_bytes_written =
            static_cast<std::size_t>(bytes_written) > (pcm_data.size() - offset)
                ? (pcm_data.size() - offset)
                : static_cast<std::size_t>(bytes_written);
        offset += safe_bytes_written;

        bool should_start = false;
        {
            std::lock_guard lock(mutex_);
            stats_.frames_written += static_cast<std::uint64_t>(write.frames_written);
            stats_.bytes_written += static_cast<std::uint64_t>(safe_bytes_written);
            stats_.pcm_write_started = true;
            should_start = !stats_.output_started;
        }
        if (should_start) {
            bool started = false;
            {
                std::lock_guard io_lock(impl->io_mutex);
                started = impl->writer.start();
            }
            if (!started) {
                set_error(impl->writer.last_error().empty() ? "wasapi start failed"
                                                            : impl->writer.last_error());
                return false;
            }
            std::lock_guard lock(mutex_);
            stats_.output_started = true;
        }

        if (offset < pcm_data.size() && write.is_buffer_full) {
            std::unique_lock lock(mutex_);
            queue_changed_.wait_for(lock, std::chrono::milliseconds(2), [this]() noexcept {
                return stop_requested_;
            });
        }
    }

    return true;
}

void LocalPcmAudioRuntime::set_error(std::string error) noexcept {
    std::lock_guard lock(mutex_);
    stats_.error = std::move(error);
}

}  // namespace kivo::video::audio_plane::runtime::local_pcm
