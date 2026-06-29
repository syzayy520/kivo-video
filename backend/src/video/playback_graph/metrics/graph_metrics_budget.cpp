#include "video/playback_graph/metrics/graph_metrics_budget.hpp"

namespace kivo::video::playback_graph::runtime {

BudgetValidationResult GraphMetricsBudget::validate(const PlaybackGraphPolicy& policy,
                                                    bool safety_critical) noexcept {
    if (policy.video_queue_budget.frames_mode == QueueBudgetDimensionMode::Bounded &&
        policy.video_queue_budget.frames_limit == 0) {
        return BudgetValidationResult{false,
                                      PlaybackGraphError::PolicyRejectedSafetyCritical,
                                      false};
    }
    if (policy.audio_queue_budget.bytes_mode == QueueBudgetDimensionMode::Bounded &&
        policy.audio_queue_budget.bytes_limit == 0) {
        return BudgetValidationResult{false,
                                      PlaybackGraphError::PolicyRejectedSafetyCritical,
                                      false};
    }
    if (!safety_critical && policy.tuning.seqlock_max_attempt_count == 0) {
        return BudgetValidationResult{true, PlaybackGraphError::PolicyAdjustedNonCritical, true};
    }
    if (safety_critical && policy.tuning.seqlock_max_attempt_count == 0) {
        return BudgetValidationResult{false,
                                      PlaybackGraphError::PolicyRejectedSafetyCritical,
                                      false};
    }
    return BudgetValidationResult{true, PlaybackGraphError::None, false};
}

std::uint32_t GraphMetricsBudget::required_policy_field_count() const noexcept {
    return 30;
}

}  // namespace kivo::video::playback_graph::runtime
