#include "kivo/video/source_core/contracts/capability/source_capability_snapshot.hpp"
#include "source_core/test_helpers.hpp"

using namespace kivo::video::source_core;
using namespace kivo::video::source_core::test;

int main() {
    SourceCapabilitySnapshot cap;
    CHECK_EQ(cap.seek, CapabilityState::unknown);
    CHECK_EQ(cap.range_read, CapabilityState::unknown);
    CHECK_EQ(cap.direct_play_candidate, CapabilityState::unknown);
    CHECK_TRUE(!cap.content_length_bytes.has_value());
    CHECK_TRUE(!cap.mime_type.has_value());

    cap.seek = CapabilityState::supported;
    cap.range_read = CapabilityState::unsupported;
    cap.content_length_bytes = 12345;
    CHECK_EQ(cap.seek, CapabilityState::supported);
    CHECK_EQ(cap.range_read, CapabilityState::unsupported);
    CHECK_EQ(cap.content_length_bytes.value(), 12345ULL);
    CHECK_NEQ(CapabilityState::unknown, CapabilityState::supported);
    return 0;
}
