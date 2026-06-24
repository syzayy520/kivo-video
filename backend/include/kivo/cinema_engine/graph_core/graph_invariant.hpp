#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace kivo::cinema_engine {

struct GraphNodeInvariantState {
  std::string node_id;
  std::string lifecycle_state;
};

struct GraphEdgeInvariantState {
  std::string edge_id;
  std::uint64_t capacity{0};
};

struct GraphInvariantInput {
  std::vector<GraphNodeInvariantState> nodes;
  std::vector<GraphEdgeInvariantState> edges;
  bool stopped{false};
  bool produced_frame_after_stop{false};
  std::optional<std::uint64_t> flush_generation;
};

struct GraphInvariantViolation {
  std::string code;
  std::string message;
};

std::vector<GraphInvariantViolation> check_graph_invariants(const GraphInvariantInput& input);

}  // namespace kivo::cinema_engine

