#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/color_science_core/fake_hdr_color_service.hpp"

using namespace kivo::cinema_engine;

// P2-040: HDR / Color Output Runtime tests

void test_hdr10_detection() {
    auto d = FakeHdrColorService::build_hdr10_detection();
    assert(d.metadata.schema_version == "1.1");
    assert(d.is_hdr10 == true);
    assert(d.is_hlg == false);
    assert(d.is_dolby_vision == false);
    assert(d.hdr10_max_luminance == 1000);
    assert(d.color_primaries == "bt2020");
    std::cout << "PASS: test_hdr10_detection" << std::endl;
}

void test_hlg_detection() {
    auto d = FakeHdrColorService::build_hlg_detection();
    assert(d.is_hlg == true);
    assert(d.is_hdr10 == false);
    assert(d.is_dolby_vision == false);
    assert(d.color_primaries == "bt2020");
    std::cout << "PASS: test_hlg_detection" << std::endl;
}

void test_dolby_vision_detection() {
    auto d = FakeHdrColorService::build_dolby_vision_detection();
    assert(d.is_dolby_vision == true);
    assert(d.is_hdr10 == true);  // DV backward-compatible
    assert(d.hdr10_max_luminance == 4000);
    std::cout << "PASS: test_dolby_vision_detection" << std::endl;
}

void test_sdr_detection() {
    auto d = FakeHdrColorService::build_sdr_detection();
    assert(d.is_hdr10 == false);
    assert(d.is_hlg == false);
    assert(d.is_dolby_vision == false);
    assert(d.color_primaries == "bt709");
    std::cout << "PASS: test_sdr_detection" << std::endl;
}

void test_sdr_color_space() {
    auto c = FakeHdrColorService::build_sdr_color_space();
    assert(c.color_space == "srgb");
    assert(c.transfer_function == "srgb");
    assert(c.primaries == "bt709");
    assert(c.is_hdr == false);
    std::cout << "PASS: test_sdr_color_space" << std::endl;
}

void test_hdr10_color_space() {
    auto c = FakeHdrColorService::build_hdr10_color_space();
    assert(c.color_space == "bt2020");
    assert(c.transfer_function == "pq");
    assert(c.is_hdr == true);
    assert(c.max_cll == 1000);
    assert(c.max_fall == 400);
    std::cout << "PASS: test_hdr10_color_space" << std::endl;
}

void test_valid_dolby_vision_claim() {
    auto v = FakeHdrColorService::build_valid_dolby_vision_claim();
    assert(v.claim_type == "dolby_vision");
    assert(v.is_valid == true);
    assert(v.failure_reason.empty());
    std::cout << "PASS: test_valid_dolby_vision_claim" << std::endl;
}

void test_false_dolby_vision_claim() {
    auto v = FakeHdrColorService::build_false_dolby_vision_claim();
    assert(v.claim_type == "dolby_vision");
    assert(v.is_valid == false);
    assert(!v.failure_reason.empty());
    std::cout << "PASS: test_false_dolby_vision_claim" << std::endl;
}

void test_hdr10_luminance_tracking() {
    auto d = FakeHdrColorService::build_hdr10_detection();
    assert(d.hdr10_max_luminance > 0);
    assert(d.hdr10_min_luminance >= 0);
    std::cout << "PASS: test_hdr10_luminance_tracking" << std::endl;
}

void test_transfer_function_tracking() {
    auto sdr = FakeHdrColorService::build_sdr_color_space();
    auto hdr = FakeHdrColorService::build_hdr10_color_space();
    assert(sdr.transfer_function == "srgb");
    assert(hdr.transfer_function == "pq");
    std::cout << "PASS: test_transfer_function_tracking" << std::endl;
}

int main() {
    // Detection tests
    test_hdr10_detection();
    test_hlg_detection();
    test_dolby_vision_detection();
    test_sdr_detection();

    // Color space tests
    test_sdr_color_space();
    test_hdr10_color_space();

    // Claim validation tests
    test_valid_dolby_vision_claim();
    test_false_dolby_vision_claim();

    // Utility tests
    test_hdr10_luminance_tracking();
    test_transfer_function_tracking();

    std::cout << "All P2-040 HDR / Color Output Runtime tests passed." << std::endl;
    return 0;
}
