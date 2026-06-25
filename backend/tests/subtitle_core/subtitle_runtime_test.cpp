#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/subtitle_core/fake_subtitle_runtime_service.hpp"

using namespace kivo::cinema_engine;

// P2-039: Subtitle Runtime tests

void test_srt_track_info() {
    auto t = FakeSubtitleRuntimeService::build_srt_track();
    assert(t.metadata.schema_version == "1.1");
    assert(t.track_id == "sub-track-001");
    assert(t.format == "srt");
    assert(t.language == "en");
    assert(t.is_default == true);
    assert(t.entry_count == 150);
    std::cout << "PASS: test_srt_track_info" << std::endl;
}

void test_ass_track_info() {
    auto t = FakeSubtitleRuntimeService::build_ass_track();
    assert(t.format == "ass");
    assert(t.language == "zh");
    assert(t.is_default == false);
    assert(t.entry_count == 200);
    std::cout << "PASS: test_ass_track_info" << std::endl;
}

void test_srt_entry() {
    auto e = FakeSubtitleRuntimeService::build_srt_entry();
    assert(e.entry_id == 1);
    assert(e.start_time_ms == 0);
    assert(e.end_time_ms == 5000);
    assert(e.text == "Hello, world!");
    assert(e.style.empty());
    std::cout << "PASS: test_srt_entry" << std::endl;
}

void test_ass_entry() {
    auto e = FakeSubtitleRuntimeService::build_ass_entry();
    assert(e.entry_id == 1);
    assert(e.start_time_ms == 1000);
    assert(e.end_time_ms == 4000);
    assert(e.text == "你好世界");
    assert(e.style == "Default");
    std::cout << "PASS: test_ass_entry" << std::endl;
}

void test_successful_srt_load() {
    auto r = FakeSubtitleRuntimeService::build_successful_srt_load();
    assert(r.success == true);
    assert(r.track_info.format == "srt");
    assert(r.entries.size() == 1);
    assert(r.error_message.empty());
    std::cout << "PASS: test_successful_srt_load" << std::endl;
}

void test_successful_ass_load() {
    auto r = FakeSubtitleRuntimeService::build_successful_ass_load();
    assert(r.success == true);
    assert(r.track_info.format == "ass");
    assert(r.entries.size() == 1);
    std::cout << "PASS: test_successful_ass_load" << std::endl;
}

void test_failed_load() {
    auto r = FakeSubtitleRuntimeService::build_failed_load();
    assert(r.success == false);
    assert(r.entries.empty());
    assert(!r.error_message.empty());
    std::cout << "PASS: test_failed_load" << std::endl;
}

void test_runtime_state_transitions() {
    auto unloaded = FakeSubtitleRuntimeService::build_unloaded_state();
    auto active = FakeSubtitleRuntimeService::build_active_state();
    assert(unloaded == SubtitleRuntimeState::Unloaded);
    assert(active == SubtitleRuntimeState::Active);
    assert(unloaded != active);
    std::cout << "PASS: test_runtime_state_transitions" << std::endl;
}

void test_entry_timing_validity() {
    auto e = FakeSubtitleRuntimeService::build_srt_entry();
    assert(e.end_time_ms > e.start_time_ms);
    std::cout << "PASS: test_entry_timing_validity" << std::endl;
}

void test_language_code_tracking() {
    auto srt = FakeSubtitleRuntimeService::build_srt_track();
    auto ass = FakeSubtitleRuntimeService::build_ass_track();
    assert(srt.language == "en");
    assert(ass.language == "zh");
    std::cout << "PASS: test_language_code_tracking" << std::endl;
}

void test_default_track_flag() {
    auto default_track = FakeSubtitleRuntimeService::build_srt_track();
    auto non_default_track = FakeSubtitleRuntimeService::build_ass_track();
    assert(default_track.is_default == true);
    assert(non_default_track.is_default == false);
    std::cout << "PASS: test_default_track_flag" << std::endl;
}

int main() {
    // Track info tests
    test_srt_track_info();
    test_ass_track_info();

    // Entry tests
    test_srt_entry();
    test_ass_entry();

    // Load result tests
    test_successful_srt_load();
    test_successful_ass_load();
    test_failed_load();

    // State and utility tests
    test_runtime_state_transitions();
    test_entry_timing_validity();
    test_language_code_tracking();
    test_default_track_flag();

    std::cout << "All P2-039 Subtitle Runtime tests passed." << std::endl;
    return 0;
}
