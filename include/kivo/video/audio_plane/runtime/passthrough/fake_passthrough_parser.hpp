// fake_passthrough_parser.hpp — P6B runtime: fake passthrough parser
// HEADER-ONLY fake implementation. Platform-neutral: no WASAPI/FFmpeg/Qt/Win32/COM.
// FAKE ADAPTER: NOT P6E runtime. No real IEC61937, no real S/PDIF.
#pragma once

#include <cstdint>
#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/passthrough/encoded_access_unit.hpp"

namespace kivo::video::audio_plane::runtime::passthrough {

using kivo::video::audio_plane::passthrough::EncodedAccessUnit;

// ParseError: why parsing failed.
enum class ParseError : int32_t {
    InvalidCodec = 0,
    MissingPayload = 1,
    UnsupportedFormat = 2,
};

// FakeCodecId: fake codec enumeration (no real codec detection).
// Values 1/2/3/4 map to AC3/EAC3/DTS/TrueHD. 0 = invalid/unknown.
enum class FakeCodecId : int32_t {
    Unknown = 0,
    AC3 = 1,
    EAC3 = 2,
    DTS = 3,
    TrueHD = 4,
};

// FakePassthroughParser
// Deterministic fake of the passthrough parser. Produces EncodedAccessUnit
// with owned payload only (payload_handle_id is a uint64_t fake handle,
// NOT std::span). No real S/PDIF header extraction, no real IEC61937 framing.
// This is a FAKE ADAPTER, NOT P6E: no real passthrough runtime.
class FakePassthroughParser {
public:
    FakePassthroughParser() = default;

    // Parse a packet into an EncodedAccessUnit.
    // - packet_id: source packet identity
    // - codec_id: fake codec (1=AC3, 2=EAC3, 3=DTS, 4=TrueHD; 0=invalid)
    // - pts: presentation timestamp
    // - duration: encoded duration
    // Returns EncodedAccessUnit with a fake owned payload handle, or ParseError.
    kivo::Expected<EncodedAccessUnit, ParseError> parse(
        uint64_t packet_id,
        int32_t codec_id,
        int64_t pts,
        int64_t duration) {
        if (codec_id < 1 || codec_id > 4) {
            return ParseError::InvalidCodec;
        }
        if (packet_id == 0) {
            return ParseError::MissingPayload;
        }

        EncodedAccessUnit unit{};
        unit.metadata.family_name = "passthrough";
        unit.metadata.header_name = "encoded_access_unit";
        unit.metadata.classification = "P6B-runtime-fake";
        unit.metadata.version = 1;

        unit.codec_id = codec_id;
        unit.profile = 0;
        unit.source_packet_id = packet_id;
        unit.media_start = pts;
        unit.encoded_duration = duration;
        unit.boundary = 1;  // fake: frame boundary marker
        unit.time_base_num = 1;
        unit.time_base_den = 48000;
        unit.sync_frame = true;        // fake: deterministic sync
        unit.discontinuity = false;
        unit.payload_handle_id = allocate_fake_handle();  // owned, NOT span
        unit.validation_evidence_id = ++evidence_counter_;

        ++parsed_count_;
        return unit;
    }

    // Map a FakeCodecId to the int32_t codec_id used by parse().
    static int32_t codec_id_from_enum(FakeCodecId c) noexcept {
        return static_cast<int32_t>(c);
    }

    uint64_t parsed_count() const noexcept { return parsed_count_; }
    uint64_t next_payload_handle() const noexcept { return next_payload_handle_; }

private:
    // Allocate a fake owned payload handle (uint64_t, NOT std::span).
    // Deterministic: monotonically increasing. No real buffer is allocated.
    uint64_t allocate_fake_handle() noexcept {
        return next_payload_handle_++;
    }

    uint64_t next_payload_handle_{1};
    uint64_t evidence_counter_{0};
    uint64_t parsed_count_{0};
};

}  // namespace kivo::video::audio_plane::runtime::passthrough
