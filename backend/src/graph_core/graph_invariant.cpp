#include "kivo/cinema_engine/graph_core/graph_invariant.hpp"

namespace kivo::cinema_engine {

std::vector<GraphInvariantViolation> check_graph_invariants(const GraphInvariantInput& input) {
  std::vector<GraphInvariantViolation> violations;

  for (const auto& node : input.nodes) {
    if (node.node_id.empty() || node.lifecycle_state.empty()) {
      violations.push_back({"GRAPH_NODE_REQUIRES_LIFECYCLE", "Every graph node must have lifecycle state."});
    }
  }

  for (const auto& edge : input.edges) {
    if (edge.edge_id.empty() || edge.capacity == 0) {
      violations.push_back({"GRAPH_EDGE_REQUIRES_CAPACITY", "Every graph edge must have bounded capacity."});
    }
  }

  if (input.stopped && input.produced_frame_after_stop) {
    violations.push_back({"GRAPH_STOP_FORBIDS_FRAME", "Graph stop must prevent later frame production."});
  }

  if (input.flush_generation.has_value() && input.flush_generation.value() == 0) {
    violations.push_back({"GRAPH_FLUSH_REQUIRES_GENERATION", "Graph flush must carry a non-zero generation."});
  }

  return violations;
}

}  // namespace kivo::cinema_engine

