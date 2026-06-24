#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/capability_core/fake_playback_eligibility_service.hpp"

using namespace kivo::cinema_engine;

static void test_valid_result() {
    ContractMetadata metadata;

    auto result = build_fake_playback_eligibility(
        metadata,
        "static-cap-001",
        "device-cap-001",
        "session-cap-001",
        {"runtime-obs-001"},
        {},
        {},
        1.0);

    assert(result.metadata.schema_version == "1.1");
    assert(result.join_result_id.find("capability-join-") != std::string::npos);
    assert(result.static_capability_ref == "static-cap-001");
    assert(result.device_capability_ref == "device-cap-001");
    assert(result.session_capability_ref == "session-cap-001");
    assert(result.runtime_observation_refs.size() == 1);
    assert(result.confidence == 1.0);
    std::cout << "  PASS: valid_result\n";
}

static void test_conflicts_reduce_confidence() {
    ContractMetadata metadata;

    auto result = build_fake_playback_eligibility(
        metadata,
        "static-cap-001",
        "device-cap-001",
        "session-cap-001",
        {"runtime-obs-001"},
        {"codec-not-supported"},
        {},
        0.5);

    assert(result.conflicts.size() == 1);
    assert(result.conflicts[0] == "codec-not-supported");
    assert(result.confidence == 0.5);
    std::cout << "  PASS: conflicts_reduce_confidence\n";
}

static void test_missing_capabilities_zero_confidence() {
    ContractMetadata metadata;

    auto result = build_fake_playback_eligibility(
        metadata,
        "static-cap-001",
        "device-cap-001",
        "session-cap-001",
        {},
        {},
        {"gpu-decode", "audio-output"},
        0.0);

    assert(result.missing_capabilities.size() == 2);
    assert(result.confidence == 0.0);
    std::cout << "  PASS: missing_capabilities_zero_confidence\n";
}

int main() {
    std::cout << "playback_eligibility_test:\n";
    test_valid_result();
    test_conflicts_reduce_confidence();
    test_missing_capabilities_zero_confidence();
    std::cout << "ALL 3 TESTS PASSED\n";
    return 0;
}
