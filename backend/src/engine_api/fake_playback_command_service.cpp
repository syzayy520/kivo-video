#include "kivo/cinema_engine/engine_api/fake_playback_command_service.hpp"

namespace kivo::cinema_engine {

EngineCommand build_open_command(std::string session_id) {
  EngineCommand cmd;
  cmd.metadata.schema_version = "1.1";
  cmd.command_id = "cmd.open.001";
  cmd.kind = EngineCommandKind::Open;
  cmd.payload_ref = std::move(session_id);
  return cmd;
}

EngineCommand build_play_command(uint64_t generation) {
  EngineCommand cmd;
  cmd.metadata.schema_version = "1.1";
  cmd.command_id = "cmd.play." + std::to_string(generation);
  cmd.kind = EngineCommandKind::Play;
  return cmd;
}

EngineCommand build_pause_command(uint64_t generation) {
  EngineCommand cmd;
  cmd.metadata.schema_version = "1.1";
  cmd.command_id = "cmd.pause." + std::to_string(generation);
  cmd.kind = EngineCommandKind::Pause;
  return cmd;
}

EngineCommand build_seek_command(uint64_t generation, std::string seek_target_ref) {
  EngineCommand cmd;
  cmd.metadata.schema_version = "1.1";
  cmd.command_id = "cmd.seek." + std::to_string(generation);
  cmd.kind = EngineCommandKind::Seek;
  cmd.payload_ref = std::move(seek_target_ref);
  return cmd;
}

EngineCommand build_close_command(uint64_t generation) {
  EngineCommand cmd;
  cmd.metadata.schema_version = "1.1";
  cmd.command_id = "cmd.close." + std::to_string(generation);
  cmd.kind = EngineCommandKind::Close;
  return cmd;
}

bool is_command_generation_valid(const EngineCommand& cmd, uint64_t expected_generation) {
  // Close is always valid regardless of generation.
  if (cmd.kind == EngineCommandKind::Close) {
    return true;
  }
  // Parse generation from command_id (format: "cmd.kind.GENERATION")
  // For simplicity, we check if command_id contains the expected generation.
  auto pos = cmd.command_id.rfind('.');
  if (pos == std::string::npos) return false;
  try {
    uint64_t cmd_gen = std::stoull(cmd.command_id.substr(pos + 1));
    return cmd_gen >= expected_generation;
  } catch (...) {
    return false;
  }
}

}  // namespace kivo::cinema_engine
