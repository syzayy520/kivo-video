#pragma once

#include "kivo/cinema_engine/update_core/update_policy.hpp"

namespace kivo::cinema_engine {

// Fake update service for testing.
// Provides static builder functions for update policies.
class FakeUpdateService {
 public:
    // Update policy builders
    static UpdatePolicy build_strict_update_policy();
    static UpdatePolicy build_staged_update_policy();
    static UpdatePolicy build_manual_update_policy();
};

}  // namespace kivo::cinema_engine