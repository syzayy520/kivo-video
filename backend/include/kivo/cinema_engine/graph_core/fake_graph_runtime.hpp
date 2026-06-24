#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "kivo/cinema_engine/graph_core/graph_invariant.hpp"
#include "kivo/cinema_engine/graph_core/graph_runtime_state.hpp"

namespace kivo::cinema_engine {

struct FakeGraphNode {
  std::string node_id;
  std::string lifecycle_state;
};

struct FakeGraphEdge {
  std::string edge_id;
  std::string from_node_id;
  std::string to_node_id;
  std::uint64_t capacity{0};
};

class FakeGraphRuntime {
 public:
  bool bind(std::string graph_id, std::uint64_t generation);
  bool add_node(FakeGraphNode node);
  bool add_edge(FakeGraphEdge edge);
  bool set_seek_barrier(std::uint64_t generation);
  bool set_backpressure(std::string backpressure);
  bool flush(std::uint64_t generation);
  bool stop();
  bool mark_frame_produced_after_stop_for_test();

  GraphRuntimeState state() const;
  GraphInvariantInput invariant_input() const;
  std::vector<GraphInvariantViolation> invariant_violations() const;

 private:
  std::string graph_id_;
  std::uint64_t generation_{0};
  std::vector<FakeGraphNode> nodes_;
  std::vector<FakeGraphEdge> edges_;
  std::string queue_pressure_{"normal"};
  std::string backpressure_{"none"};
  std::optional<std::uint64_t> seek_barrier_generation_;
  std::optional<std::uint64_t> flush_generation_;
  bool stopped_{false};
  bool produced_frame_after_stop_{false};
};

}  // namespace kivo::cinema_engine

