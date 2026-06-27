// find_stream_info_deadline.hpp — P5C C4: find_stream_info deadline enforcement
#pragma once
#include <cstdint>
#include <optional>
#include "../evidence/evidence_record.hpp"

namespace kivo::video_plane::ffmpeg_demux {

struct FindStreamInfoConfig {
    uint64_t deadline_ms{10000};  // 10s default
    uint64_t progress_interval_ms{1000};
    bool emit_evidence_on_timeout{true};
    bool operator==(FindStreamInfoConfig const&) const = default;
};

enum class FindStreamInfoResult {
    Success,
    Timeout,
    PartialSuccess,
    Failed
};

class FakeFindStreamInfo {
public:
    explicit FakeFindStreamInfo(FindStreamInfoConfig const& config)
        : config_(config) {}

    // Simulate find_stream_info with deadline
    std::pair<FindStreamInfoResult, std::optional<evidence::P5EvidenceRecord>>
    execute(uint64_t elapsed_ms, bool streams_found) {
        if (elapsed_ms > config_.deadline_ms) {
            if (config_.emit_evidence_on_timeout) {
                evidence::P5EvidenceRecord ev;
                ev.evidence_kind = "FindStreamInfoTimeout";
                ev.reason_code = "deadline_exceeded";
                ev.technical_detail_code = "find_stream_info_timeout";
                return {FindStreamInfoResult::Timeout, ev};
            }
            return {FindStreamInfoResult::Timeout, std::nullopt};
        }
        if (streams_found) {
            return {FindStreamInfoResult::Success, std::nullopt};
        }
        return {FindStreamInfoResult::Failed, std::nullopt};
    }

    FindStreamInfoConfig const& config() const { return config_; }

private:
    FindStreamInfoConfig config_;
};

}  // namespace kivo::video_plane::ffmpeg_demux
