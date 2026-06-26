#include "kivo/video/source_core/contracts/decision_input/direct_play_input.hpp"
#include "kivo/video/source_core/contracts/decision_input/direct_stream_input.hpp"
#include "kivo/video/source_core/contracts/decision_input/source_core_contract_version.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    // Contract version is 1
    CHECK_TRUE(kSourceCoreContractVersion == 1);

    // DirectPlayInput carries identity, capability, evidence
    DirectPlayInput dpi;
    CHECK_EQ(dpi.contract_schema_version, kSourceCoreContractVersion);
    CHECK_EQ(dpi.identity.id.value, 0ULL);
    CHECK_EQ(dpi.capability.seek, CapabilityState::unknown);
    CHECK_TRUE(dpi.evidence.is_contract_pass_only());
    CHECK_TRUE(!dpi.evidence.has_runtime_pass());
    CHECK_TRUE(!dpi.safe_metadata_hint.has_value());
    
    dpi.identity.id = SourceIdentityId{42};
    dpi.capability.seek = CapabilityState::supported;
    CHECK_EQ(dpi.identity.id.value, 42ULL);
    CHECK_EQ(dpi.capability.seek, CapabilityState::supported);

    // DirectStreamInput carries same evidence pack
    DirectStreamInput dsi;
    CHECK_EQ(dsi.contract_schema_version, kSourceCoreContractVersion);
    CHECK_EQ(dsi.identity.id.value, 0ULL);
    CHECK_TRUE(dsi.evidence.is_contract_pass_only());
    CHECK_TRUE(!dsi.evidence.has_runtime_pass());
    CHECK_TRUE(!dsi.safe_metadata_hint.has_value());
    
    dsi.identity.id = SourceIdentityId{99};
    CHECK_EQ(dsi.identity.id.value, 99ULL);
    return 0;
}
