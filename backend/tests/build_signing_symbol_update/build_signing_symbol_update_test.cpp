#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/build_core/fake_build_config_service.hpp"
#include "kivo/cinema_engine/signing_core/fake_signing_service.hpp"
#include "kivo/cinema_engine/symbol_core/fake_symbol_service.hpp"
#include "kivo/cinema_engine/update_core/fake_update_service.hpp"

namespace {

using namespace kivo::cinema_engine;

void test_release_build_config() {
    auto policy = FakeBuildConfigService::build_release_build_config();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "release-build-001");
    assert(policy.build_type == "Release");
    assert(policy.reproducible_build == true);
    assert(policy.deterministic_timestamps == true);
    assert(policy.strip_debug_info == true);
    assert(policy.max_build_time_seconds == 600);
}

void test_debug_build_config() {
    auto policy = FakeBuildConfigService::build_debug_build_config();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "debug-build-001");
    assert(policy.build_type == "Debug");
    assert(policy.reproducible_build == false);
    assert(policy.deterministic_timestamps == false);
    assert(policy.strip_debug_info == false);
    assert(policy.max_build_time_seconds == 1200);
}

void test_reproducible_build_config() {
    auto policy = FakeBuildConfigService::build_reproducible_build_config();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "reproducible-build-001");
    assert(policy.build_type == "RelWithDebInfo");
    assert(policy.reproducible_build == true);
    assert(policy.deterministic_timestamps == true);
    assert(policy.strip_debug_info == false);
    assert(policy.max_build_time_seconds == 900);
}

void test_strict_signing_policy() {
    auto policy = FakeSigningService::build_strict_signing_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "strict-signing-001");
    assert(policy.signing_required == true);
    assert(policy.certificate_validation == true);
    assert(policy.timestamp_signing == true);
    assert(policy.allowed_signers.size() == 2);
    assert(policy.allowed_signers[0] == "kivo-video");
    assert(policy.allowed_signers[1] == "kivo-official");
    assert(policy.revocation_check == true);
}

void test_relaxed_signing_policy() {
    auto policy = FakeSigningService::build_relaxed_signing_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "relaxed-signing-001");
    assert(policy.signing_required == true);
    assert(policy.certificate_validation == false);
    assert(policy.timestamp_signing == false);
    assert(policy.allowed_signers.size() == 1);
    assert(policy.allowed_signers[0] == "kivo-video");
    assert(policy.revocation_check == false);
}

void test_no_signing_policy() {
    auto policy = FakeSigningService::build_no_signing_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "no-signing-001");
    assert(policy.signing_required == false);
    assert(policy.certificate_validation == false);
    assert(policy.timestamp_signing == false);
    assert(policy.allowed_signers.empty());
    assert(policy.revocation_check == false);
}

void test_full_symbol_policy() {
    auto policy = FakeSymbolService::build_full_symbol_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "full-symbol-001");
    assert(policy.symbol_server_enabled == true);
    assert(policy.symbol_upload_required == true);
    assert(policy.retention_days == 365);
    assert(policy.compression_enabled == true);
    assert(policy.public_symbols_only == false);
}

void test_public_only_symbol_policy() {
    auto policy = FakeSymbolService::build_public_only_symbol_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "public-only-symbol-001");
    assert(policy.symbol_server_enabled == true);
    assert(policy.symbol_upload_required == true);
    assert(policy.retention_days == 180);
    assert(policy.compression_enabled == true);
    assert(policy.public_symbols_only == true);
}

void test_minimal_symbol_policy() {
    auto policy = FakeSymbolService::build_minimal_symbol_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "minimal-symbol-001");
    assert(policy.symbol_server_enabled == false);
    assert(policy.symbol_upload_required == false);
    assert(policy.retention_days == 30);
    assert(policy.compression_enabled == false);
    assert(policy.public_symbols_only == true);
}

void test_strict_update_policy() {
    auto policy = FakeUpdateService::build_strict_update_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "strict-update-001");
    assert(policy.auto_update_enabled == true);
    assert(policy.signature_verification == true);
    assert(policy.rollback_enabled == true);
    assert(policy.max_update_size_mb == 50);
    assert(policy.staged_rollout == false);
}

void test_staged_update_policy() {
    auto policy = FakeUpdateService::build_staged_update_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "staged-update-001");
    assert(policy.auto_update_enabled == true);
    assert(policy.signature_verification == true);
    assert(policy.rollback_enabled == true);
    assert(policy.max_update_size_mb == 200);
    assert(policy.staged_rollout == true);
}

void test_manual_update_policy() {
    auto policy = FakeUpdateService::build_manual_update_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "manual-update-001");
    assert(policy.auto_update_enabled == false);
    assert(policy.signature_verification == true);
    assert(policy.rollback_enabled == false);
    assert(policy.max_update_size_mb == 500);
    assert(policy.staged_rollout == false);
}

}  // namespace

int main() {
    std::cout << "Running P2-052 Build / Signing / Symbol / Update tests..." << std::endl;

    test_release_build_config();
    std::cout << "PASS: test_release_build_config" << std::endl;

    test_debug_build_config();
    std::cout << "PASS: test_debug_build_config" << std::endl;

    test_reproducible_build_config();
    std::cout << "PASS: test_reproducible_build_config" << std::endl;

    test_strict_signing_policy();
    std::cout << "PASS: test_strict_signing_policy" << std::endl;

    test_relaxed_signing_policy();
    std::cout << "PASS: test_relaxed_signing_policy" << std::endl;

    test_no_signing_policy();
    std::cout << "PASS: test_no_signing_policy" << std::endl;

    test_full_symbol_policy();
    std::cout << "PASS: test_full_symbol_policy" << std::endl;

    test_public_only_symbol_policy();
    std::cout << "PASS: test_public_only_symbol_policy" << std::endl;

    test_minimal_symbol_policy();
    std::cout << "PASS: test_minimal_symbol_policy" << std::endl;

    test_strict_update_policy();
    std::cout << "PASS: test_strict_update_policy" << std::endl;

    test_staged_update_policy();
    std::cout << "PASS: test_staged_update_policy" << std::endl;

    test_manual_update_policy();
    std::cout << "PASS: test_manual_update_policy" << std::endl;

    std::cout << "\nAll 12 P2-052 tests passed!" << std::endl;
    return 0;
}