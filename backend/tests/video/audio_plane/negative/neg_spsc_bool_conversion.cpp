// neg_spsc_bool_conversion.cpp — P6G-039 Negative Fixture
// Forbidden pattern: bool return from SPSC queue (§15, §42)
// Expectation: COMPILE FAIL (enum class AudioSpscPushResult is NOT implicitly convertible to bool)
//
// If this file COMPILES SUCCESSFULLY, the strong-typed SPSC result contract is VIOLATED.
// If this file FAILS TO COMPILE, the contract is ENFORCED (test passes via WILL_FAIL).
#include "kivo/video/audio_plane/render/audio_spsc_result.hpp"

int main() {
    using kivo::video::audio_plane::render::AudioSpscPushResult;
    // Forbidden: SPSC result must NOT be convertible to bool.
    // enum class prevents implicit conversion — this line must fail to compile.
    bool b = AudioSpscPushResult::Pushed;
    (void)b;
    return 0;
}
