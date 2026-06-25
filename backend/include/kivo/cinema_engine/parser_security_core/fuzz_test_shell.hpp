#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Fuzz test shell requirements.
// Defines configuration for fuzz testing of media parsers.
struct FuzzTestShell {
    ContractMetadata metadata;
    std::string shell_id;  // Unique shell identifier
    int max_iterations{10000};  // Maximum fuzz iterations
    bool crash_reproduction{true};  // Whether to support crash reproduction
    bool coverage_guided{true};  // Whether fuzzing is coverage-guided
    std::string seed_corpus_path;  // Path to seed corpus
    std::string mutation_strategy{"bitflip"};  // Mutation strategy
};

}  // namespace kivo::cinema_engine