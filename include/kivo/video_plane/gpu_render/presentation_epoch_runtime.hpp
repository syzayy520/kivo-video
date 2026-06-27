// presentation_epoch_runtime.hpp -- P5F F11: PresentationEpoch trigger/non-trigger
// Subtitle-only change does NOT trigger epoch change
// Geometry/color/protected-output changes DO trigger epoch change
#pragma once
#include <cstdint>
#include <string>
#include "../presentation/epoch_condition.hpp"

namespace kivo::video_plane::gpu_render {

using presentation::EpochCondition;
using presentation::PresentationEpoch;

enum class EpochTriggerKind {
    None,
    ViewportChange,
    CropChange,
    RotationChange,
    SurfaceChange,
    OutputColorChange,
    ProtectedOutputChange,
    SubtitleOnlyChange,  // does NOT trigger epoch
    GeometryChange,
    ColorTransitionChange
};

struct EpochTriggerResult {
    bool triggers_epoch{false};
    EpochTriggerKind trigger_kind{EpochTriggerKind::None};
    std::string reason;
    PresentationEpoch new_epoch;
};

class FakePresentationEpochRuntime {
public:
    PresentationEpoch const& current_epoch() const { return current_; }

    EpochTriggerResult evaluate_trigger(EpochTriggerKind kind) {
        EpochTriggerResult r;
        r.trigger_kind = kind;

        // Subtitle-only change does NOT trigger epoch
        if (kind == EpochTriggerKind::SubtitleOnlyChange || kind == EpochTriggerKind::None) {
            r.triggers_epoch = false;
            r.reason = "subtitle_only_change_no_epoch";
            r.new_epoch = current_;
            return r;
        }

        // All other changes trigger epoch
        r.triggers_epoch = true;
        r.reason = "geometry_color_or_protected_change_triggers_epoch";
        current_.epoch_id = ++next_epoch_id_;
        current_.start_timestamp = ++next_timestamp_;
        current_.condition = EpochCondition::Active;
        r.new_epoch = current_;
        return r;
    }

    void suspend() { current_.condition = EpochCondition::Suspended; }
    void end() { current_.condition = EpochCondition::Ended; }
    void activate() { current_.condition = EpochCondition::Active; }

private:
    PresentationEpoch current_;
    uint64_t next_epoch_id_{0};
    uint64_t next_timestamp_{0};
};

}  // namespace kivo::video_plane::gpu_render
