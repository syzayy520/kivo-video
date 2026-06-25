#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Hermetic fake test requirement.
// Defines isolation level and reset requirements for hermetic testing.
struct HermeticFakeRequirement {
    ContractMetadata metadata;
    std::string requirement_id;  // Unique requirement identifier
    std::string fake_type;  // Type of fake (mock, stub, spy, fake)
    std::string isolation_level;  // Isolation level (process, thread, function)
    bool state_reset_required{true};  // Whether state reset is required between tests
    bool dependency_injection{true};  // Whether dependency injection is required
    std::string description;  // Requirement description
};

}  // namespace kivo::cinema_engine