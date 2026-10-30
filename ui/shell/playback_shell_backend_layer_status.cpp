#include "playback_shell_backend_layer_status.hpp"

#include <QString>

namespace kivo::ui::shell {

QJsonObject build_backend_layer_status(const QVariantMap& playback_query) {
    const bool p2_connected =
        playback_query.value(QStringLiteral("source_session_connected")).toBool();
    const bool p6_written =
        playback_query.value(QStringLiteral("audio_frames_written")).toULongLong() > 0;
    const auto p4_revision =
        playback_query.value(QStringLiteral("p4_playback_revision")).toULongLong();
    const auto retained_commands =
        playback_query.value(QStringLiteral("p7_retained_command_count")).toULongLong();
    const QString audio_error =
        playback_query.value(QStringLiteral("audio_error")).toString();
    const QString source_provider =
        playback_query.value(QStringLiteral("source_provider_kind")).toString();
    const bool p3_handoff_created =
        playback_query.value(QStringLiteral("p3_handoff_created")).toBool();
    const bool p3_consumed =
        playback_query.value(QStringLiteral("p3_handoff_consumed_by_p7")).toBool();
    const bool p4_open_envelope =
        playback_query.value(QStringLiteral("p4_open_envelope_from_p3_created")).toBool();
    const bool p4_timeline_connected =
        playback_query.value(QStringLiteral("p4_timeline_contract_connected")).toBool();
    const bool p4_runtime_owner =
        playback_query.value(QStringLiteral("p4_runtime_state_machine_owner")).toBool();
    const bool p5_runtime_connected =
        playback_query.value(QStringLiteral("p5_local_video_runtime_connected")).toBool();
    const bool p5_audio_boundary =
        playback_query.value(QStringLiteral("p5_audio_packet_boundary_connected")).toBool();
    const bool p5_subtitle_boundary =
        playback_query.value(QStringLiteral("p5_subtitle_packet_boundary_connected")).toBool();
    const bool p5_d3d11_uploaded =
        playback_query.value(QStringLiteral("d3d11_uploaded")).toBool() &&
        playback_query.value(QStringLiteral("d3d11_frames_uploaded")).toULongLong() > 0;
    const bool p8_embedded_ingest =
        playback_query.value(QStringLiteral("p8_embedded_packet_ingest_connected")).toBool();
    const bool p8_text_frame =
        playback_query.value(QStringLiteral("subtitle_frame_available")).toBool() &&
        !playback_query.value(QStringLiteral("subtitle_primary")).toString().isEmpty();
    const bool p8_pgs_render =
        playback_query.value(QStringLiteral("p8_pgs_bitmap_render_connected")).toBool();
    const auto subtitle_stream_count =
        playback_query.value(QStringLiteral("subtitle_stream_count")).toULongLong();

    QJsonObject layer_status;
    layer_status.insert(QStringLiteral("p0_contract_governance_applies"), true);
    layer_status.insert(QStringLiteral("p0_status"),
                        QStringLiteral("audited: governs contract registry, schema/versioning, "
                                       "claim safety, diagnostics, forbidden include gates"));
    layer_status.insert(QStringLiteral("p1_fake_backend_governance_applies"), true);
    layer_status.insert(QStringLiteral("p1_status"),
                        QStringLiteral("audited: fake backend/state machine/gate discipline apply "
                                       "as governance, not real playback data plane"));
    layer_status.insert(QStringLiteral("p1_governance_gate"),
                        QStringLiteral("FAIL: verify_p1_gates.py stops on existing ignored "
                                       "root-level cmake_p5d_build.log"));
    layer_status.insert(QStringLiteral("p2_source_session_connected"), p2_connected);
    layer_status.insert(QStringLiteral("p2_source_provider_kind"), source_provider);
    layer_status.insert(QStringLiteral("p2_status"),
                        p2_connected
                            ? QStringLiteral("connected: FFmpeg input reads through P2 "
                                             "SourceSession read/seek callbacks")
                            : QStringLiteral("deferred: no P2 SourceSession-backed input "
                                             "observed in playback query"));
    layer_status.insert(QStringLiteral("p3_contract_family_present"), true);
    layer_status.insert(QStringLiteral("p3_contract_tests_passed_locally"), true);
    layer_status.insert(QStringLiteral("p3_candidate_set_created"),
                        playback_query.value(QStringLiteral("p3_candidate_set_created")).toBool());
    layer_status.insert(QStringLiteral("p3_handoff_created"), p3_handoff_created);
    layer_status.insert(QStringLiteral("p3_handoff_consumed_by_p7"), p3_consumed);
    layer_status.insert(QStringLiteral("p3_candidate_kind"),
                        playback_query.value(QStringLiteral("p3_candidate_kind")).toString());
    layer_status.insert(QStringLiteral("p3_playback_ready_input_bridge_connected"),
                        p3_handoff_created && p3_consumed && p4_open_envelope);
    layer_status.insert(QStringLiteral("p3_status"),
                        (p3_handoff_created && p3_consumed && p4_open_envelope)
                            ? QStringLiteral("connected: local media open creates P3 direct-play "
                                             "candidate/handoff and P7 consumes its P4 open envelope")
                            : QStringLiteral("deferred: P3 contracts exist, but no consumed "
                                             "handoff/candidate envelope observed in playback query"));
    layer_status.insert(QStringLiteral("p7_session_commands_connected"), true);
    layer_status.insert(QStringLiteral("p4_control_plane_contract_family_present"), true);
    layer_status.insert(QStringLiteral("p4_control_plane_contract_tests_passed_locally"), true);
    layer_status.insert(QStringLiteral("p4_open_envelope_from_p3_created"), p4_open_envelope);
    layer_status.insert(QStringLiteral("p4_command_lifecycle_connected"),
                        retained_commands > 0);
    layer_status.insert(QStringLiteral("p4_timeline_contract_connected"),
                        p4_timeline_connected);
    layer_status.insert(QStringLiteral("p4_timeline_uses_public_contract"),
                        playback_query.value(
                            QStringLiteral("p4_timeline_uses_public_contract")).toBool());
    layer_status.insert(QStringLiteral("p4_timeline_revision"),
                        static_cast<qint64>(
                            playback_query.value(
                                QStringLiteral("p4_timeline_revision")).toULongLong()));
    layer_status.insert(QStringLiteral("p4_timeline_observed_position_ms"),
                        playback_query.value(
                            QStringLiteral("p4_timeline_observed_position_ms")).toLongLong());
    layer_status.insert(QStringLiteral("p4_timeline_duration_ms"),
                        playback_query.value(
                            QStringLiteral("p4_timeline_duration_ms")).toLongLong());
    layer_status.insert(QStringLiteral("p4_playback_revision_committed"),
                        p4_revision > 0);
    layer_status.insert(QStringLiteral("p4_playback_revision"),
                        static_cast<qint64>(p4_revision));
    layer_status.insert(QStringLiteral("p4_runtime_state_machine_owner"), p4_runtime_owner);
    layer_status.insert(QStringLiteral("p4_full_state_sync_connected"), p4_runtime_owner);
    layer_status.insert(QStringLiteral("p4_status"),
                        retained_commands > 0 && p4_timeline_connected
                            ? QStringLiteral("partial: P7 command lifecycle allocates P4 command "
                                             "ids, commits playback revision evidence, and maps "
                                             "the local timeline into P4 public timeline "
                                             "contracts; full P4 runtime state/timeline ownership "
                                             "is still deferred")
                            : retained_commands > 0
                            ? QStringLiteral("partial: P7 command lifecycle allocates P4 command "
                                             "ids and commits playback revision evidence; full P4 "
                                             "state/timeline ownership deferred")
                            : QStringLiteral("deferred: no P4 command lifecycle evidence "
                                             "observed in playback query"));
    layer_status.insert(QStringLiteral("p5_contract_family_present"), true);
    layer_status.insert(QStringLiteral("p5_production_runtime_target_present"), true);
    layer_status.insert(QStringLiteral("p5_formal_video_plane_connected"),
                        p5_runtime_connected);
    layer_status.insert(QStringLiteral("p5_continuous_d3d11_upload_connected"),
                        p5_d3d11_uploaded);
    layer_status.insert(QStringLiteral("p5_hardware_presenter_connected"), false);
    layer_status.insert(QStringLiteral("p5_presentation_path"),
                        QStringLiteral("qt_scenegraph_texture"));
    layer_status.insert(QStringLiteral("p5_d3d11_frames_uploaded"),
                        static_cast<qint64>(
                            playback_query.value(
                                QStringLiteral("d3d11_frames_uploaded")).toULongLong()));
    layer_status.insert(QStringLiteral("p5_d3d11_upload_failures"),
                        static_cast<qint64>(
                            playback_query.value(
                                QStringLiteral("d3d11_upload_failures")).toULongLong()));
    layer_status.insert(QStringLiteral("p5_d3d11_device_type"),
                        playback_query.value(QStringLiteral("d3d11_device_type")).toString());
    layer_status.insert(QStringLiteral("p5_d3d11_upload_error"),
                        playback_query.value(QStringLiteral("d3d11_upload_error")).toString());
    layer_status.insert(QStringLiteral("p5_audio_packet_boundary_connected"),
                        p5_audio_boundary);
    layer_status.insert(QStringLiteral("p5_subtitle_packet_boundary_connected"),
                        p5_subtitle_boundary);
    layer_status.insert(QStringLiteral("p5_audio_packets_dispatched"),
                        static_cast<qint64>(
                            playback_query.value(
                                QStringLiteral("p5_audio_packets_dispatched")).toULongLong()));
    layer_status.insert(QStringLiteral("p5_subtitle_packets_dispatched"),
                        static_cast<qint64>(
                            playback_query.value(
                                QStringLiteral("p5_subtitle_packets_dispatched")).toULongLong()));
    layer_status.insert(QStringLiteral("p5_current_video_path"),
                        p5_runtime_connected
                            ? QStringLiteral("P7 coordinator -> P5 local media runtime -> "
                                             "P2 SourceSession + FFmpeg demux/video decode/RGBA "
                                             "+ continuous D3D11 texture upload; presentation "
                                             "uses Qt scene graph texture nodes")
                            : QStringLiteral("not observed"));
    layer_status.insert(QStringLiteral("p5_status"),
                        p5_runtime_connected
                            ? QStringLiteral("connected: production P5 local media runtime target "
                                             "owns real SourceSession-backed demux/video decode/RGBA "
                                             "for this local/UNC path; continuous D3D11 texture "
                                             "upload and Qt scene graph texture presentation are "
                                             "evidenced when available, while swapchain presenter, "
                                             "hardware frame pacing, and advanced GPU/HDR closure "
                                             "remain deferred")
                            : QStringLiteral("deferred: P5 contracts exist, but no production P5 "
                                             "video runtime evidence was observed in playback query"));
    layer_status.insert(QStringLiteral("p6_audio_connected"), p6_written);
    layer_status.insert(QStringLiteral("p6_audio_decode_started"),
                        playback_query.value(QStringLiteral("audio_decode_started")).toBool());
    layer_status.insert(QStringLiteral("p6_audio_output_started"),
                        playback_query.value(QStringLiteral("audio_output_started")).toBool());
    layer_status.insert(QStringLiteral("p6_audio_frames_written"),
                        static_cast<qint64>(
                            playback_query.value(
                                QStringLiteral("audio_frames_written")).toULongLong()));
    layer_status.insert(QStringLiteral("p6_status"),
                        p6_written
                            ? QStringLiteral("connected: P7 dispatches audio packets to P6 "
                                             "local PCM runtime and WASAPI accepted PCM writes")
                            : (audio_error.isEmpty()
                                   ? QStringLiteral("deferred: P6 audio bridge has no PCM write "
                                                    "evidence in this path")
                                   : QStringLiteral("failed: ") + audio_error));
    layer_status.insert(QStringLiteral("p8_contract_family_present"), true);
    layer_status.insert(QStringLiteral("p8_subtitle_track_inventory_discovered"),
                        subtitle_stream_count > 0);
    layer_status.insert(QStringLiteral("p8_subtitle_stream_count"),
                        static_cast<qint64>(subtitle_stream_count));
    layer_status.insert(QStringLiteral("p8_subtitle_connected"),
                        p8_embedded_ingest && p8_text_frame);
    layer_status.insert(QStringLiteral("p8_text_frame_connected"),
                        p8_embedded_ingest && p8_text_frame);
    layer_status.insert(QStringLiteral("p8_full_subtitle_render_connected"), false);
    layer_status.insert(QStringLiteral("p8_embedded_packet_ingest_connected"),
                        p8_embedded_ingest);
    layer_status.insert(QStringLiteral("p8_text_buffer_fixture_only"), !p8_embedded_ingest);
    layer_status.insert(QStringLiteral("p8_pgs_bitmap_render_connected"), p8_pgs_render);
    layer_status.insert(QStringLiteral("p8_subtitle_packets_submitted"),
                        static_cast<qint64>(
                            playback_query.value(
                                QStringLiteral("p8_subtitle_packets_submitted")).toULongLong()));
    layer_status.insert(QStringLiteral("p8_unsupported_feature_count"),
                        static_cast<qint64>(
                            playback_query.value(
                                QStringLiteral("p8_unsupported_feature_count")).toULongLong()));
    layer_status.insert(QStringLiteral("p8_real_local_bridge_attached"), p8_embedded_ingest);
    layer_status.insert(QStringLiteral("p8_status"),
                        p8_embedded_ingest && p8_text_frame
                            ? QStringLiteral("partial: real embedded text subtitle packets reach "
                                             "P8 and produce a text frame through P7/QML; PGS "
                                             "bitmap rendering and full subtitle composition remain "
                                             "deferred")
                            : p8_embedded_ingest
                            ? QStringLiteral("partial: real embedded subtitle packets reach P8 "
                                             "through P7; PGS bitmap rendering remains deferred, "
                                             "so subtitle frame display is not claimed")
                            : QStringLiteral("deferred: P8 contracts exist and subtitle tracks may be "
                                             "inventoried, but real embedded packet/cue ingest and PGS "
                                             "bitmap rendering are not attached for this local/UNC path; "
                                             "fixture text is forbidden for real media"));
    layer_status.insert(QStringLiteral("shell_core_playback_loop"), false);
    return layer_status;
}

}  // namespace kivo::ui::shell
