// device_removed_sim.hpp -- P5F F10: device removed simulation
#pragma once
#include <cstdint>
#include <string>
#include "../gpu/device_removed_reason.hpp"

namespace kivo::video_plane::gpu_render {

using gpu::DeviceRemovedReason;
using gpu::DeviceRemovedInfo;

struct DeviceRemovedSimulation {
    DeviceRemovedInfo info;
    bool recovery_triggered{false};
    std::string recovery_action;
    std::string evidence_id;
};

class FakeDeviceRemovedHandler {
public:
    DeviceRemovedSimulation simulate(DeviceRemovedReason reason, const std::string& description) {
        DeviceRemovedSimulation sim;
        sim.info.reason = reason;
        sim.info.description = description;
        sim.recovery_triggered = true;
        sim.evidence_id = "device_removed_" + std::to_string(++event_count_);

        switch (reason) {
            case DeviceRemovedReason::Hung:
                sim.recovery_action = "rebuild_device_from_scratch";
                break;
            case DeviceRemovedReason::OutOfMemory:
                sim.recovery_action = "release_nonessential_resources";
                break;
            case DeviceRemovedReason::DriverError:
                sim.recovery_action = "fallback_to_software_decode";
                break;
            case DeviceRemovedReason::HardwareFailure:
                sim.recovery_action = "fail_to_upper_layer";
                break;
            default:
                sim.recovery_action = "log_and_attempt_recovery";
                break;
        }
        last_sim_ = sim;
        return sim;
    }

    DeviceRemovedSimulation const& last() const { return last_sim_; }
    uint32_t event_count() const { return event_count_; }

private:
    DeviceRemovedSimulation last_sim_;
    uint32_t event_count_{0};
};

}  // namespace kivo::video_plane::gpu_render
