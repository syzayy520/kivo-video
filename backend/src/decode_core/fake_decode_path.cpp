#include "kivo/cinema_engine/decode_core/fake_decode_path.hpp"

#include <utility>

namespace kivo::cinema_engine {

FakeDecodePath build_fake_decode_path(const ContractMetadata& metadata, std::string selected_video_stream) {
  FakeDecodePath path;
  path.metadata = metadata;
  path.decode_path_id = "decode.path.fake.0001";
  path.selected_video_stream = std::move(selected_video_stream);
  path.decoder_kind = "fake-hevc-main10-hardware";
  path.hardware_preferred = true;
  path.software_fallback_available = true;
  path.fallback_reasons = {"HW_DECODE_TO_SW_DECODE"};
  return path;
}

}  // namespace kivo::cinema_engine

