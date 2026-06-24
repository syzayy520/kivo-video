#pragma once

#include <cstddef>
#include <optional>
#include <queue>

#include "kivo/cinema_engine/engine_api/engine_command.hpp"

namespace kivo::cinema_engine {

class CommandQueue {
 public:
  bool submit(EngineCommand command);
  std::optional<EngineCommand> try_pop();
  std::size_t size() const;
  bool empty() const;

 private:
  std::queue<EngineCommand> commands_;
};

}  // namespace kivo::cinema_engine

