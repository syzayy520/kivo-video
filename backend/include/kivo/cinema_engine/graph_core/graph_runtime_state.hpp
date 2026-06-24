#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::cinema_engine {

struct GraphRuntimeState {
  std::string graph_id;
  std::string lifecycle_state;
  std::vector<std::string> node_states;
  std::vector<std::string> edge_states;
  std::string queue_pressure;
  std::string backpressure;
  std::uint64_t graph_generation{0};
};

}  // namespace kivo::cinema_engine

