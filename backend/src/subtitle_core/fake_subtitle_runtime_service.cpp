#include "kivo/cinema_engine/subtitle_core/fake_subtitle_runtime_service.hpp"

namespace kivo::cinema_engine {

// --- Track info builders ---

SubtitleTrackInfo FakeSubtitleRuntimeService::build_srt_track() {
    SubtitleTrackInfo t;
    t.metadata.schema_version = "1.1";
    t.track_id = "sub-track-001";
    t.file_path = "/movies/test.en.srt";
    t.format = "srt";
    t.language = "en";
    t.is_default = true;
    t.entry_count = 150;
    return t;
}

SubtitleTrackInfo FakeSubtitleRuntimeService::build_ass_track() {
    SubtitleTrackInfo t;
    t.metadata.schema_version = "1.1";
    t.track_id = "sub-track-002";
    t.file_path = "/movies/test.zh.ass";
    t.format = "ass";
    t.language = "zh";
    t.is_default = false;
    t.entry_count = 200;
    return t;
}

// --- Entry builders ---

SubtitleEntry FakeSubtitleRuntimeService::build_srt_entry() {
    SubtitleEntry e;
    e.metadata.schema_version = "1.1";
    e.entry_id = 1;
    e.start_time_ms = 0;
    e.end_time_ms = 5000;  // 5 seconds
    e.text = "Hello, world!";
    e.style = "";
    return e;
}

SubtitleEntry FakeSubtitleRuntimeService::build_ass_entry() {
    SubtitleEntry e;
    e.metadata.schema_version = "1.1";
    e.entry_id = 1;
    e.start_time_ms = 1000;
    e.end_time_ms = 4000;  // 3 seconds
    e.text = "你好世界";
    e.style = "Default";
    return e;
}

// --- Load result builders ---

SubtitleLoadResult FakeSubtitleRuntimeService::build_successful_srt_load() {
    SubtitleLoadResult r;
    r.metadata.schema_version = "1.1";
    r.success = true;
    r.track_info = build_srt_track();
    r.entries = {build_srt_entry()};
    r.error_message = "";
    return r;
}

SubtitleLoadResult FakeSubtitleRuntimeService::build_successful_ass_load() {
    SubtitleLoadResult r;
    r.metadata.schema_version = "1.1";
    r.success = true;
    r.track_info = build_ass_track();
    r.entries = {build_ass_entry()};
    r.error_message = "";
    return r;
}

SubtitleLoadResult FakeSubtitleRuntimeService::build_failed_load() {
    SubtitleLoadResult r;
    r.metadata.schema_version = "1.1";
    r.success = false;
    r.track_info = {};
    r.entries = {};
    r.error_message = "failed to parse subtitle file: invalid format";
    return r;
}

// --- State builders ---

SubtitleRuntimeState FakeSubtitleRuntimeService::build_unloaded_state() {
    return SubtitleRuntimeState::Unloaded;
}

SubtitleRuntimeState FakeSubtitleRuntimeService::build_active_state() {
    return SubtitleRuntimeState::Active;
}

}  // namespace kivo::cinema_engine
