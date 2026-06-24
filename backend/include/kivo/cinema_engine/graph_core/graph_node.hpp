#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// GraphNode represents a node in the processing graph.
// Each node has a lifecycle state and can route frames.
struct GraphNode {
    ContractMetadata metadata;
    std::string node_id;
    std::string node_kind;  // "source", "demux", "decode", "filter", "sink"
    std::string lifecycle_state;  // "created", "running", "paused", "draining", "stopped", "failed"
    int64_t generation{0};
    bool has_input{false};
    bool has_output{false};
    int64_t frames_processed{0};
    int64_t frames_dropped{0};
};

}  // namespace kivo::cinema_engine
