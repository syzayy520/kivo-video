// demux_quarantine.hpp — P5C C5/C6: Demux + Zombie quarantine
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "../failure/failure_correlation_id.hpp"

namespace kivo::video_plane::ffmpeg_demux {

enum class QuarantineReason {
    DemuxCrash,
    DemuxTimeout,
    ZombieProcess,
    RepeatedFailure,
    MalformedCorpus,
    Unknown
};

struct QuarantineEntry {
    failure::FailureCorrelationId correlation_id;
    QuarantineReason reason{QuarantineReason::Unknown};
    std::string source_uri;
    uint64_t quarantined_at_ms{0};
    bool is_zombie{false};
};

class FakeDemuxQuarantine {
public:
    FakeDemuxQuarantine() = default;

    // C5: Quarantine after demux crash
    void quarantine_for_crash(failure::FailureCorrelationId cid,
                               std::string const& source_uri,
                               uint64_t timestamp) {
        entries_.push_back({cid, QuarantineReason::DemuxCrash, source_uri, timestamp, false});
    }

    // C6: Quarantine zombie process
    void quarantine_zombie(failure::FailureCorrelationId cid,
                            std::string const& source_uri,
                            uint64_t timestamp) {
        entries_.push_back({cid, QuarantineReason::ZombieProcess, source_uri, timestamp, true});
    }

    // Check if source is quarantined
    bool is_quarantined(std::string const& source_uri) const {
        for (auto const& e : entries_) {
            if (e.source_uri == source_uri) return true;
        }
        return false;
    }

    // Check for zombies
    bool has_zombies() const {
        for (auto const& e : entries_) {
            if (e.is_zombie) return true;
        }
        return false;
    }

    // Get quarantine count
    size_t quarantine_count() const { return entries_.size(); }

    // Get zombie count
    size_t zombie_count() const {
        size_t count = 0;
        for (auto const& e : entries_) {
            if (e.is_zombie) ++count;
        }
        return count;
    }

    // Clear all quarantines
    void clear() { entries_.clear(); }

    std::vector<QuarantineEntry> const& entries() const { return entries_; }

private:
    std::vector<QuarantineEntry> entries_;
};

}  // namespace kivo::video_plane::ffmpeg_demux
