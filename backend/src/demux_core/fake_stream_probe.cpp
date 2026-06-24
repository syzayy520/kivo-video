#include "kivo/cinema_engine/demux_core/fake_stream_probe.hpp"

namespace kivo::cinema_engine {

StreamInventory build_fake_stream_inventory(const ContractMetadata& metadata) {
  StreamInventory inventory;
  inventory.metadata = metadata;
  inventory.inventory_id = "stream.inventory.fake.0001";
  inventory.session_id = metadata.session_id;
  inventory.container_format = "matroska";
  inventory.duration = "PT2H00M00S";
  inventory.bitrate_estimate = "80Mbps";
  inventory.video_streams = {"video.hevc.main10"};
  inventory.audio_streams = {"audio.truehd.7.1", "audio.eac3.compat"};
  inventory.subtitle_streams = {"sub.ass.jpn", "sub.srt.eng"};
  inventory.chapters = {"chapter.1", "chapter.2"};
  inventory.attachments = {"font.embedded.primary"};
  inventory.hdr_metadata = "hdr10-static";
  inventory.dovi_metadata = "rpu-present-profile8";
  return inventory;
}

}  // namespace kivo::cinema_engine

