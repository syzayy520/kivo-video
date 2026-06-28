// fake_evidence_collector.hpp — P6B runtime: fake in-memory evidence collector
// Header-only fake implementation. Platform-neutral.
#pragma once

#include <cstdint>
#include <vector>
#include "kivo/video/audio_plane/evidence/p6_evidence_record.hpp"

namespace kivo::video::audio_plane::runtime::evidence {

using kivo::video::audio_plane::evidence::P6EvidenceRecord;

// Severity bit indicating a ChainRoot record that must not be silently dropped.
constexpr int32_t CHAIN_ROOT_SEVERITY_BIT = 0x40000000;

// Fake in-memory evidence collector with TTL enforcement and a ring buffer.
// ChainRoot evidence (severity & CHAIN_ROOT_SEVERITY_BIT) is never silently
// dropped; when storage is full it forces compaction of non-chain-root
// records and increments compact_critical_counter_.
class FakeEvidenceCollector {
public:
    FakeEvidenceCollector() = default;

    // Configure TTL in milliseconds and storage capacity.
    void configure(uint64_t ttl_ms, size_t capacity) noexcept {
        ttl_ms_ = ttl_ms;
        capacity_ = capacity;
    }

    // Record a new evidence entry. Returns the assigned evidence_id (monotonic).
    // If storage is full:
    //   - non-chain-root records are evicted (oldest first, expired first)
    //   - if only chain-root records remain, compact_critical_counter_ is
    //     incremented and the new record is still appended (capacity grows)
    uint64_t record(
        int32_t kind,
        int32_t severity,
        uint64_t correlation_id,
        uint64_t engine_epoch,
        int64_t timestamp_ms) {
        drop_expired(timestamp_ms);
        if (records_.size() >= capacity_) {
            if (!evict_one_non_chain_root()) {
                compact_critical_counter_ += 1;
            }
        }
        next_id_ += 1;
        P6EvidenceRecord r{};
        r.evidence_id = next_id_;
        r.correlation_id = correlation_id;
        r.kind = kind;
        r.severity = severity;
        r.engine_epoch = engine_epoch;
        r.timestamp_ms = timestamp_ms;
        records_.push_back(r);
        return next_id_;
    }

    uint64_t compact_critical_counter() const noexcept {
        return compact_critical_counter_;
    }

    size_t count() const noexcept { return records_.size(); }

    void clear() noexcept {
        records_.clear();
        compact_critical_counter_ = 0;
        next_id_ = 0;
    }

    const std::vector<P6EvidenceRecord>& records() const noexcept {
        return records_;
    }

private:
    void drop_expired(int64_t now_ms) {
        for (size_t i = 0; i < records_.size();) {
            const auto& r = records_[i];
            bool is_chain_root = (r.severity & CHAIN_ROOT_SEVERITY_BIT) != 0;
            if (!is_chain_root && now_ms - r.timestamp_ms > static_cast<int64_t>(ttl_ms_)) {
                records_.erase(records_.begin() + static_cast<ptrdiff_t>(i));
            } else {
                ++i;
            }
        }
    }

    bool evict_one_non_chain_root() {
        for (size_t i = 0; i < records_.size(); ++i) {
            const auto& r = records_[i];
            bool is_chain_root = (r.severity & CHAIN_ROOT_SEVERITY_BIT) != 0;
            if (!is_chain_root) {
                records_.erase(records_.begin() + static_cast<ptrdiff_t>(i));
                return true;
            }
        }
        return false;
    }

    std::vector<P6EvidenceRecord> records_{};
    uint64_t next_id_{0};
    uint64_t ttl_ms_{30000};
    size_t capacity_{1024};
    uint64_t compact_critical_counter_{0};
};

}  // namespace kivo::video::audio_plane::runtime::evidence
