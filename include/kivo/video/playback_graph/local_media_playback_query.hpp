#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace kivo::video::playback_graph {

struct LocalMediaPlaybackQuery {
    bool active{false};
    bool source_session_connected{false};
    bool source_opened{false};
    bool demux_started{false};
    bool video_decode_started{false};
    bool audio_decode_started{false};
    bool audio_output_started{false};
    bool first_frame_decoded{false};
    bool d3d11_uploaded{false};
    bool d3d11_upload_available{false};
    bool d3d11_upload_initialized{false};
    bool frame_rendered{false};
    bool frame_pixels_valid{false};
    bool continuous_rendering{false};
    std::uint64_t decoded_frame_count{0};
    std::uint64_t rendered_frame_count{0};
    std::uint64_t d3d11_frames_uploaded{0};
    std::uint64_t d3d11_upload_failures{0};
    std::uint64_t d3d11_upload_last_elapsed_ms{0};
    std::uint64_t d3d11_upload_max_elapsed_ms{0};
    std::uint64_t audio_packets_submitted{0};
    std::uint64_t audio_frames_decoded{0};
    std::uint64_t audio_frames_written{0};
    std::uint64_t audio_flush_count{0};
    std::uint64_t audio_writer_reset_count{0};
    std::uint64_t audio_stale_packets_discarded{0};
    std::uint64_t p7_background_pump_count{0};
    std::uint64_t p7_background_pump_last_elapsed_ms{0};
    std::uint64_t p7_background_pump_max_elapsed_ms{0};
    std::uint64_t p7_background_pump_total_elapsed_ms{0};
    std::uint64_t p7_background_pump_last_new_frames{0};
    std::uint64_t p7_background_pump_packet_budget{0};
    std::uint64_t p7_background_pump_max_packet_budget{0};
    bool p7_seek_preroll_active{false};
    std::uint64_t p7_seek_preroll_completed_count{0};
    std::uint64_t p7_seek_preroll_last_elapsed_ms{0};
    std::uint64_t p7_seek_preroll_max_elapsed_ms{0};
    bool p3_candidate_set_created{false};
    bool p3_handoff_created{false};
    bool p3_handoff_consumed_by_p7{false};
    bool p4_open_envelope_from_p3_created{false};
    bool p5_local_video_runtime_connected{false};
    bool p5_audio_packet_boundary_connected{false};
    bool p5_subtitle_packet_boundary_connected{false};
    bool p8_embedded_packet_ingest_connected{false};
    bool p8_pgs_bitmap_render_connected{false};
    bool p4_timeline_contract_connected{false};
    bool p4_timeline_uses_public_contract{false};
    bool p4_runtime_state_machine_owner{false};
    std::string p3_candidate_kind;
    std::string p3_handoff_id;
    std::string p3_candidate_set_id;
    std::int64_t playback_elapsed_ms{0};
    std::uint64_t p4_timeline_revision{0};
    std::uint64_t p4_timeline_generation{0};
    std::int64_t p4_timeline_commanded_position_ms{0};
    std::int64_t p4_timeline_observed_position_ms{0};
    std::int64_t p4_timeline_duration_ms{0};
    std::int64_t p4_timeline_buffered_start_ms{0};
    std::int64_t p4_timeline_buffered_end_ms{0};
    std::uint64_t last_frame_signature{0};
    std::int64_t last_frame_time_us{0};
    std::int64_t last_frame_duration_us{0};
    std::uint64_t p5_audio_packets_dispatched{0};
    std::uint64_t p5_subtitle_packets_dispatched{0};
    std::uint64_t p8_subtitle_packets_submitted{0};
    std::uint64_t p8_unsupported_feature_count{0};
    std::string container;
    std::string source_provider_kind;
    std::string d3d11_device_type;
    std::string d3d11_upload_error;
    std::string video_codec;
    std::string audio_codec;
    std::string audio_error;
    std::string error;
    std::int64_t duration_ms{0};
    std::int32_t width{0};
    std::int32_t height{0};
    std::uint64_t video_stream_count{0};
    std::uint64_t audio_stream_count{0};
    std::uint64_t subtitle_stream_count{0};
    std::shared_ptr<const std::vector<std::uint8_t>> rgba8888;
};

}  // namespace kivo::video::playback_graph
