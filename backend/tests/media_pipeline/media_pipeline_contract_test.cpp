#include <cassert>

#include "kivo/cinema_engine/decode_core/fake_decode_path.hpp"
#include "kivo/cinema_engine/demux_core/fake_stream_probe.hpp"
#include "kivo/cinema_engine/fake_backend/fake_session_scenario.hpp"
#include "kivo/cinema_engine/network_cache_core/fake_cache_reader.hpp"

int main() {
  const auto scenario = kivo::cinema_engine::build_fake_direct_play_session();

  kivo::cinema_engine::FakeCacheReadRequest read_request;
  read_request.cache_extent_id = scenario.cache_extent.cache_extent_id;
  read_request.remote_object_identity_ref = scenario.remote_object_identity.remote_object_identity_id;
  read_request.offset = scenario.cache_extent.start_byte;
  read_request.length = 4096;
  const auto cache_result = kivo::cinema_engine::read_fake_cache_extent(
      scenario.cache_extent,
      scenario.remote_object_identity,
      read_request);
  assert(cache_result.allowed);
  assert(cache_result.readable_bytes == 4096);

  read_request.remote_object_identity_ref = "roi.mismatch";
  const auto mismatch_result = kivo::cinema_engine::read_fake_cache_extent(
      scenario.cache_extent,
      scenario.remote_object_identity,
      read_request);
  assert(!mismatch_result.allowed);

  const auto inventory = kivo::cinema_engine::build_fake_stream_inventory(scenario.session.metadata);
  assert(inventory.metadata.schema_version == "1.1");
  assert(!inventory.video_streams.empty());
  assert(!inventory.audio_streams.empty());
  assert(!inventory.subtitle_streams.empty());

  const auto decode_path = kivo::cinema_engine::build_fake_decode_path(scenario.session.metadata, inventory.video_streams.front());
  assert(decode_path.selected_video_stream == inventory.video_streams.front());
  assert(decode_path.hardware_preferred);
  assert(decode_path.software_fallback_available);

  return 0;
}

