// neg_span_in_access_unit.cpp — P6G-039 Negative Fixture
// Forbidden pattern: span stored in EncodedAccessUnit (§8, §12, §42)
// Expectation: COMPILE FAIL (payload_handle_id is uint64_t, NOT std::span)
//
// EncodedAccessUnit must hold OWNED payload only (§12). The payload_handle_id
// field is uint64_t (opaque handle), not std::span. Assigning a span to it
// must fail to compile due to type mismatch.
//
// If this file COMPILES SUCCESSFULLY, the owned-payload contract is VIOLATED.
// If this file FAILS TO COMPILE, the contract is ENFORCED (test passes via WILL_FAIL).
#include "kivo/video/audio_plane/passthrough/encoded_access_unit.hpp"
#include <span>

int main() {
    using kivo::video::audio_plane::passthrough::EncodedAccessUnit;
    // Forbidden: EncodedAccessUnit must NOT store borrowed span.
    // payload_handle_id is uint64_t — assigning std::span must fail to compile.
    std::span<uint8_t> borrowed_bytes{};
    EncodedAccessUnit unit{};
    unit.payload_handle_id = borrowed_bytes;  // type mismatch: span != uint64_t
    return 0;
}
