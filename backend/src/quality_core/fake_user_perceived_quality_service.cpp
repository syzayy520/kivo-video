#include "kivo/cinema_engine/quality_core/fake_user_perceived_quality_service.hpp"

namespace kivo::cinema_engine {

UserPerceivedQualitySnapshot build_good_quality_snapshot(const std::string& snapshot_id,
                                                        const std::optional<std::string>& session_id) {
    UserPerceivedQualitySnapshot snapshot;
    snapshot.metadata.schema_version = "1.1";
    snapshot.metadata.trace_id = "fake-good-quality";
    snapshot.quality_snapshot_id = snapshot_id;
    snapshot.session_id = session_id;
    snapshot.overall_score = 80.0;
    snapshot.video_quality_score = 85.0;
    snapshot.audio_quality_score = 90.0;
    snapshot.buffering_score = 95.0;
    snapshot.latency_score = 70.0;
    snapshot.stability_score = 85.0;
    snapshot.user_sentiment = UserSentiment::Good;
    snapshot.major_issues = {};
    snapshot.minor_issues = {"Occasional slight stutter"};
    snapshot.recommendations = {"Consider lowering video bitrate"};
    return snapshot;
}

UserPerceivedQualitySnapshot build_excellent_quality_snapshot(const std::string& snapshot_id,
                                                             const std::optional<std::string>& session_id) {
    UserPerceivedQualitySnapshot snapshot;
    snapshot.metadata.schema_version = "1.1";
    snapshot.metadata.trace_id = "fake-excellent-quality";
    snapshot.quality_snapshot_id = snapshot_id;
    snapshot.session_id = session_id;
    snapshot.overall_score = 95.0;
    snapshot.video_quality_score = 98.0;
    snapshot.audio_quality_score = 99.0;
    snapshot.buffering_score = 100.0;
    snapshot.latency_score = 95.0;
    snapshot.stability_score = 99.0;
    snapshot.user_sentiment = UserSentiment::Excellent;
    snapshot.major_issues = {};
    snapshot.minor_issues = {};
    snapshot.recommendations = {};
    return snapshot;
}

UserPerceivedQualitySnapshot build_poor_quality_snapshot(const std::string& snapshot_id,
                                                        const std::optional<std::string>& session_id) {
    UserPerceivedQualitySnapshot snapshot;
    snapshot.metadata.schema_version = "1.1";
    snapshot.metadata.trace_id = "fake-poor-quality";
    snapshot.quality_snapshot_id = snapshot_id;
    snapshot.session_id = session_id;
    snapshot.overall_score = 30.0;
    snapshot.video_quality_score = 40.0;
    snapshot.audio_quality_score = 60.0;
    snapshot.buffering_score = 20.0;
    snapshot.latency_score = 50.0;
    snapshot.stability_score = 30.0;
    snapshot.user_sentiment = UserSentiment::Poor;
    snapshot.major_issues = {"Frequent buffering", "Low video resolution"};
    snapshot.minor_issues = {"Audio sync drift"};
    snapshot.recommendations = {"Check network connection", "Use lower quality stream"};
    return snapshot;
}

UserPerceivedQualitySnapshot build_terrible_quality_snapshot(const std::string& snapshot_id,
                                                            const std::optional<std::string>& session_id) {
    UserPerceivedQualitySnapshot snapshot;
    snapshot.metadata.schema_version = "1.1";
    snapshot.metadata.trace_id = "fake-terrible-quality";
    snapshot.quality_snapshot_id = snapshot_id;
    snapshot.session_id = session_id;
    snapshot.overall_score = 5.0;
    snapshot.video_quality_score = 10.0;
    snapshot.audio_quality_score = 20.0;
    snapshot.buffering_score = 0.0;
    snapshot.latency_score = 10.0;
    snapshot.stability_score = 5.0;
    snapshot.user_sentiment = UserSentiment::Terrible;
    snapshot.major_issues = {"Playback failure", "No video", "No audio"};
    snapshot.minor_issues = {};
    snapshot.recommendations = {"Restart application", "Check media file integrity"};
    return snapshot;
}

UserPerceivedQualitySnapshot build_custom_quality_snapshot(const std::string& snapshot_id,
                                                          double overall_score,
                                                          double video_score,
                                                          double audio_score,
                                                          double buffering_score,
                                                          double latency_score,
                                                          double stability_score) {
    UserPerceivedQualitySnapshot snapshot;
    snapshot.metadata.schema_version = "1.1";
    snapshot.metadata.trace_id = "fake-custom-quality";
    snapshot.quality_snapshot_id = snapshot_id;
    snapshot.session_id = std::nullopt;
    snapshot.overall_score = overall_score;
    snapshot.video_quality_score = video_score;
    snapshot.audio_quality_score = audio_score;
    snapshot.buffering_score = buffering_score;
    snapshot.latency_score = latency_score;
    snapshot.stability_score = stability_score;
    // Determine sentiment based on overall score
    if (overall_score >= 90.0) {
        snapshot.user_sentiment = UserSentiment::Excellent;
    } else if (overall_score >= 70.0) {
        snapshot.user_sentiment = UserSentiment::Good;
    } else if (overall_score >= 50.0) {
        snapshot.user_sentiment = UserSentiment::Fair;
    } else if (overall_score >= 30.0) {
        snapshot.user_sentiment = UserSentiment::Poor;
    } else {
        snapshot.user_sentiment = UserSentiment::Terrible;
    }
    snapshot.major_issues = {};
    snapshot.minor_issues = {};
    snapshot.recommendations = {};
    return snapshot;
}

}  // namespace kivo::cinema_engine