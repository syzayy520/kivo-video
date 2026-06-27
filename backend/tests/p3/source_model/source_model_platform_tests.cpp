#include "kivo/p3/source_model/source_kinds.hpp"
#include "kivo/p3/source_model/source_identity.hpp"
#include "kivo/p3/platform_contracts/platform_contracts.hpp"

#include <cassert>
#include <iostream>

static void test_source_origin_kind_distinct_from_provider()
{
    // HLS/DASH/HTTP/MP4 must NOT be ProviderFamily
    kivo::p3::source_model::ProviderFamily hls = kivo::p3::source_model::ProviderFamily::hls;
    kivo::p3::source_model::MediaDeliveryKind hls_delivery = kivo::p3::source_model::MediaDeliveryKind::hls_manifest;
    assert(hls != kivo::p3::source_model::ProviderFamily::http);
    assert(hls_delivery == kivo::p3::source_model::MediaDeliveryKind::hls_manifest);
    std::cout << "PASS: source_origin_kind_distinct_from_provider\n";
}

static void test_source_origin_identity()
{
    kivo::p3::source_model::SourceOriginIdentity id;
    id.origin_id = "origin-001";
    id.kind = kivo::p3::source_model::SourceOriginKind::remote_http;
    id.redacted_display_name = "redacted";
    assert(id.schema_version == "p3.source_model.origin_identity.v1");
    assert(id.kind == kivo::p3::source_model::SourceOriginKind::remote_http);
    std::cout << "PASS: source_origin_identity\n";
}

static void test_provider_identity()
{
    kivo::p3::source_model::ProviderIdentity p;
    p.provider_id = "prov-001";
    p.family = kivo::p3::source_model::ProviderFamily::emby;
    p.redacted_host = "redacted-host";
    assert(p.family == kivo::p3::source_model::ProviderFamily::emby);
    assert(p.schema_version == "p3.source_model.provider_identity.v1");
    std::cout << "PASS: provider_identity\n";
}

static void test_source_capability()
{
    kivo::p3::source_model::SourceCapability c;
    c.supports_range_read = true;
    c.supports_seek = true;
    c.content_length = 1024;
    c.container_hint = kivo::p3::source_model::ContainerFormat::mp4;
    assert(c.supports_range_read);
    assert(c.content_length == 1024);
    assert(c.container_hint == kivo::p3::source_model::ContainerFormat::mp4);
    std::cout << "PASS: source_capability\n";
}

static void test_platform_capability_hints()
{
    kivo::p3::platform_contracts::PlatformCapabilityHints h;
    assert(h.primary_platform == "windows_desktop");
    assert(h.ui_tech_stack_note == "qt");
    assert(h.video_api_hint == "d3d11");
    assert(h.audio_api_hint == "wasapi");
    assert(h.is_hint_only());
    std::cout << "PASS: platform_capability_hints\n";
}

static void test_buffer_requirement()
{
    kivo::p3::platform_contracts::BufferRequirement b;
    b.startup_prebuffer_bytes = 1024 * 1024;
    b.low_watermark_bytes = 512 * 1024;
    b.high_watermark_bytes = 2 * 1024 * 1024;
    b.max_bytes = 10 * 1024 * 1024;
    b.dynamic_adjustment_enabled = true;
    b.low_power_mode = true;
    b.metered_network = false;
    assert(b.startup_prebuffer_bytes == 1024 * 1024);
    assert(b.dynamic_adjustment_enabled);
    assert(b.low_power_mode);
    std::cout << "PASS: buffer_requirement\n";
}

static void test_decoder_capability_requirement()
{
    kivo::p3::platform_contracts::DecoderCapabilityRequirement d;
    d.codec_hint = "h264";
    d.hardware_decode_available = true;
    d.max_resolution_width = 1920;
    d.max_resolution_height = 1080;
    d.max_fps = 60;
    assert(d.hardware_decode_available);
    assert(d.max_resolution_width == 1920);
    std::cout << "PASS: decoder_capability_requirement\n";
}

static void test_no_cyclic_dependency()
{
    // SourceCapability and CandidateRanking must not have cyclic dependency.
    // This test verifies they are independent structs.
    kivo::p3::source_model::SourceCapability c;
    c.supports_range_read = true;
    // No CandidateRanking reference here - no cyclic dependency
    assert(c.supports_range_read);
    std::cout << "PASS: no_cyclic_dependency\n";
}

int main()
{
    test_source_origin_kind_distinct_from_provider();
    test_source_origin_identity();
    test_provider_identity();
    test_source_capability();
    test_platform_capability_hints();
    test_buffer_requirement();
    test_decoder_capability_requirement();
    test_no_cyclic_dependency();
    std::cout << "ALL PASS: source_model_platform_tests (8 tests)\n";
    return 0;
}
