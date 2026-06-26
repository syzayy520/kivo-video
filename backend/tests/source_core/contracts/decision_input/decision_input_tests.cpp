#include "kivo/video/source_core/contracts/decision_input/direct_play_input.hpp"
#include "kivo/video/source_core/contracts/decision_input/direct_stream_input.hpp"
#include "kivo/video/source_core/contracts/decision_input/source_core_contract_version.hpp"

#include <cassert>

using namespace kivo::video::source_core;

int main() {
    // Contract version is 1
    static_assert(kSourceCoreContractVersion == 1, "Contract version must be 1");
    
    // DirectPlayInput defaults
    DirectPlayInput dpi;
    assert(dpi.contract_version == kSourceCoreContractVersion);
    assert(dpi.seekable == CapabilityState::unknown);
    assert(!dpi.content_length_bytes.has_value());
    assert(!dpi.mime_type.has_value());
    
    // DirectStreamInput defaults
    DirectStreamInput dsi;
    assert(dsi.contract_version == kSourceCoreContractVersion);
    assert(!dsi.transcode_hint.has_value());
    assert(!dsi.bandwidth_estimate_bps.has_value());
    
    // They are carriers, not decision engines
    dpi.seekable = CapabilityState::supported;
    dpi.content_length_bytes = 999;
    assert(dpi.seekable == CapabilityState::supported);
    
    dsi.bandwidth_estimate_bps = 5000000;
    assert(dsi.bandwidth_estimate_bps.value() == 5000000);
    
    return 0;
}
