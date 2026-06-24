#pragma once

#include <string>

struct VideoPresenterContract {
  std::string id;
  double refresh_rate = 0.0;
  double content_frame_rate = 0.0;
  std::string pacing_mode;
  std::string clock_master;
};
