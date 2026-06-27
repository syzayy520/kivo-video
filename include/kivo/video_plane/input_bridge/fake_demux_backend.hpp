// fake_demux_backend.hpp — P5B B7/B8: Fake Demux Backend
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include "../stamp/core_video_stamp.hpp"
#include "../evidence/evidence_record.hpp"
#include "../failure/failure_correlation_id.hpp"

namespace kivo::video_plane::input_bridge {

struct DemuxPacket {
    CoreVideoStamp stamp;
    uint64_t stream_index{0};
    uint64_t pts{0};
    uint64_t dts{0};
    uint64_t byte_size{0};
    bool is_keyframe{false};
};

struct DemuxStreamInfo {
    uint64_t stream_index{0};
    std::string codec_name;
    uint64_t bitrate{0};  // 0 = unknown bitrate
    uint64_t duration_ms{0};
};

struct DemuxResult {
    bool success{false};
    bool eof{false};
    DemuxPacket packet;
    std::string evidence_id;
};

// FakeDemuxBackend — simulates demux without real FFmpeg
class FakeDemuxBackend {
public:
    FakeDemuxBackend() = default;

    // Initialize with known corpus (fake streams)
    void initialize(std::vector<DemuxStreamInfo> const& streams) {
        streams_ = streams;
        initialized_ = true;
        packet_index_ = 0;
    }

    // Demux next packet
    DemuxResult demux_next() {
        DemuxResult result;
        if (!initialized_) {
            result.evidence_id = "demux_not_initialized";
            return result;
        }
        if (packet_index_ >= 100) {  // fake 100 packets
            result.eof = true;
            result.success = true;
            return result;
        }
        result.success = true;
        result.packet.stamp = CoreVideoStamp{++next_stamp_};
        result.packet.stream_index = 0;
        result.packet.pts = packet_index_ * 33;  // 30fps
        result.packet.dts = result.packet.pts;
        result.packet.byte_size = 1024;
        result.packet.is_keyframe = (packet_index_ % 30 == 0);
        ++packet_index_;
        return result;
    }

    // Check for unknown bitrate and emit fallback evidence
    std::optional<evidence::P5EvidenceRecord> check_unknown_bitrate() const {
        for (auto const& s : streams_) {
            if (s.bitrate == 0) {
                evidence::P5EvidenceRecord ev;
                ev.evidence_kind = "UnknownBitrateFallback";
                ev.reason_code = "bitrate_unknown";
                ev.technical_detail_code = "fallback_to_default_bitrate";
                return ev;
            }
        }
        return std::nullopt;
    }

    std::vector<DemuxStreamInfo> const& streams() const { return streams_; }
    bool is_initialized() const { return initialized_; }
    uint64_t packet_index() const { return packet_index_; }

private:
    std::vector<DemuxStreamInfo> streams_;
    bool initialized_{false};
    uint64_t packet_index_{0};
    uint64_t next_stamp_{0};
};

}  // namespace kivo::video_plane::input_bridge
