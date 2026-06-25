#pragma once

#include "kivo/cinema_engine/signing_core/code_signing_policy.hpp"

namespace kivo::cinema_engine {

// Fake signing service for testing.
// Provides static builder functions for code signing policies.
class FakeSigningService {
 public:
    // Code signing policy builders
    static CodeSigningPolicy build_strict_signing_policy();
    static CodeSigningPolicy build_relaxed_signing_policy();
    static CodeSigningPolicy build_no_signing_policy();
};

}  // namespace kivo::cinema_engine