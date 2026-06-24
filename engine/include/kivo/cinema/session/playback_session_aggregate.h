#pragma once

#include <string>

struct PlaybackSessionAggregate {
  std::string session_id;
  std::string media_id;
  std::string version_id;
  std::string plan_id;
  std::string trace_root_id;
};
