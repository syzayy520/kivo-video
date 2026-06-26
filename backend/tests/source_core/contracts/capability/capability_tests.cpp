#include "kivo/video/source_core/contracts/capability/source_capability_snapshot.hpp"

#include <cassert>
#include <cstdint>

using namespace kivo::video::source_core;

int main() {
    // Default capability uses typed states, not booleans
    SourceCapabilitySnapshot cap;
    
    // Defaults are unknown
    assert(cap.seek == CapabilityState::unknown);
    assert(cap.range_read == CapabilityState::unknown);
    assert(cap.direct_play_candidate == CapabilityState::unknown);
    
    // Optionals are empty by default (unknown, not false)
    assert(!cap.content_length_bytes.has_value());
    assert(!cap.mime_type.has_value());
    
    // Set supported states
    cap.seek = CapabilityState::supported;
    cap.range_read = CapabilityState::unsupported;
    cap.content_length_bytes = 12345;
    
    assert(cap.seek == CapabilityState::supported);
    assert(cap.range_read == CapabilityState::unsupported);
    assert(cap.content_length_bytes.value() == 12345);
    
    // Unknown is not supported
    assert(CapabilityState::unknown != CapabilityState::supported);
    assert(CapabilityState::unknown != CapabilityState::unsupported);
    
    return 0;
}
