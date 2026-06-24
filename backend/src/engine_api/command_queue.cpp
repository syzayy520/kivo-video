#include "kivo/cinema_engine/engine_api/command_queue.hpp"

#include <utility>

namespace kivo::cinema_engine {

bool CommandQueue::submit(EngineCommand command) {
  if (command.command_id.empty()) {
    return false;
  }

  commands_.push(std::move(command));
  return true;
}

std::optional<EngineCommand> CommandQueue::try_pop() {
  if (commands_.empty()) {
    return std::nullopt;
  }

  EngineCommand command = std::move(commands_.front());
  commands_.pop();
  return command;
}

std::size_t CommandQueue::size() const {
  return commands_.size();
}

bool CommandQueue::empty() const {
  return commands_.empty();
}

}  // namespace kivo::cinema_engine
