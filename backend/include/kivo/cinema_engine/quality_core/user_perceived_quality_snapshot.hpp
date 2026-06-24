#pragma once

#include <optional>
#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

enum class UserSentiment {
    Excellent,
    Good,
    Fair,
    Poor,
    Terrible,
};

struct UserPerceivedQualitySnapshot {
    ContractMetadata metadata;
    std::string quality_snapshot_id;
    std::optional<std::string> session_id;
    double overall_score{0.0};  // 0-100
    double video_quality_score{0.0};
    double audio_quality_score{0.0};
    double buffering_score{0.0};
    double latency_score{0.0};
    double stability_score{0.0};
    UserSentiment user_sentiment{UserSentiment::Fair};
    std::vector<std::string> major_issues;
    std::vector<std::string> minor_issues;
    std::vector<std::string> recommendations;
};

}  // namespace kivo::cinema_engine