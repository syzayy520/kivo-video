// progress_watchdog.hpp — P5B B11: Progress Watchdog
#pragma once
#include <cstdint>
#include <string>
#include <optional>
#include "../evidence/evidence_record.hpp"

namespace kivo::video_plane::input_bridge {

struct WatchdogConfig {
    uint64_t stall_threshold_ms{5000};   // stall detection threshold
    uint64_t check_interval_ms{1000};     // check interval
    uint32_t max_stall_count{3};          // max stalls before alert
    bool operator==(WatchdogConfig const&) const = default;
};

enum class WatchdogState {
    NotStarted,
    Running,
    Stalled,
    Alerted
};

// ProgressWatchdog — monitors pipeline progress, detects stalls
class ProgressWatchdog {
public:
    explicit ProgressWatchdog(WatchdogConfig const& config)
        : config_(config) {}

    void start(uint64_t current_time_ms) {
        state_ = WatchdogState::Running;
        last_progress_time_ = current_time_ms;
        stall_count_ = 0;
    }

    // Report progress
    void report_progress(uint64_t current_time_ms) {
        last_progress_time_ = current_time_ms;
        if (state_ == WatchdogState::Stalled) {
            state_ = WatchdogState::Running;
        }
    }

    // Check for stall
    std::optional<evidence::P5EvidenceRecord> check(uint64_t current_time_ms) {
        if (state_ != WatchdogState::Running && state_ != WatchdogState::Stalled) {
            return std::nullopt;
        }
        uint64_t elapsed = current_time_ms - last_progress_time_;
        if (elapsed > config_.stall_threshold_ms) {
            state_ = WatchdogState::Stalled;
            ++stall_count_;
            if (stall_count_ >= config_.max_stall_count) {
                state_ = WatchdogState::Alerted;
                evidence::P5EvidenceRecord ev;
                ev.evidence_kind = "ProgressStallAlert";
                ev.reason_code = "progress_stalled";
                ev.technical_detail_code = "watchdog_alert";
                return ev;
            }
        }
        return std::nullopt;
    }

    WatchdogState state() const { return state_; }
    uint32_t stall_count() const { return stall_count_; }
    void stop() { state_ = WatchdogState::NotStarted; }

private:
    WatchdogConfig config_;
    WatchdogState state_{WatchdogState::NotStarted};
    uint64_t last_progress_time_{0};
    uint32_t stall_count_{0};
};

}  // namespace kivo::video_plane::input_bridge
