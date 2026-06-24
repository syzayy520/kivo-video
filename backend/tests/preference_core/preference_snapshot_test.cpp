#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/preference_core/fake_preference_service.hpp"

using namespace kivo::cinema_engine;

static void test_default_snapshot() {
    auto snapshot = build_default_preference_snapshot();
    assert(snapshot.metadata.schema_version == "1.1");
    assert(!snapshot.subtitle.enabled);
    assert(snapshot.subtitle.style == "default");
    assert(snapshot.subtitle.position == "bottom");
    assert(snapshot.audio.volume == 1.0);
    assert(snapshot.audio.channel_layout == "stereo");
    assert(snapshot.video.deinterlace);
    assert(snapshot.video.hdr_preference == "auto");
    assert(snapshot.playback.auto_play);
    assert(snapshot.playback.auto_next);
    assert(snapshot.playback.resume_from_last);
    std::cout << "  PASS: default_snapshot\n";
}

static void test_subtitle_preference() {
    auto snapshot = build_preference_snapshot_with_subtitle("en", true);
    assert(snapshot.subtitle.language.has_value());
    assert(snapshot.subtitle.language.value() == "en");
    assert(snapshot.subtitle.enabled);
    std::cout << "  PASS: subtitle_preference\n";
}

static void test_video_defaults() {
    auto snapshot = build_default_preference_snapshot();
    assert(!snapshot.video.max_resolution.has_value());
    assert(snapshot.video.hdr_preference == "auto");
    std::cout << "  PASS: video_defaults\n";
}

int main() {
    std::cout << "preference_snapshot_test:\n";
    test_default_snapshot();
    test_subtitle_preference();
    test_video_defaults();
    std::cout << "ALL 3 TESTS PASSED\n";
    return 0;
}
