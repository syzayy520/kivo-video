#pragma once

#include "kivo/video_plane/stamp/stamp_validation_kind.hpp"
#include <string>

namespace kivo::video_plane::stamp {

enum class StampValidationOutcome {
    Valid,                    // stamp passes all checks
    StaleCoreStamp,           // core stamp behind last known
    PipelineEpochInvalid,     // epoch doesn't match expected
    SkipForwardInvalid,       // forward jump too large
    RecoveryBookmarkMismatch, // recovery bookmark inconsistent
    BootstrapNotReady         // bootstrap prerequisites not met
};

struct StampValidationResult {
    StampValidationOutcome outcome{StampValidationOutcome::Valid};
    StampValidationKind kind{StampValidationKind::BootstrapCheck};
    std::string detail_code;
    std::string evidence_id;
    bool operator==(StampValidationResult const&) const = default;
};

}  // namespace kivo::video_plane::stamp
