#pragma once

#include <optional>
#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct StreamInventory {
  ContractMetadata metadata;
  std::string inventory_id;
  std::optional<std::string> session_id;
  std::string container_format;
  std::string duration;
  std::string bitrate_estimate;
  std::vector<std::string> video_streams;
  std::vector<std::string> audio_streams;
  std::vector<std::string> subtitle_streams;
  std::vector<std::string> chapters;
  std::vector<std::string> attachments;
  std::string hdr_metadata;
  std::string dovi_metadata;
  std::optional<std::string> disc_structure;
};

}  // namespace kivo::cinema_engine

