#include <cassert>

#include "kivo/cinema_engine/graph_core/fake_graph_runtime.hpp"

int main() {
  kivo::cinema_engine::FakeGraphRuntime graph;

  assert(!graph.add_node({"demux", "Running"}));
  assert(graph.bind("graph.fake.0001", 1));
  assert(graph.add_node({"source", "Running"}));
  assert(graph.add_node({"decoder", "Running"}));
  assert(!graph.add_edge({"edge.invalid", "source", "decoder", 0}));
  assert(graph.add_edge({"edge.source.decoder", "source", "decoder", 8}));
  assert(graph.set_seek_barrier(1));
  assert(graph.set_backpressure("bounded-queue-pressure"));
  assert(graph.flush(1));
  assert(graph.invariant_violations().empty());
  const auto state = graph.state();
  assert(state.graph_id == "graph.fake.0001");
  assert(state.graph_generation == 1);
  assert(state.edge_states.size() == 1);
  assert(graph.stop());
  assert(graph.mark_frame_produced_after_stop_for_test());
  assert(!graph.invariant_violations().empty());

  return 0;
}

