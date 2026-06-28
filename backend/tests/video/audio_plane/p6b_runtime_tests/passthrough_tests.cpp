// passthrough_tests.cpp — P6B runtime: fake passthrough parser tests
// Tests B22 (FakePassthroughParser).
// Uses try/catch to prevent abort popups (follows P5B test pattern).
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <type_traits>

#include "kivo/video/audio_plane/runtime/passthrough/fake_passthrough_parser.hpp"
#include "kivo/video/audio_plane/passthrough/encoded_access_unit.hpp"

namespace rt_passthrough = kivo::video::audio_plane::runtime::passthrough;
using kivo::video::audio_plane::passthrough::EncodedAccessUnit;

static int g_failed = 0;

#define P6B_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// ============================================================
// B22: FakePassthroughParser
// ============================================================

// B22.1: parse AC3 (codec_id=1) returns EncodedAccessUnit with correct codec.
static void test_b22_parse_ac3() {
    rt_passthrough::FakePassthroughParser parser;
    auto result = parser.parse(1001, 1, 0, 1024);
    if (result.is_error()) throw std::runtime_error("parse AC3 must succeed");
    auto& unit = result.value();
    if (unit.codec_id != 1) throw std::runtime_error("AC3 codec_id must be 1");
    if (unit.source_packet_id != 1001) throw std::runtime_error("source_packet_id must be 1001");
    if (unit.payload_handle_id == 0) throw std::runtime_error("payload_handle_id must be non-zero");
    if (unit.encoded_duration != 1024) throw std::runtime_error("encoded_duration must be 1024");
    if (!unit.sync_frame) throw std::runtime_error("sync_frame must be true (deterministic)");
    if (unit.discontinuity) throw std::runtime_error("discontinuity must be false");
    std::printf("[B22.1] FakePassthroughParser parse AC3 PASS\n");
}

// B22.2: parse EAC3 (codec_id=2).
static void test_b22_parse_eac3() {
    rt_passthrough::FakePassthroughParser parser;
    auto result = parser.parse(1002, 2, 1024, 2048);
    if (result.is_error()) throw std::runtime_error("parse EAC3 must succeed");
    auto& unit = result.value();
    if (unit.codec_id != 2) throw std::runtime_error("EAC3 codec_id must be 2");
    if (unit.source_packet_id != 1002) throw std::runtime_error("source_packet_id must be 1002");
    if (unit.media_start != 1024) throw std::runtime_error("media_start must be 1024");
    std::printf("[B22.2] FakePassthroughParser parse EAC3 PASS\n");
}

// B22.3: parse DTS (codec_id=3).
static void test_b22_parse_dts() {
    rt_passthrough::FakePassthroughParser parser;
    auto result = parser.parse(1003, 3, 2048, 3072);
    if (result.is_error()) throw std::runtime_error("parse DTS must succeed");
    auto& unit = result.value();
    if (unit.codec_id != 3) throw std::runtime_error("DTS codec_id must be 3");
    if (unit.source_packet_id != 1003) throw std::runtime_error("source_packet_id must be 1003");
    if (unit.media_start != 2048) throw std::runtime_error("media_start must be 2048");
    std::printf("[B22.3] FakePassthroughParser parse DTS PASS\n");
}

// B22.4: parse TrueHD (codec_id=4).
static void test_b22_parse_truehd() {
    rt_passthrough::FakePassthroughParser parser;
    auto result = parser.parse(1004, 4, 3072, 4096);
    if (result.is_error()) throw std::runtime_error("parse TrueHD must succeed");
    auto& unit = result.value();
    if (unit.codec_id != 4) throw std::runtime_error("TrueHD codec_id must be 4");
    if (unit.source_packet_id != 1004) throw std::runtime_error("source_packet_id must be 1004");
    if (unit.encoded_duration != 4096) throw std::runtime_error("encoded_duration must be 4096");
    std::printf("[B22.4] FakePassthroughParser parse TrueHD PASS\n");
}

// B22.5: parse invalid codec (codec_id=0) returns InvalidCodec.
static void test_b22_parse_invalid_codec_zero() {
    rt_passthrough::FakePassthroughParser parser;
    auto result = parser.parse(1005, 0, 0, 1024);
    if (!result.is_error()) throw std::runtime_error("parse codec_id=0 must fail");
    if (result.error() != rt_passthrough::ParseError::InvalidCodec) throw std::runtime_error("expected InvalidCodec");
    std::printf("[B22.5] FakePassthroughParser parse invalid codec 0 PASS\n");
}

// B22.6: parse out-of-range codec (codec_id=5) returns InvalidCodec.
static void test_b22_parse_invalid_codec_out_of_range() {
    rt_passthrough::FakePassthroughParser parser;
    auto result = parser.parse(1006, 5, 0, 1024);
    if (!result.is_error()) throw std::runtime_error("parse codec_id=5 must fail");
    if (result.error() != rt_passthrough::ParseError::InvalidCodec) throw std::runtime_error("expected InvalidCodec");
    std::printf("[B22.6] FakePassthroughParser parse out-of-range codec PASS\n");
}

// B22.7: parse negative codec (codec_id=-1) returns InvalidCodec.
static void test_b22_parse_negative_codec() {
    rt_passthrough::FakePassthroughParser parser;
    auto result = parser.parse(1007, -1, 0, 1024);
    if (!result.is_error()) throw std::runtime_error("parse codec_id=-1 must fail");
    if (result.error() != rt_passthrough::ParseError::InvalidCodec) throw std::runtime_error("expected InvalidCodec");
    std::printf("[B22.7] FakePassthroughParser parse negative codec PASS\n");
}

// B22.8: parse with packet_id=0 returns MissingPayload.
static void test_b22_parse_missing_payload() {
    rt_passthrough::FakePassthroughParser parser;
    auto result = parser.parse(0, 1, 0, 1024);
    if (!result.is_error()) throw std::runtime_error("parse packet_id=0 must fail");
    if (result.error() != rt_passthrough::ParseError::MissingPayload) throw std::runtime_error("expected MissingPayload");
    std::printf("[B22.8] FakePassthroughParser parse missing payload PASS\n");
}

// B22.9: payload_handle_id is uint64_t (NOT std::span — owned payload).
// Compile-time check: the field type is uint64_t.
static void test_b22_payload_handle_id_is_uint64() {
    static_assert(std::is_same_v<decltype(EncodedAccessUnit::payload_handle_id), uint64_t>,
                  "payload_handle_id must be uint64_t (owned, NOT span)");
    // Runtime check: each parse produces a non-zero owned handle.
    rt_passthrough::FakePassthroughParser parser;
    auto r1 = parser.parse(2001, 1, 0, 1024);
    auto r2 = parser.parse(2002, 2, 0, 1024);
    if (r1.is_error() || r2.is_error()) throw std::runtime_error("both parses must succeed");
    if (r1.value().payload_handle_id == 0) throw std::runtime_error("first payload_handle_id must be non-zero");
    if (r2.value().payload_handle_id == 0) throw std::runtime_error("second payload_handle_id must be non-zero");
    if (r1.value().payload_handle_id == r2.value().payload_handle_id) {
        throw std::runtime_error("payload_handle_id must be unique per parse");
    }
    std::printf("[B22.9] FakePassthroughParser payload_handle_id is uint64_t (owned) PASS\n");
}

// B22.10: payload_handle_id is monotonically increasing.
static void test_b22_payload_handle_monotonic() {
    rt_passthrough::FakePassthroughParser parser;
    auto r1 = parser.parse(3001, 1, 0, 1024);
    auto r2 = parser.parse(3002, 2, 0, 1024);
    auto r3 = parser.parse(3003, 3, 0, 1024);
    if (r1.is_error() || r2.is_error() || r3.is_error()) throw std::runtime_error("all parses must succeed");
    if (r1.value().payload_handle_id >= r2.value().payload_handle_id) throw std::runtime_error("handle must increase");
    if (r2.value().payload_handle_id >= r3.value().payload_handle_id) throw std::runtime_error("handle must increase");
    std::printf("[B22.10] FakePassthroughParser payload_handle monotonic PASS\n");
}

// B22.11: parsed_count tracks successful parses only.
static void test_b22_parsed_count_tracks() {
    rt_passthrough::FakePassthroughParser parser;
    if (parser.parsed_count() != 0) throw std::runtime_error("initial parsed_count must be 0");
    parser.parse(4001, 1, 0, 1024);  // success
    parser.parse(4002, 2, 0, 1024);  // success
    parser.parse(0, 1, 0, 1024);     // failure (MissingPayload)
    parser.parse(4003, 0, 0, 1024);  // failure (InvalidCodec)
    if (parser.parsed_count() != 2) throw std::runtime_error("parsed_count must be 2 (only successes)");
    std::printf("[B22.11] FakePassthroughParser parsed_count tracks successes PASS\n");
}

// B22.12: codec_id_from_enum maps FakeCodecId to int32_t correctly.
static void test_b22_codec_id_from_enum() {
    if (rt_passthrough::FakePassthroughParser::codec_id_from_enum(rt_passthrough::FakeCodecId::Unknown) != 0) throw std::runtime_error("Unknown must map to 0");
    if (rt_passthrough::FakePassthroughParser::codec_id_from_enum(rt_passthrough::FakeCodecId::AC3) != 1) throw std::runtime_error("AC3 must map to 1");
    if (rt_passthrough::FakePassthroughParser::codec_id_from_enum(rt_passthrough::FakeCodecId::EAC3) != 2) throw std::runtime_error("EAC3 must map to 2");
    if (rt_passthrough::FakePassthroughParser::codec_id_from_enum(rt_passthrough::FakeCodecId::DTS) != 3) throw std::runtime_error("DTS must map to 3");
    if (rt_passthrough::FakePassthroughParser::codec_id_from_enum(rt_passthrough::FakeCodecId::TrueHD) != 4) throw std::runtime_error("TrueHD must map to 4");
    std::printf("[B22.12] FakePassthroughParser codec_id_from_enum mapping PASS\n");
}

// B22.13: EncodedAccessUnit metadata populated by parse.
static void test_b22_metadata_populated() {
    rt_passthrough::FakePassthroughParser parser;
    auto result = parser.parse(5001, 1, 0, 1024);
    if (result.is_error()) throw std::runtime_error("parse must succeed");
    auto& unit = result.value();
    if (unit.metadata.family_name != "passthrough") throw std::runtime_error("family_name must be passthrough");
    if (unit.metadata.header_name != "encoded_access_unit") throw std::runtime_error("header_name must be encoded_access_unit");
    if (unit.metadata.classification != "P6B-runtime-fake") throw std::runtime_error("classification must be P6B-runtime-fake");
    if (unit.metadata.version != 1) throw std::runtime_error("version must be 1");
    std::printf("[B22.13] FakePassthroughParser metadata populated PASS\n");
}

// B22.14: time_base fields populated (1/48000 default).
static void test_b22_time_base_populated() {
    rt_passthrough::FakePassthroughParser parser;
    auto result = parser.parse(6001, 1, 0, 1024);
    if (result.is_error()) throw std::runtime_error("parse must succeed");
    auto& unit = result.value();
    if (unit.time_base_num != 1) throw std::runtime_error("time_base_num must be 1");
    if (unit.time_base_den != 48000) throw std::runtime_error("time_base_den must be 48000");
    std::printf("[B22.14] FakePassthroughParser time_base populated PASS\n");
}

// B22.15: validation_evidence_id is non-zero and unique per parse.
static void test_b22_validation_evidence_id_unique() {
    rt_passthrough::FakePassthroughParser parser;
    auto r1 = parser.parse(7001, 1, 0, 1024);
    auto r2 = parser.parse(7002, 2, 0, 1024);
    if (r1.is_error() || r2.is_error()) throw std::runtime_error("both parses must succeed");
    if (r1.value().validation_evidence_id == 0) throw std::runtime_error("validation_evidence_id must be non-zero");
    if (r1.value().validation_evidence_id == r2.value().validation_evidence_id) {
        throw std::runtime_error("validation_evidence_id must be unique per parse");
    }
    std::printf("[B22.15] FakePassthroughParser validation_evidence_id unique PASS\n");
}

// B22.16: boundary marker set (fake frame boundary).
static void test_b22_boundary_marker_set() {
    rt_passthrough::FakePassthroughParser parser;
    auto result = parser.parse(8001, 1, 0, 1024);
    if (result.is_error()) throw std::runtime_error("parse must succeed");
    auto& unit = result.value();
    if (unit.boundary != 1) throw std::runtime_error("boundary must be 1 (fake frame boundary marker)");
    std::printf("[B22.16] FakePassthroughParser boundary marker set PASS\n");
}

// B22.17: profile defaults to 0 (no profile detection in fake).
static void test_b22_profile_default_zero() {
    rt_passthrough::FakePassthroughParser parser;
    auto result = parser.parse(9001, 1, 0, 1024);
    if (result.is_error()) throw std::runtime_error("parse must succeed");
    if (result.value().profile != 0) throw std::runtime_error("profile must default to 0 in fake");
    std::printf("[B22.17] FakePassthroughParser profile default zero PASS\n");
}

int main() {
    std::printf("=== P6B Passthrough Runtime Tests (B22) ===\n\n");

    P6B_RUN(test_b22_parse_ac3);
    P6B_RUN(test_b22_parse_eac3);
    P6B_RUN(test_b22_parse_dts);
    P6B_RUN(test_b22_parse_truehd);
    P6B_RUN(test_b22_parse_invalid_codec_zero);
    P6B_RUN(test_b22_parse_invalid_codec_out_of_range);
    P6B_RUN(test_b22_parse_negative_codec);
    P6B_RUN(test_b22_parse_missing_payload);
    P6B_RUN(test_b22_payload_handle_id_is_uint64);
    P6B_RUN(test_b22_payload_handle_monotonic);
    P6B_RUN(test_b22_parsed_count_tracks);
    P6B_RUN(test_b22_codec_id_from_enum);
    P6B_RUN(test_b22_metadata_populated);
    P6B_RUN(test_b22_time_base_populated);
    P6B_RUN(test_b22_validation_evidence_id_unique);
    P6B_RUN(test_b22_boundary_marker_set);
    P6B_RUN(test_b22_profile_default_zero);

    if (g_failed == 0) {
        std::printf("\n=== P6B Passthrough Summary: all 17 exit criteria PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P6B Passthrough Summary: %d test(s) FAILED ===\n", g_failed);
        return 1;
    }
}
