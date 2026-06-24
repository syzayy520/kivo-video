#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/quality_core/user_perceived_quality_snapshot.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

/// Build a default quality snapshot with Good sentiment.
UserPerceivedQualitySnapshot build_good_quality_snapshot(const std::string& snapshot_id,
                                                        const std::optional<std::string>& session_id = std::nullopt);

/// Build an excellent quality snapshot.
UserPerceivedQualitySnapshot build_excellent_quality_snapshot(const std::string& snapshot_id,
                                                             const std::optional<std::string>& session_id = std::nullopt);

/// Build a poor quality snapshot with major issues.
UserPerceivedQualitySnapshot build_poor_quality_snapshot(const std::string& snapshot_id,
                                                        const std::optional<std::string>& session_id = std::nullopt);

/// Build a terrible quality snapshot with critical failures.
UserPerceivedQualitySnapshot build_terrible_quality_snapshot(const std::string& snapshot_id,
                                                            const std::optional<std::string>& session_id = std::nullopt);

/// Build a custom quality snapshot with given scores.
UserPerceivedQualitySnapshot build_custom_quality_snapshot(const std::string& snapshot_id,
                                                          double overall_score,
                                                          double video_score,
                                                          double audio_score,
                                                          double buffering_score,
                                                          double latency_score,
                                                          double stability_score);

}  // namespace kivo::cinema_engine