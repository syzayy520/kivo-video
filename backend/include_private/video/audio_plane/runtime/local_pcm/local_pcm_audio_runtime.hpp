#pragma once

#include <cstdint>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "kivo/cinema_engine/demux_core/kivo_packet.hpp"

namespace kivo::video::audio_plane::runtime::local_pcm {

struct LocalPcmAudioOpenRequest {
    void* codec_context{nullptr};
    std::string codec_name;
    std::int32_t stream_index{-1};
};

struct LocalPcmAudioPacket {
    std::int32_t stream_index{-1};
    std::int64_t pts{0};
    std::int64_t dts{0};
    std::int64_t duration{0};
    bool is_key_frame{false};
    std::vector<std::uint8_t> data;
    std::vector<kivo::cinema_engine::KivoPacketSideData> side_data;
};

struct LocalPcmAudioStats {
    bool decoder_open{false};
    bool output_initialized{false};
    bool output_started{false};
    bool decode_started{false};
    bool pcm_conversion_started{false};
    bool pcm_write_started{false};
    std::uint64_t packets_submitted{0};
    std::uint64_t frames_decoded{0};
    std::uint64_t frames_converted{0};
    std::uint64_t frames_written{0};
    std::uint64_t packets_dropped{0};
    std::uint64_t bytes_written{0};
    std::uint64_t flush_count{0};
    std::uint64_t writer_reset_count{0};
    std::uint64_t stale_packets_discarded{0};
    std::int32_t stream_index{-1};
    std::int32_t output_sample_rate{0};
    std::int32_t output_channel_count{0};
    std::int32_t output_bit_depth{0};
    std::string codec_name;
    std::string output_sample_format;
    std::string error;
};

class LocalPcmAudioRuntime {
public:
    LocalPcmAudioRuntime();
    ~LocalPcmAudioRuntime();

    LocalPcmAudioRuntime(const LocalPcmAudioRuntime&) = delete;
    LocalPcmAudioRuntime& operator=(const LocalPcmAudioRuntime&) = delete;

    [[nodiscard]] bool open(LocalPcmAudioOpenRequest request) noexcept;
    [[nodiscard]] bool submit_packet(LocalPcmAudioPacket packet) noexcept;
    void pause() noexcept;
    void resume() noexcept;
    void flush() noexcept;
    void close() noexcept;

    [[nodiscard]] bool is_active() const noexcept;
    [[nodiscard]] LocalPcmAudioStats stats() const noexcept;

private:
    struct Impl;

    void worker_loop() noexcept;
    [[nodiscard]] bool process_packet(LocalPcmAudioPacket packet) noexcept;
    [[nodiscard]] bool write_pcm_buffer(const std::vector<std::uint8_t>& pcm_data,
                                        std::uint64_t flush_generation) noexcept;
    void set_error(std::string error) noexcept;

    static constexpr std::size_t kMaxQueuedPackets = 2048;

    mutable std::mutex mutex_;
    std::condition_variable queue_changed_;
    std::deque<LocalPcmAudioPacket> queue_;
    std::thread worker_;
    LocalPcmAudioStats stats_{};
    Impl* impl_{nullptr};
    std::uint64_t flush_generation_{0};
    bool accepting_packets_{false};
    bool stop_requested_{false};
};

}  // namespace kivo::video::audio_plane::runtime::local_pcm
