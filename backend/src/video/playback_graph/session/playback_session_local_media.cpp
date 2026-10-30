#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

LocalMediaPlaybackQuery PlaybackSessionRuntime::query_local_media_playback(
    const bool include_frame_pixels) const noexcept {
    LocalMediaPlaybackQuery query{};
    const auto& probe_status = local_media_pipeline_.status();
    const bool continuous_active = local_media_continuous_.is_active();
    const auto continuous_stats = local_media_continuous_.stats();

    query.active = local_media_pipeline_.is_active() || continuous_active;
    query.source_session_connected = continuous_active
                                         ? continuous_stats.source_session_connected
                                         : probe_status.source_session_connected;
    query.source_opened = probe_status.source_opened;
    if (continuous_active) {
        query.demux_started = continuous_stats.demux_started;
        query.video_decode_started = continuous_stats.video_decode_started;
        query.audio_decode_started = continuous_stats.audio_decode_started;
        query.audio_output_started = continuous_stats.audio_output_started;
        query.first_frame_decoded = continuous_stats.first_frame_decoded;
        query.d3d11_uploaded = continuous_stats.d3d11_uploaded;
        query.d3d11_upload_available = continuous_stats.d3d11_upload_available;
        query.d3d11_upload_initialized = continuous_stats.d3d11_upload_initialized;
        query.frame_rendered = continuous_stats.frame_rendered;
        query.continuous_rendering = continuous_stats.continuous_rendering;
        query.decoded_frame_count = continuous_stats.decoded_frame_count;
        query.rendered_frame_count = continuous_stats.rendered_frame_count;
        query.d3d11_frames_uploaded = continuous_stats.d3d11_frames_uploaded;
        query.d3d11_upload_failures = continuous_stats.d3d11_upload_failures;
        query.d3d11_upload_last_elapsed_ms =
            continuous_stats.d3d11_upload_last_elapsed_ms;
        query.d3d11_upload_max_elapsed_ms =
            continuous_stats.d3d11_upload_max_elapsed_ms;
        query.audio_packets_submitted = continuous_stats.audio_packets_submitted;
        query.audio_frames_decoded = continuous_stats.audio_frames_decoded;
        query.audio_frames_written = continuous_stats.audio_frames_written;
        query.audio_flush_count = continuous_stats.audio_flush_count;
        query.audio_writer_reset_count = continuous_stats.audio_writer_reset_count;
        query.audio_stale_packets_discarded =
            continuous_stats.audio_stale_packets_discarded;
        query.p7_background_pump_count =
            continuous_stats.p7_background_pump_count;
        query.p7_background_pump_last_elapsed_ms =
            continuous_stats.p7_background_pump_last_elapsed_ms;
        query.p7_background_pump_max_elapsed_ms =
            continuous_stats.p7_background_pump_max_elapsed_ms;
        query.p7_background_pump_total_elapsed_ms =
            continuous_stats.p7_background_pump_total_elapsed_ms;
        query.p7_background_pump_last_new_frames =
            continuous_stats.p7_background_pump_last_new_frames;
        query.p7_background_pump_packet_budget =
            continuous_stats.p7_background_pump_packet_budget;
        query.p7_background_pump_max_packet_budget =
            continuous_stats.p7_background_pump_max_packet_budget;
        query.p7_seek_preroll_active =
            continuous_stats.p7_seek_preroll_active;
        query.p7_seek_preroll_completed_count =
            continuous_stats.p7_seek_preroll_completed_count;
        query.p7_seek_preroll_last_elapsed_ms =
            continuous_stats.p7_seek_preroll_last_elapsed_ms;
        query.p7_seek_preroll_max_elapsed_ms =
            continuous_stats.p7_seek_preroll_max_elapsed_ms;
        query.p5_local_video_runtime_connected =
            continuous_stats.p5_local_video_runtime_connected;
        query.p5_audio_packet_boundary_connected =
            continuous_stats.p5_audio_packet_boundary_connected;
        query.p5_subtitle_packet_boundary_connected =
            continuous_stats.p5_subtitle_packet_boundary_connected;
        query.p8_embedded_packet_ingest_connected =
            continuous_stats.p8_embedded_packet_ingest_connected;
        query.p8_pgs_bitmap_render_connected =
            continuous_stats.p8_pgs_bitmap_render_connected;
        query.p5_audio_packets_dispatched =
            continuous_stats.p5_audio_packets_dispatched;
        query.p5_subtitle_packets_dispatched =
            continuous_stats.p5_subtitle_packets_dispatched;
        query.p8_subtitle_packets_submitted =
            continuous_stats.p8_subtitle_packets_submitted;
        query.p8_unsupported_feature_count =
            continuous_stats.p8_unsupported_feature_count;
        query.playback_elapsed_ms = continuous_stats.playback_elapsed_ms;
        query.last_frame_signature = continuous_stats.last_frame_signature;
        query.last_frame_time_us = continuous_stats.last_frame_time_us;
        query.last_frame_duration_us = continuous_stats.last_frame_duration_us;
        query.d3d11_device_type = continuous_stats.d3d11_device_type;
        query.d3d11_upload_error = continuous_stats.d3d11_upload_error;
        query.audio_codec = continuous_stats.audio_codec;
        query.audio_error = continuous_stats.audio_error;
        query.error = continuous_stats.error;
    } else {
        query.demux_started = probe_status.demux_started;
        query.video_decode_started = probe_status.video_decode_started;
        query.first_frame_decoded = probe_status.first_frame_decoded;
        query.d3d11_uploaded = probe_status.d3d11_uploaded;
        query.frame_rendered = probe_status.frame_rendered;
        query.error = probe_status.error;
    }

    query.container = probe_status.container;
    query.source_provider_kind = continuous_active
                                     ? continuous_stats.source_provider_kind
                                     : probe_status.source_provider_kind;
    query.video_codec = probe_status.video_codec;
    if (query.audio_codec.empty()) {
        query.audio_codec = probe_status.audio_codec;
    }
    query.duration_ms = probe_status.duration_ms;
    query.width = probe_status.width;
    query.height = probe_status.height;
    query.video_stream_count = probe_status.video_stream_count;
    query.audio_stream_count = probe_status.audio_stream_count;
    query.subtitle_stream_count = probe_status.subtitle_stream_count;
    const auto& p3_status = local_media_p3_handoff_.status();
    query.p3_candidate_set_created = p3_status.candidate_set_created;
    query.p3_handoff_created = p3_status.handoff_created;
    query.p3_handoff_consumed_by_p7 = p3_status.consumed_by_p7;
    query.p4_open_envelope_from_p3_created = p3_status.p4_open_envelope_created;
    query.p3_candidate_kind = p3_status.candidate_kind;
    query.p3_handoff_id = p3_status.handoff_id;
    query.p3_candidate_set_id = p3_status.candidate_set_id;
    const auto timeline = query_timeline();
    P4TimelineContractInput p4_timeline_input{};
    p4_timeline_input.session_id = session_id_;
    p4_timeline_input.playback_revision = p4_adapter_.current_playback_revision();
    p4_timeline_input.graph_snapshot_revision = state_machine_.snapshot_revision();
    p4_timeline_input.graph_state = state_machine_.state();
    p4_timeline_input.position_ms = timeline.position_ms;
    p4_timeline_input.duration_ms = timeline.duration_ms;
    p4_timeline_input.seek_in_progress = timeline.seek_in_progress;
    p4_timeline_input.last_seek_target_ms = timeline.last_seek_target_ms;
    p4_timeline_input.buffered_start_ms = timeline.buffered_range.start_ms;
    p4_timeline_input.buffered_end_ms = timeline.buffered_range.end_ms;
    p4_timeline_input.buffered_range_valid =
        timeline.buffered_range.end_ms > timeline.buffered_range.start_ms;
    p4_timeline_input.continuous_playback_active = continuous_active;
    p4_timeline_input.source_seekable = timeline.duration_ms > 0;
    const auto p4_timeline = p4_timeline_bridge_.build(p4_timeline_input);
    query.p4_timeline_contract_connected = p4_timeline.connected;
    query.p4_timeline_uses_public_contract =
        p4_timeline.uses_p4_public_timeline_state;
    query.p4_runtime_state_machine_owner =
        p4_timeline.p4_runtime_state_machine_owner;
    query.p4_timeline_revision = p4_timeline.state.revision.value;
    query.p4_timeline_generation = p4_timeline.state.generation.value;
    query.p4_timeline_commanded_position_ms =
        p4_timeline.state.commanded_position_ms.value;
    query.p4_timeline_observed_position_ms =
        p4_timeline.state.observed_position_ms.value;
    if (p4_timeline.state.duration_ms.has_value()) {
        query.p4_timeline_duration_ms =
            static_cast<std::int64_t>(p4_timeline.state.duration_ms->value);
    }
    if (p4_timeline.state.source_timeline.buffered_range.has_value()) {
        const auto& range = *p4_timeline.state.source_timeline.buffered_range;
        if (range.buffered_time_begin.has_value()) {
            query.p4_timeline_buffered_start_ms =
                range.buffered_time_begin->value;
        }
        if (range.buffered_time_end.has_value()) {
            query.p4_timeline_buffered_end_ms =
                range.buffered_time_end->value;
        }
    }
    if (continuous_active) {
        query.frame_pixels_valid = continuous_stats.frame_rendered;
        if (include_frame_pixels) {
            const auto frame = local_media_continuous_.frame_buffer();
            query.frame_pixels_valid = frame.valid;
            if (frame.valid) {
                query.width = frame.width;
                query.height = frame.height;
                query.rgba8888 = frame.rgba8888;
            }
        }
    } else {
        const auto& frame = local_media_pipeline_.frame_buffer();
        query.frame_pixels_valid = frame.valid;
        if (include_frame_pixels && frame.valid) {
            query.width = frame.width;
            query.height = frame.height;
            query.rgba8888 = frame.rgba8888;
        }
    }
    return query;
}

}  // namespace kivo::video::playback_graph::runtime
