#pragma once

#include <cstdint>

#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_policy.hpp"

namespace kivo::video::playback_graph::runtime {

struct BudgetValidationResult {
    bool accepted{false};
    PlaybackGraphError error{PlaybackGraphError::None};
    bool noncritical_adjusted{false};
};

class GraphMetricsBudget {
public:
    [[nodiscard]] BudgetValidationResult validate(const PlaybackGraphPolicy& policy,
                                                  bool safety_critical) noexcept;
    [[nodiscard]] std::uint32_t required_policy_field_count() const noexcept;
};

}  // namespace kivo::video::playback_graph::runtime
