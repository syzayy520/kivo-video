#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Reason for falling back from the primary path.
enum class FallbackReason {
    SourceUnavailable,
    CodecUnsupported,
    DeviceLost,
    NetworkTimeout,
    InsufficientBandwidth
};

// Estimated quality impact of using the fallback path.
enum class QualityImpact {
    None,
    Minor,
    Moderate,
    Significant
};

// Describes a fallback plan when the primary playback path is unavailable.
struct FallbackStrategy {
    ContractMetadata metadata;
    std::string strategy_id;
    std::string primary_path;
    std::string fallback_path;
    FallbackReason fallback_reason{FallbackReason::SourceUnavailable};
    QualityImpact estimated_quality_impact{QualityImpact::None};
    bool auto_apply{true};
    bool user_notification_required{false};
};

}  // namespace kivo::cinema_engine
