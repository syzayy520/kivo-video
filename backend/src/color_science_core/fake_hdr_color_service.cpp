#include "kivo/cinema_engine/color_science_core/fake_hdr_color_service.hpp"

namespace kivo::cinema_engine {

// --- Detection builders ---

HdrDetectionResult FakeHdrColorService::build_hdr10_detection() {
    HdrDetectionResult d;
    d.metadata.schema_version = "1.1";
    d.detection_id = "hdr-detect-001";
    d.source_id = "source-movie-42";
    d.is_hdr10 = true;
    d.is_hlg = false;
    d.is_dolby_vision = false;
    d.hdr10_max_luminance = 1000;  // 1000 nits
    d.hdr10_min_luminance = 0;
    d.color_primaries = "bt2020";
    return d;
}

HdrDetectionResult FakeHdrColorService::build_hlg_detection() {
    HdrDetectionResult d;
    d.metadata.schema_version = "1.1";
    d.detection_id = "hdr-detect-002";
    d.source_id = "source-tv-show-01";
    d.is_hdr10 = false;
    d.is_hlg = true;
    d.is_dolby_vision = false;
    d.hdr10_max_luminance = 0;
    d.hdr10_min_luminance = 0;
    d.color_primaries = "bt2020";
    return d;
}

HdrDetectionResult FakeHdrColorService::build_dolby_vision_detection() {
    HdrDetectionResult d;
    d.metadata.schema_version = "1.1";
    d.detection_id = "hdr-detect-003";
    d.source_id = "source-movie-dv-01";
    d.is_hdr10 = true;  // DV backward-compatible with HDR10
    d.is_hlg = false;
    d.is_dolby_vision = true;
    d.hdr10_max_luminance = 4000;  // 4000 nits peak
    d.hdr10_min_luminance = 0;
    d.color_primaries = "bt2020";
    return d;
}

HdrDetectionResult FakeHdrColorService::build_sdr_detection() {
    HdrDetectionResult d;
    d.metadata.schema_version = "1.1";
    d.detection_id = "hdr-detect-004";
    d.source_id = "source-movie-sdr-01";
    d.is_hdr10 = false;
    d.is_hlg = false;
    d.is_dolby_vision = false;
    d.hdr10_max_luminance = 0;
    d.hdr10_min_luminance = 0;
    d.color_primaries = "bt709";
    return d;
}

// --- Color space builders ---

ColorSpaceDescriptor FakeHdrColorService::build_sdr_color_space() {
    ColorSpaceDescriptor c;
    c.metadata.schema_version = "1.1";
    c.descriptor_id = "color-space-001";
    c.color_space = "srgb";
    c.transfer_function = "srgb";
    c.primaries = "bt709";
    c.is_hdr = false;
    c.max_cll = 0;
    c.max_fall = 0;
    return c;
}

ColorSpaceDescriptor FakeHdrColorService::build_hdr10_color_space() {
    ColorSpaceDescriptor c;
    c.metadata.schema_version = "1.1";
    c.descriptor_id = "color-space-002";
    c.color_space = "bt2020";
    c.transfer_function = "pq";
    c.primaries = "bt2020";
    c.is_hdr = true;
    c.max_cll = 1000;
    c.max_fall = 400;
    return c;
}

// --- Claim validation builders ---

HdrClaimValidation FakeHdrColorService::build_valid_dolby_vision_claim() {
    HdrClaimValidation v;
    v.metadata.schema_version = "1.1";
    v.validation_id = "claim-valid-001";
    v.claim_type = "dolby_vision";
    v.is_valid = true;
    v.evidence_source = "dv_rpu_metadata_parser";
    v.failure_reason = "";
    return v;
}

HdrClaimValidation FakeHdrColorService::build_false_dolby_vision_claim() {
    HdrClaimValidation v;
    v.metadata.schema_version = "1.1";
    v.validation_id = "claim-false-001";
    v.claim_type = "dolby_vision";
    v.is_valid = false;
    v.evidence_source = "dv_rpu_metadata_parser";
    v.failure_reason = "no Dolby Vision RPU metadata found in elementary stream";
    return v;
}

}  // namespace kivo::cinema_engine
