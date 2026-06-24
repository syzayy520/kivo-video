#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct FakeDecodePath {
  ContractMetadata metadata;
  std::string decode_path_id;
  std::string selected_video_stream;
  std::string decoder_kind;
  bool hardware_preferred{true};
  bool software_fallback_available{true};
  std::vector<std::string> fallback_reasons;
};

FakeDecodePath build_fake_decode_path(const ContractMetadata& metadata, std::string selected_video_stream);

}  // namespace kivo::cinema_engine

