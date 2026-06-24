#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/quality_core/fake_user_perceived_quality_service.hpp"

using namespace kivo::cinema_engine;

static void test_good_quality_snapshot() {
    auto snapshot = build_good_quality_snapshot("snap-001", "session-001");
    assert(snapshot.metadata.schema_version == "1.1");
    assert(snapshot.quality_snapshot_id == "snap-001");
    assert(snapshot.session_id.has_value());
    assert(snapshot.session_id.value() == "session-001");
    assert(snapshot.overall_score == 80.0);
    assert(snapshot.video_quality_score == 85.0);
    assert(snapshot.audio_quality_score == 90.0);
    assert(snapshot.buffering_score == 95.0);
    assert(snapshot.latency_score == 70.0);
    assert(snapshot.stability_score == 85.0);
    assert(snapshot.user_sentiment == UserSentiment::Good);
    assert(snapshot.major_issues.empty());
    assert(snapshot.minor_issues.size() == 1);
    assert(snapshot.recommendations.size() == 1);
    std::cout << "  PASS: good_quality_snapshot\n";
}

static void test_excellent_quality_snapshot() {
    auto snapshot = build_excellent_quality_snapshot("snap-002");
    assert(snapshot.quality_snapshot_id == "snap-002");
    assert(!snapshot.session_id.has_value());
    assert(snapshot.overall_score == 95.0);
    assert(snapshot.user_sentiment == UserSentiment::Excellent);
    assert(snapshot.major_issues.empty());
    assert(snapshot.minor_issues.empty());
    assert(snapshot.recommendations.empty());
    std::cout << "  PASS: excellent_quality_snapshot\n";
}

static void test_poor_quality_snapshot() {
    auto snapshot = build_poor_quality_snapshot("snap-003", "session-003");
    assert(snapshot.quality_snapshot_id == "snap-003");
    assert(snapshot.overall_score == 30.0);
    assert(snapshot.user_sentiment == UserSentiment::Poor);
    assert(snapshot.major_issues.size() == 2);
    assert(snapshot.minor_issues.size() == 1);
    assert(snapshot.recommendations.size() == 2);
    std::cout << "  PASS: poor_quality_snapshot\n";
}

static void test_terrible_quality_snapshot() {
    auto snapshot = build_terrible_quality_snapshot("snap-004");
    assert(snapshot.quality_snapshot_id == "snap-004");
    assert(snapshot.overall_score == 5.0);
    assert(snapshot.user_sentiment == UserSentiment::Terrible);
    assert(snapshot.major_issues.size() == 3);
    std::cout << "  PASS: terrible_quality_snapshot\n";
}

static void test_custom_quality_snapshot() {
    auto snapshot = build_custom_quality_snapshot("snap-005", 75.0, 80.0, 90.0, 70.0, 60.0, 80.0);
    assert(snapshot.quality_snapshot_id == "snap-005");
    assert(snapshot.overall_score == 75.0);
    assert(snapshot.user_sentiment == UserSentiment::Good);  // 75 >= 70
    assert(snapshot.major_issues.empty());
    std::cout << "  PASS: custom_quality_snapshot\n";
}

static void test_custom_quality_snapshot_fair() {
    auto snapshot = build_custom_quality_snapshot("snap-006", 55.0, 60.0, 70.0, 50.0, 40.0, 55.0);
    assert(snapshot.user_sentiment == UserSentiment::Fair);  // 55 >= 50
    std::cout << "  PASS: custom_quality_snapshot_fair\n";
}

int main() {
    std::cout << "user_perceived_quality_test:\n";
    test_good_quality_snapshot();
    test_excellent_quality_snapshot();
    test_poor_quality_snapshot();
    test_terrible_quality_snapshot();
    test_custom_quality_snapshot();
    test_custom_quality_snapshot_fair();
    std::cout << "ALL 6 TESTS PASSED\n";
    return 0;
}