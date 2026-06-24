#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// GraphEdge represents a bounded queue between two graph nodes.
// All edges must have capacity; backpressure is enforced.
struct GraphEdge {
    ContractMetadata metadata;
    std::string edge_id;
    std::string source_node_id;
    std::string target_node_id;
    int64_t capacity{0};        // max items in queue
    int64_t current_size{0};    // current items in queue
    bool is_backpressured{false};
    int64_t generation{0};
    int64_t flush_generation{0};
};

}  // namespace kivo::cinema_engine
