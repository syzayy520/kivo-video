#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

#include "video/subtitle_plane/core/plane_lifecycle_model.hpp"
#include "video/subtitle_plane/generation/generation_freshness.hpp"

int main() {
    using namespace kivo::video::subtitle_plane::runtime;

    if (timeline_for_plane_state(KivoSubtitlePlaneState_Created) !=
        KivoSubtitleTimelineAvailability_NotYetStarted) {
        return 1;
    }
    if (timeline_for_plane_state(KivoSubtitlePlaneState_Parsing) !=
        KivoSubtitleTimelineAvailability_Buffering) {
        return 2;
    }
    if (timeline_for_plane_state(KivoSubtitlePlaneState_Ready) !=
        KivoSubtitleTimelineAvailability_Available) {
        return 3;
    }
    if (timeline_for_plane_state(KivoSubtitlePlaneState_Seeking) !=
        KivoSubtitleTimelineAvailability_Seeking) {
        return 4;
    }
    if (timeline_for_plane_state(KivoSubtitlePlaneState_Recovering) !=
        KivoSubtitleTimelineAvailability_Recovering) {
        return 5;
    }
    if (timeline_for_plane_state(KivoSubtitlePlaneState_Closed) !=
        KivoSubtitleTimelineAvailability_Closed) {
        return 6;
    }
    if (timeline_for_plane_state(KivoSubtitlePlaneState_Failed) !=
        KivoSubtitleTimelineAvailability_Failed) {
        return 7;
    }
    if (!plane_state_allows_frame_build(KivoSubtitlePlaneState_Ready) ||
        !plane_state_allows_frame_build(KivoSubtitlePlaneState_EosReached) ||
        plane_state_allows_frame_build(KivoSubtitlePlaneState_Parsing)) {
        return 8;
    }

    KivoSubtitleTimelineGeneration old_generation{};
    old_generation.playback_revision.value = 10;
    old_generation.graph_snapshot_revision.value = 20;
    old_generation.seek_epoch.value = 30;
    old_generation.delay_epoch = 1;
    KivoSubtitleTimelineGeneration new_generation = old_generation;
    new_generation.delay_epoch = 2;
    if (!delay_change_keeps_static_cues(old_generation, new_generation)) return 9;
    new_generation.seek_epoch.value = 31;
    if (delay_change_keeps_static_cues(old_generation, new_generation)) return 10;

    KivoSubtitleContentGeneration content{};
    content.source_generation = 1;
    content.track_generation = 2;
    content.parser_generation = 3;
    if (!static_external_index_survives_seek(content)) return 11;
    content.parser_generation = 0;
    if (static_external_index_survives_seek(content)) return 12;

    KivoSubtitlePacketGeneration expected{};
    expected.source_generation = 1;
    expected.track_generation = 2;
    expected.seek_epoch.value = 3;
    expected.packet_generation = 4;
    KivoSubtitlePacketGeneration packet = expected;
    packet.packet_generation = 5;
    if (!embedded_packet_matches_generation(expected, packet)) return 13;
    packet.seek_epoch.value = 4;
    if (embedded_packet_matches_generation(expected, packet)) return 14;

    if (!cold_start_accepts_packet(KivoSubtitleTimelineAvailability_NotYetStarted)) return 15;
    if (!cold_start_accepts_packet(KivoSubtitleTimelineAvailability_TemporarilyUnavailable)) return 16;
    if (cold_start_accepts_packet(KivoSubtitleTimelineAvailability_Available)) return 17;

    return 0;
}
