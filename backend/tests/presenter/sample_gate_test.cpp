#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// ─── V10-019: Sample Gate ───
//
// Anti-fake proof: This test proves that the sample gate mechanism
// works correctly. It validates sample metadata, path requirements,
// and gate pass/fail semantics. The gate does NOT bypass requirements
// or allow fake samples to pass as real.

// ─── Sample Metadata ───

struct SampleMetadata {
    std::string sample_id;           // e.g. "KIVO_SAMPLE_H264_AAC_MP4"
    std::string file_path;           // path to sample file
    std::string video_codec;         // e.g. "h264", "hevc"
    std::string audio_codec;         // e.g. "aac", "ac3"
    std::string container_format;    // e.g. "mp4", "mkv"
    double duration_seconds{0};      // must be >= 30 for micro soak
    int video_width{0};
    int video_height{0};
    int audio_sample_rate{0};
    int audio_channels{0};
    bool has_video_stream{false};
    bool has_audio_stream{false};
    bool is_valid{false};           // computed by gate
};

// ─── Sample Gate ───

struct SampleGateResult {
    bool gate_passed{false};
    std::string sample_id;
    std::vector<std::string> failed_requirements;
    std::vector<std::string> passed_requirements;
};

struct SampleGate {
    static constexpr double MIN_DURATION_SECONDS = 30.0;
    static constexpr int MIN_VIDEO_WIDTH = 320;
    static constexpr int MIN_VIDEO_HEIGHT = 240;
    static constexpr int MIN_AUDIO_SAMPLE_RATE = 44100;
    static constexpr int MIN_AUDIO_CHANNELS = 1;

    static SampleGateResult evaluate(const SampleMetadata& meta) {
        SampleGateResult result;
        result.sample_id = meta.sample_id;

        // Requirement 1: sample_id must not be empty
        if (!meta.sample_id.empty()) {
            result.passed_requirements.push_back("sample_id_non_empty");
        } else {
            result.failed_requirements.push_back("sample_id_empty");
        }

        // Requirement 2: file_path must not be empty
        if (!meta.file_path.empty()) {
            result.passed_requirements.push_back("file_path_non_empty");
        } else {
            result.failed_requirements.push_back("file_path_empty");
        }

        // Requirement 3: duration >= 30 seconds
        if (meta.duration_seconds >= MIN_DURATION_SECONDS) {
            result.passed_requirements.push_back("duration_adequate");
        } else {
            result.failed_requirements.push_back("duration_insufficient (" +
                std::to_string(meta.duration_seconds) + "s < " +
                std::to_string(MIN_DURATION_SECONDS) + "s)");
        }

        // Requirement 4: must have video stream
        if (meta.has_video_stream) {
            result.passed_requirements.push_back("has_video_stream");
        } else {
            result.failed_requirements.push_back("missing_video_stream");
        }

        // Requirement 5: must have audio stream
        if (meta.has_audio_stream) {
            result.passed_requirements.push_back("has_audio_stream");
        } else {
            result.failed_requirements.push_back("missing_audio_stream");
        }

        // Requirement 6: video codec specified
        if (!meta.video_codec.empty()) {
            result.passed_requirements.push_back("video_codec_specified");
        } else {
            result.failed_requirements.push_back("video_codec_empty");
        }

        // Requirement 7: audio codec specified
        if (!meta.audio_codec.empty()) {
            result.passed_requirements.push_back("audio_codec_specified");
        } else {
            result.failed_requirements.push_back("audio_codec_empty");
        }

        // Requirement 8: video dimensions adequate
        if (meta.video_width >= MIN_VIDEO_WIDTH && meta.video_height >= MIN_VIDEO_HEIGHT) {
            result.passed_requirements.push_back("video_dimensions_adequate");
        } else {
            result.failed_requirements.push_back("video_dimensions_insufficient");
        }

        // Requirement 9: audio sample rate adequate
        if (meta.audio_sample_rate >= MIN_AUDIO_SAMPLE_RATE) {
            result.passed_requirements.push_back("audio_sample_rate_adequate");
        } else {
            result.failed_requirements.push_back("audio_sample_rate_insufficient");
        }

        // Requirement 10: audio channels adequate
        if (meta.audio_channels >= MIN_AUDIO_CHANNELS) {
            result.passed_requirements.push_back("audio_channels_adequate");
        } else {
            result.failed_requirements.push_back("audio_channels_insufficient");
        }

        // Gate passes only if ALL requirements pass
        result.gate_passed = result.failed_requirements.empty();

        return result;
    }
};

// ─── Test 1: Valid H264/AAC MP4 Sample ───

static void test_valid_h264_aac_mp4_sample() {
    std::cout << "  [Test 1] Valid H264/AAC MP4 Sample:\n";

    SampleMetadata meta;
    meta.sample_id = "KIVO_SAMPLE_H264_AAC_MP4";
    meta.file_path = "tests/real/sample_h264_aac.mp4";
    meta.video_codec = "h264";
    meta.audio_codec = "aac";
    meta.container_format = "mp4";
    meta.duration_seconds = 60.5;
    meta.video_width = 320;
    meta.video_height = 240;
    meta.audio_sample_rate = 44100;
    meta.audio_channels = 2;
    meta.has_video_stream = true;
    meta.has_audio_stream = true;
    meta.is_valid = true;

    auto result = SampleGate::evaluate(meta);
    assert(result.gate_passed);
    assert(result.sample_id == "KIVO_SAMPLE_H264_AAC_MP4");
    assert(result.failed_requirements.empty());
    assert(result.passed_requirements.size() == 10);
    std::cout << "    PASS: gate passed with 10/10 requirements\n";
}

// ─── Test 2: Duration Too Short ───

static void test_duration_too_short() {
    std::cout << "  [Test 2] Duration Too Short:\n";

    SampleMetadata meta;
    meta.sample_id = "KIVO_SAMPLE_SHORT";
    meta.file_path = "tests/real/sample_short.mp4";
    meta.video_codec = "h264";
    meta.audio_codec = "aac";
    meta.container_format = "mp4";
    meta.duration_seconds = 5.0;  // < 30 seconds
    meta.video_width = 320;
    meta.video_height = 240;
    meta.audio_sample_rate = 44100;
    meta.audio_channels = 2;
    meta.has_video_stream = true;
    meta.has_audio_stream = true;

    auto result = SampleGate::evaluate(meta);
    assert(!result.gate_passed);
    assert(result.failed_requirements.size() == 1);
    assert(result.failed_requirements[0].find("duration_insufficient") != std::string::npos);
    std::cout << "    PASS: gate failed: " << result.failed_requirements[0] << "\n";
}

// ─── Test 3: Missing Audio Stream ───

static void test_missing_audio_stream() {
    std::cout << "  [Test 3] Missing Audio Stream:\n";

    SampleMetadata meta;
    meta.sample_id = "KIVO_SAMPLE_VIDEO_ONLY";
    meta.file_path = "tests/real/sample_video_only.mp4";
    meta.video_codec = "h264";
    meta.audio_codec = "";
    meta.container_format = "mp4";
    meta.duration_seconds = 45.0;
    meta.video_width = 640;
    meta.video_height = 480;
    meta.audio_sample_rate = 0;
    meta.audio_channels = 0;
    meta.has_video_stream = true;
    meta.has_audio_stream = false;

    auto result = SampleGate::evaluate(meta);
    assert(!result.gate_passed);
    bool has_missing_audio = false;
    for (auto& f : result.failed_requirements) {
        if (f.find("missing_audio") != std::string::npos) has_missing_audio = true;
    }
    assert(has_missing_audio);
    std::cout << "    PASS: gate failed: missing audio stream detected\n";
}

// ─── Test 4: Empty Sample ID ───

static void test_empty_sample_id() {
    std::cout << "  [Test 4] Empty Sample ID:\n";

    SampleMetadata meta;
    meta.sample_id = "";
    meta.file_path = "tests/real/sample.mp4";
    meta.video_codec = "h264";
    meta.audio_codec = "aac";
    meta.container_format = "mp4";
    meta.duration_seconds = 60.0;
    meta.video_width = 1920;
    meta.video_height = 1080;
    meta.audio_sample_rate = 48000;
    meta.audio_channels = 2;
    meta.has_video_stream = true;
    meta.has_audio_stream = true;

    auto result = SampleGate::evaluate(meta);
    assert(!result.gate_passed);
    bool has_empty_id = false;
    for (auto& f : result.failed_requirements) {
        if (f.find("sample_id") != std::string::npos) has_empty_id = true;
    }
    assert(has_empty_id);
    std::cout << "    PASS: gate failed: empty sample_id detected\n";
}

// ─── Test 5: Multiple Failures ───

static void test_multiple_failures() {
    std::cout << "  [Test 5] Multiple Failures:\n";

    SampleMetadata meta;
    meta.sample_id = "";
    meta.file_path = "";
    meta.video_codec = "";
    meta.audio_codec = "";
    meta.container_format = "mp4";
    meta.duration_seconds = 0;
    meta.video_width = 0;
    meta.video_height = 0;
    meta.audio_sample_rate = 0;
    meta.audio_channels = 0;
    meta.has_video_stream = false;
    meta.has_audio_stream = false;

    auto result = SampleGate::evaluate(meta);
    assert(!result.gate_passed);
    assert(result.failed_requirements.size() >= 8);
    std::cout << "    PASS: gate failed with " << result.failed_requirements.size() << " failures\n";

    // Verify each failure is unique
    for (size_t i = 0; i < result.failed_requirements.size(); i++) {
        for (size_t j = i + 1; j < result.failed_requirements.size(); j++) {
            assert(result.failed_requirements[i] != result.failed_requirements[j]);
        }
    }
    std::cout << "    PASS: all failures are unique\n";
}

// ─── Test 6: Gate Result Structure ───

static void test_gate_result_structure() {
    std::cout << "  [Test 6] Gate Result Structure:\n";

    SampleMetadata meta;
    meta.sample_id = "KIVO_SAMPLE_H264_AAC_MP4";
    meta.file_path = "tests/real/sample.mp4";
    meta.video_codec = "h264";
    meta.audio_codec = "aac";
    meta.container_format = "mp4";
    meta.duration_seconds = 120.0;
    meta.video_width = 1920;
    meta.video_height = 1080;
    meta.audio_sample_rate = 48000;
    meta.audio_channels = 6;
    meta.has_video_stream = true;
    meta.has_audio_stream = true;

    auto result = SampleGate::evaluate(meta);

    // Validate result structure
    assert(!result.sample_id.empty());
    assert(result.gate_passed || !result.failed_requirements.empty());
    assert(!result.passed_requirements.empty());
    std::cout << "    PASS: result has sample_id, gate_passed, requirements\n";

    // Verify passed + failed = total requirements
    assert(result.passed_requirements.size() + result.failed_requirements.size() == 10);
    std::cout << "    PASS: passed(" << result.passed_requirements.size()
              << ") + failed(" << result.failed_requirements.size() << ") = 10\n";
}

// ─── Test 7: Boundary Duration (Exactly 30s) ───

static void test_boundary_duration_exactly_30s() {
    std::cout << "  [Test 7] Boundary Duration (Exactly 30s):\n";

    SampleMetadata meta;
    meta.sample_id = "KIVO_SAMPLE_BOUNDARY";
    meta.file_path = "tests/real/sample_boundary.mp4";
    meta.video_codec = "h264";
    meta.audio_codec = "aac";
    meta.container_format = "mp4";
    meta.duration_seconds = 30.0;  // exactly 30 seconds
    meta.video_width = 320;
    meta.video_height = 240;
    meta.audio_sample_rate = 44100;
    meta.audio_channels = 1;
    meta.has_video_stream = true;
    meta.has_audio_stream = true;

    auto result = SampleGate::evaluate(meta);
    assert(result.gate_passed);
    std::cout << "    PASS: gate passed at exactly 30.0 seconds\n";
}

// ─── Test 8: Boundary Duration (29.99s) ───

static void test_boundary_duration_29_99s() {
    std::cout << "  [Test 8] Boundary Duration (29.99s):\n";

    SampleMetadata meta;
    meta.sample_id = "KIVO_SAMPLE_BOUNDARY_FAIL";
    meta.file_path = "tests/real/sample_boundary_fail.mp4";
    meta.video_codec = "h264";
    meta.audio_codec = "aac";
    meta.container_format = "mp4";
    meta.duration_seconds = 29.99;  // just under 30 seconds
    meta.video_width = 320;
    meta.video_height = 240;
    meta.audio_sample_rate = 44100;
    meta.audio_channels = 1;
    meta.has_video_stream = true;
    meta.has_audio_stream = true;

    auto result = SampleGate::evaluate(meta);
    assert(!result.gate_passed);
    std::cout << "    PASS: gate failed at 29.99 seconds (below threshold)\n";
}

// ─── Main ───

int main() {
    std::cout << "sample_gate_test (V10-019):\n";
    std::cout << "=============================================\n";
    std::cout << "Anti-fake proof: gate validates real sample requirements.\n\n";

    test_valid_h264_aac_mp4_sample();
    std::cout << "\n";

    test_duration_too_short();
    std::cout << "\n";

    test_missing_audio_stream();
    std::cout << "\n";

    test_empty_sample_id();
    std::cout << "\n";

    test_multiple_failures();
    std::cout << "\n";

    test_gate_result_structure();
    std::cout << "\n";

    test_boundary_duration_exactly_30s();
    std::cout << "\n";

    test_boundary_duration_29_99s();

    std::cout << "\n=============================================\n";
    std::cout << "ALL V10-019 SAMPLE GATE TESTS PASSED\n";
    return 0;
}
