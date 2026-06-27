// p4_video_input_bridge.hpp — P5B B1: P4 Video Input Bridge (fake runtime)
#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include <atomic>
#include "../stamp/core_video_stamp.hpp"
#include "../queue/video_pipeline_id.hpp"

namespace kivo::video_plane::input_bridge {

using stamp::CoreVideoStamp;
using queue::VideoPipelineId;

struct P4InputBridgeConfig {
    VideoPipelineId pipeline_id;
    uint64_t max_lease_window_ms{5000};
    bool enable_progress_watchdog{true};
    bool operator==(P4InputBridgeConfig const&) const = default;
};

enum class BridgeState {
    NotInitialized,
    Ready,
    Accepting,
    Draining,
    Closed
};

// Fake P4VideoInputBridge — simulates P4->P5 input bridge without real P4
class FakeP4VideoInputBridge {
public:
    explicit FakeP4VideoInputBridge(P4InputBridgeConfig const& config)
        : config_(config), state_(BridgeState::Ready) {}

    BridgeState state() const { return state_; }
    VideoPipelineId pipeline_id() const { return config_.pipeline_id; }

    // Accept input from P4, assign CoreVideoStamp
    CoreVideoStamp accept_input(uint64_t input_data_ref) {
        if (state_ != BridgeState::Ready && state_ != BridgeState::Accepting) {
            return CoreVideoStamp{0};
        }
        state_ = BridgeState::Accepting;
        CoreVideoStamp stamp{++next_stamp_};
        last_input_ref_ = input_data_ref;
        return stamp;
    }

    // Drain remaining inputs
    void drain() { state_ = BridgeState::Draining; }

    // Close bridge
    void close() { state_ = BridgeState::Closed; }

    // Check if lease is still valid
    bool is_lease_valid(uint64_t current_time_ms) const {
        if (last_lease_time_ == 0) return false;
        return (current_time_ms - last_lease_time_) <= config_.max_lease_window_ms;
    }

    // Renew lease from P4
    void renew_lease(uint64_t current_time_ms) {
        last_lease_time_ = current_time_ms;
    }

    uint64_t last_input_ref() const { return last_input_ref_; }

private:
    P4InputBridgeConfig config_;
    BridgeState state_{BridgeState::NotInitialized};
    uint64_t next_stamp_{0};
    uint64_t last_input_ref_{0};
    uint64_t last_lease_time_{0};
};

}  // namespace kivo::video_plane::input_bridge
