#pragma once

#include <cstdint>

#include "kivo/cinema_engine/engine_api/engine_command.hpp"

namespace kivo::cinema_engine {

// Build an Open command with generation 1.
EngineCommand build_open_command(std::string session_id);

// Build a Play command with given generation.
EngineCommand build_play_command(uint64_t generation);

// Build a Pause command with given generation.
EngineCommand build_pause_command(uint64_t generation);

// Build a Seek command with given generation and target.
EngineCommand build_seek_command(uint64_t generation, std::string seek_target_ref);

// Build a Close command with given generation.
EngineCommand build_close_command(uint64_t generation);

// Check if a command's generation is valid (>= expected).
bool is_command_generation_valid(const EngineCommand& cmd, uint64_t expected_generation);

}  // namespace kivo::cinema_engine
