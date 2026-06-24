#include "kivo/cinema_engine/graph_core/fake_graph_runtime.hpp"

#include <utility>

namespace kivo::cinema_engine {

bool FakeGraphRuntime::bind(std::string graph_id, std::uint64_t generation) {
  if (!graph_id_.empty() || graph_id.empty() || generation == 0) {
    return false;
  }

  graph_id_ = std::move(graph_id);
  generation_ = generation;
  return true;
}

bool FakeGraphRuntime::add_node(FakeGraphNode node) {
  if (graph_id_.empty() || node.node_id.empty() || node.lifecycle_state.empty()) {
    return false;
  }

  nodes_.push_back(std::move(node));
  return invariant_violations().empty();
}

bool FakeGraphRuntime::add_edge(FakeGraphEdge edge) {
  if (graph_id_.empty() || edge.edge_id.empty() || edge.from_node_id.empty() || edge.to_node_id.empty() || edge.capacity == 0) {
    return false;
  }

  edges_.push_back(std::move(edge));
  return invariant_violations().empty();
}

bool FakeGraphRuntime::set_seek_barrier(std::uint64_t generation) {
  if (generation == 0 || graph_id_.empty()) {
    return false;
  }

  seek_barrier_generation_ = generation;
  return true;
}

bool FakeGraphRuntime::set_backpressure(std::string backpressure) {
  if (backpressure.empty() || graph_id_.empty()) {
    return false;
  }

  backpressure_ = std::move(backpressure);
  return true;
}

bool FakeGraphRuntime::flush(std::uint64_t generation) {
  if (generation == 0 || graph_id_.empty()) {
    return false;
  }

  flush_generation_ = generation;
  return invariant_violations().empty();
}

bool FakeGraphRuntime::stop() {
  if (graph_id_.empty()) {
    return false;
  }

  stopped_ = true;
  return invariant_violations().empty();
}

bool FakeGraphRuntime::mark_frame_produced_after_stop_for_test() {
  if (!stopped_) {
    return false;
  }

  produced_frame_after_stop_ = true;
  return true;
}

GraphRuntimeState FakeGraphRuntime::state() const {
  GraphRuntimeState state;
  state.graph_id = graph_id_;
  state.lifecycle_state = stopped_ ? "Stopped" : "Running";
  for (const auto& node : nodes_) {
    state.node_states.push_back(node.node_id + ":" + node.lifecycle_state);
  }
  for (const auto& edge : edges_) {
    state.edge_states.push_back(edge.edge_id + ":capacity=" + std::to_string(edge.capacity));
  }
  state.queue_pressure = queue_pressure_;
  state.backpressure = backpressure_;
  state.graph_generation = generation_;
  return state;
}

GraphInvariantInput FakeGraphRuntime::invariant_input() const {
  GraphInvariantInput input;
  for (const auto& node : nodes_) {
    input.nodes.push_back({node.node_id, node.lifecycle_state});
  }
  for (const auto& edge : edges_) {
    input.edges.push_back({edge.edge_id, edge.capacity});
  }
  input.stopped = stopped_;
  input.produced_frame_after_stop = produced_frame_after_stop_;
  input.flush_generation = flush_generation_;
  return input;
}

std::vector<GraphInvariantViolation> FakeGraphRuntime::invariant_violations() const {
  return check_graph_invariants(invariant_input());
}

}  // namespace kivo::cinema_engine

