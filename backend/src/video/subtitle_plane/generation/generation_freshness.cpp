#include "video/subtitle_plane/generation/generation_freshness.hpp"

namespace kivo::video::subtitle_plane::runtime {

bool delay_change_keeps_static_cues(
    const KivoSubtitleTimelineGeneration old_generation,
    const KivoSubtitleTimelineGeneration new_generation) noexcept {
    return old_generation.playback_revision.value == new_generation.playback_revision.value &&
           old_generation.graph_snapshot_revision.value == new_generation.graph_snapshot_revision.value &&
           old_generation.seek_epoch.value == new_generation.seek_epoch.value;
}

bool static_external_index_survives_seek(
    const KivoSubtitleContentGeneration content) noexcept {
    return content.source_generation != 0 &&
           content.track_generation != 0 &&
           content.parser_generation != 0;
}

bool embedded_packet_matches_generation(
    const KivoSubtitlePacketGeneration expected,
    const KivoSubtitlePacketGeneration packet) noexcept {
    return expected.source_generation == packet.source_generation &&
           expected.track_generation == packet.track_generation &&
           expected.seek_epoch.value == packet.seek_epoch.value &&
           packet.packet_generation >= expected.packet_generation;
}

bool cold_start_accepts_packet(
    const KivoSubtitleTimelineAvailability availability) noexcept {
    return availability == KivoSubtitleTimelineAvailability_NotYetStarted ||
           availability == KivoSubtitleTimelineAvailability_TemporarilyUnavailable ||
           availability == KivoSubtitleTimelineAvailability_Buffering;
}

}  // namespace kivo::video::subtitle_plane::runtime
