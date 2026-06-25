#pragma once

#include "kivo/cinema_engine/symbol_core/symbol_management_policy.hpp"

namespace kivo::cinema_engine {

// Fake symbol service for testing.
// Provides static builder functions for symbol management policies.
class FakeSymbolService {
 public:
    // Symbol management policy builders
    static SymbolManagementPolicy build_full_symbol_policy();
    static SymbolManagementPolicy build_public_only_symbol_policy();
    static SymbolManagementPolicy build_minimal_symbol_policy();
};

}  // namespace kivo::cinema_engine