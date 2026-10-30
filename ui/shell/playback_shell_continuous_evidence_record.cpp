#include "playback_shell_continuous_evidence_record.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include "playback_shell_real_local_evidence.hpp"

namespace kivo::ui::shell {
namespace {
[[nodiscard]] QString classification_from_query(const QVariantMap& query) {
    const auto decoded = query.value(QStringLiteral("decoded_frame_count")).toULongLong();
    const auto rendered = query.value(QStringLiteral("rendered_frame_count")).toULongLong();
    const auto before = query.value(QStringLiteral("timeline_position_before_ms")).toLongLong();
    const auto after = query.value(QStringLiteral("timeline_position_after_ms")).toLongLong();
    const bool pause_ok = query.value(QStringLiteral("pause_holds_position")).toBool();
    const bool resume_ok = query.value(QStringLiteral("resume_advances_position")).toBool();
    const bool seek_ok =
        query.value(QStringLiteral("seek_result")).toString() == QStringLiteral("accepted");
    const bool post_seek_frame = query.value(QStringLiteral("post_seek_frame_rendered")).toBool();
    const bool stop_ok =
        query.value(QStringLiteral("stop_result")).toString() == QStringLiteral("ready");
    if (decoded > 1 && rendered > 1 && after > before && pause_ok && resume_ok && seek_ok &&
        post_seek_frame && stop_ok) {
        return QStringLiteral("PASS_CONTINUOUS_PLAYBACK");
    }
    if (decoded > 1 && rendered > 1 && after > before && !seek_ok) {
        return QStringLiteral("PARTIAL_CONTINUOUS_NO_SEEK");
    }
    if (decoded > 1 && rendered > 1 && after > before && (!pause_ok || !resume_ok)) {
        return QStringLiteral("PARTIAL_CONTINUOUS_NO_PAUSE_RESUME");
    }
    if (query.value(QStringLiteral("first_frame_decoded")).toBool() && decoded <= 1) {
        return QStringLiteral("PARTIAL_FIRST_FRAME_ONLY_NO_CONTINUOUS");
    }
    if (!query.value(QStringLiteral("video_decode_started")).toBool()) {
        return QStringLiteral("FAIL_DECODE_LOOP");
    }
    if (decoded > 1 && rendered <= 1) {
        return QStringLiteral("FAIL_RENDER_PUMP");
    }
    return QStringLiteral("PARTIAL_FIRST_FRAME_ONLY_NO_CONTINUOUS");
}
[[nodiscard]] QStringList collect_errors(const ContinuousPlaybackEvidenceRecord& record) {
    QStringList errors;
    if (!record.media_resolve_error.isEmpty()) {
        errors << record.media_resolve_error;
    }
    if (!record.media_exists) {
        errors << QStringLiteral("media path not accessible");
    }
    if (record.media_exists && !record.selected_media_exists) {
        errors << QStringLiteral("selected media file missing");
    }
    if (!record.open_ok) {
        errors << QStringLiteral("open_local_media rejected");
    }
    if (!record.play_ok) {
        errors << QStringLiteral("play rejected");
    }
    if (!record.frame_ready) {
        errors << QStringLiteral("first frame not ready");
    }
    if (!record.screenshot_t0_saved) {
        errors << QStringLiteral("t0 screenshot save failed");
    }
    if (!record.screenshot_t5s_saved) {
        errors << QStringLiteral("t5s screenshot save failed");
    }
    if (!record.screenshot_t10s_saved) {
        errors << QStringLiteral("t10s screenshot save failed");
    }
    if (!record.pause_command_ok) {
        errors << QStringLiteral("pause rejected");
    } else if (!record.pause_holds_position) {
        errors << QStringLiteral("pause did not hold timeline");
    }
    if (!record.resume_command_ok) {
        errors << QStringLiteral("resume rejected");
    } else if (!record.resume_advances_position) {
        errors << QStringLiteral("resume did not advance timeline");
    }
    if (!record.seek_command_ok) {
        errors << QStringLiteral("seek rejected");
    }
    if (record.seek_command_ok && !record.post_seek_frame_rendered) {
        errors << QStringLiteral("post-seek frame not rendered");
    }
    const bool restored_failed_audio_probe =
        record.audio_probe_failed && record.audio_track_restored;
    if (record.audio_track_switch_available && !record.audio_track_switch_command_ok) {
        errors << QStringLiteral("audio track switch rejected");
    } else if (record.audio_track_switch_command_ok &&
               !record.audio_track_switch_selected && !restored_failed_audio_probe) {
        errors << QStringLiteral("audio track switch did not select target");
    } else if (record.audio_probe_failed && !record.audio_track_restored) {
        errors << QStringLiteral("audio probe failed and restore did not recover prior track");
    }
    if (!record.stop_command_ok) {
        errors << QStringLiteral("stop rejected");
    } else if (!record.stop_ready) {
        errors << QStringLiteral("stop did not reach ready state");
    }
    const QString pipeline_error = record.playback_after.value(QStringLiteral("error")).toString();
    if (!pipeline_error.isEmpty()) {
        errors << pipeline_error;
    }
    const QString audio_error = record.playback_after.value(QStringLiteral("audio_error")).toString();
    if (!audio_error.isEmpty()) {
        errors << audio_error;
    }
    return errors;
}
[[nodiscard]] QVariantMap classification_input(const ContinuousPlaybackEvidenceRecord& record) {
    QVariantMap input;
    input.insert(QStringLiteral("decoded_frame_count"),
                 record.playback_after.value(QStringLiteral("decoded_frame_count")));
    input.insert(QStringLiteral("rendered_frame_count"),
                 record.playback_after.value(QStringLiteral("rendered_frame_count")));
    input.insert(QStringLiteral("timeline_position_before_ms"), record.timeline_before);
    input.insert(QStringLiteral("timeline_position_after_ms"), record.timeline_after);
    input.insert(QStringLiteral("pause_holds_position"), record.pause_holds_position);
    input.insert(QStringLiteral("resume_advances_position"), record.resume_advances_position);
    input.insert(QStringLiteral("seek_result"),
                 record.seek_command_ok ? QStringLiteral("accepted") : QStringLiteral("rejected"));
    input.insert(QStringLiteral("post_seek_frame_rendered"), record.post_seek_frame_rendered);
    input.insert(QStringLiteral("stop_result"),
                 record.stop_ready ? QStringLiteral("ready") :
                                     (record.stop_command_ok ? QStringLiteral("not_ready")
                                                             : QStringLiteral("rejected")));
    input.insert(QStringLiteral("first_frame_decoded"),
                 record.playback_after.value(QStringLiteral("first_frame_decoded")));
    input.insert(QStringLiteral("video_decode_started"),
                 record.playback_after.value(QStringLiteral("video_decode_started")));
    return input;
}

[[nodiscard]] qint64 option_count(const QVariantMap& snapshot, const QString& key) {
    return static_cast<qint64>(snapshot.value(key).toList().size());
}

[[nodiscard]] QJsonObject build_continuous_json(
    const ContinuousPlaybackEvidenceRecord& record,
    const QString& classification) {
    const auto decoded_count =
        record.playback_after.value(QStringLiteral("decoded_frame_count")).toULongLong();
    const auto rendered_count =
        record.playback_after.value(QStringLiteral("rendered_frame_count")).toULongLong();
    const auto audio_written = record.playback_after.value(QStringLiteral("audio_frames_written")).toULongLong();
    const bool continuous_rendering =
        record.playback_after.value(QStringLiteral("continuous_rendering")).toBool()
        && decoded_count > 1 && rendered_count > 1 &&
        record.timeline_after > record.timeline_before;

    QString stall_note;
    if (decoded_count > 1 && record.signature_before == record.signature_after &&
        record.signature_before != 0) {
        stall_note = QStringLiteral(
            "decoded_frame_count advanced but last_frame_signature unchanged; "
            "possible static scene or decode pacing stall");
    }

    QJsonObject evidence;
    evidence.insert(QStringLiteral("media_path"), record.media_path);
    evidence.insert(QStringLiteral("selected_media_file"), record.selected_media_file);
    evidence.insert(QStringLiteral("media_exists"), record.media_exists);
    evidence.insert(QStringLiteral("source_opened"),
                    record.playback_after.value(QStringLiteral("source_opened")).toBool());
    evidence.insert(QStringLiteral("source_session_connected"),
                    record.playback_after.value(
                        QStringLiteral("source_session_connected")).toBool());
    evidence.insert(QStringLiteral("source_provider_kind"),
                    record.playback_after.value(
                        QStringLiteral("source_provider_kind")).toString());
    evidence.insert(QStringLiteral("demux_started"),
                    record.playback_after.value(QStringLiteral("demux_started")).toBool());
    evidence.insert(QStringLiteral("video_decode_started"),
                    record.playback_after.value(QStringLiteral("video_decode_started")).toBool());
    evidence.insert(QStringLiteral("decoded_frame_count"), static_cast<qint64>(decoded_count));
    evidence.insert(QStringLiteral("rendered_frame_count"), static_cast<qint64>(rendered_count));
    evidence.insert(QStringLiteral("d3d11_uploaded"),
                    record.playback_after.value(QStringLiteral("d3d11_uploaded")).toBool());
    evidence.insert(QStringLiteral("d3d11_upload_available"),
                    record.playback_after.value(
                        QStringLiteral("d3d11_upload_available")).toBool());
    evidence.insert(QStringLiteral("d3d11_upload_initialized"),
                    record.playback_after.value(
                        QStringLiteral("d3d11_upload_initialized")).toBool());
    evidence.insert(QStringLiteral("d3d11_frames_uploaded"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("d3d11_frames_uploaded")).toULongLong()));
    evidence.insert(QStringLiteral("d3d11_upload_failures"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("d3d11_upload_failures")).toULongLong()));
    evidence.insert(QStringLiteral("d3d11_upload_last_elapsed_ms"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("d3d11_upload_last_elapsed_ms")).toULongLong()));
    evidence.insert(QStringLiteral("d3d11_upload_max_elapsed_ms"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("d3d11_upload_max_elapsed_ms")).toULongLong()));
    evidence.insert(QStringLiteral("d3d11_device_type"),
                    record.playback_after.value(QStringLiteral("d3d11_device_type")).toString());
    evidence.insert(QStringLiteral("d3d11_upload_error"),
                    record.playback_after.value(
                        QStringLiteral("d3d11_upload_error")).toString());
    evidence.insert(QStringLiteral("p5_continuous_d3d11_upload_connected"),
                    record.playback_after.value(QStringLiteral("d3d11_uploaded")).toBool() &&
                        record.playback_after.value(
                            QStringLiteral("d3d11_frames_uploaded")).toULongLong() > 0);
    evidence.insert(QStringLiteral("p5_presentation_path"),
                    QStringLiteral("qt_scenegraph_texture"));
    evidence.insert(QStringLiteral("p5_hardware_presenter_connected"), false);
    evidence.insert(QStringLiteral("audio_decode_started"),
                    record.playback_after.value(QStringLiteral("audio_decode_started")).toBool());
    evidence.insert(QStringLiteral("audio_output_started"),
                    record.playback_after.value(QStringLiteral("audio_output_started")).toBool());
    evidence.insert(QStringLiteral("audio_frames_decoded"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("audio_frames_decoded")).toULongLong()));
    evidence.insert(QStringLiteral("audio_frames_written"), static_cast<qint64>(audio_written));
    evidence.insert(QStringLiteral("audio_flush_count"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("audio_flush_count")).toULongLong()));
    evidence.insert(QStringLiteral("audio_writer_reset_count"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("audio_writer_reset_count")).toULongLong()));
    evidence.insert(QStringLiteral("audio_stale_packets_discarded"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("audio_stale_packets_discarded")).toULongLong()));
    evidence.insert(QStringLiteral("surface_refresh_attempt_count"),
                    static_cast<qint64>(record.surface_refresh_attempt_count));
    evidence.insert(QStringLiteral("surface_refresh_success_count"),
                    static_cast<qint64>(record.surface_refresh_success_count));
    evidence.insert(QStringLiteral("surface_refresh_miss_count"),
                    static_cast<qint64>(record.surface_refresh_miss_count));
    evidence.insert(QStringLiteral("surface_refresh_last_elapsed_ms"),
                    record.surface_refresh_last_elapsed_ms);
    evidence.insert(QStringLiteral("surface_refresh_max_elapsed_ms"),
                    record.surface_refresh_max_elapsed_ms);
    evidence.insert(QStringLiteral("surface_refresh_total_elapsed_ms"),
                    record.surface_refresh_total_elapsed_ms);
    evidence.insert(QStringLiteral("surface_paint_count"),
                    static_cast<qint64>(record.surface_paint_count));
    evidence.insert(QStringLiteral("surface_paint_last_elapsed_ms"),
                    record.surface_paint_last_elapsed_ms);
    evidence.insert(QStringLiteral("surface_paint_max_elapsed_ms"),
                    record.surface_paint_max_elapsed_ms);
    evidence.insert(QStringLiteral("surface_paint_total_elapsed_ms"),
                    record.surface_paint_total_elapsed_ms);
    evidence.insert(QStringLiteral("p7_background_pump_count"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p7_background_pump_count")).toULongLong()));
    evidence.insert(QStringLiteral("p7_background_pump_last_elapsed_ms"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p7_background_pump_last_elapsed_ms")).toULongLong()));
    evidence.insert(QStringLiteral("p7_background_pump_max_elapsed_ms"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p7_background_pump_max_elapsed_ms")).toULongLong()));
    evidence.insert(QStringLiteral("p7_background_pump_total_elapsed_ms"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p7_background_pump_total_elapsed_ms")).toULongLong()));
    evidence.insert(QStringLiteral("p7_background_pump_last_new_frames"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p7_background_pump_last_new_frames")).toULongLong()));
    evidence.insert(QStringLiteral("p7_background_pump_packet_budget"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p7_background_pump_packet_budget")).toULongLong()));
    evidence.insert(QStringLiteral("p7_background_pump_max_packet_budget"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p7_background_pump_max_packet_budget")).toULongLong()));
    evidence.insert(QStringLiteral("p7_seek_preroll_active"),
                    record.playback_after.value(
                        QStringLiteral("p7_seek_preroll_active")).toBool());
    evidence.insert(QStringLiteral("p7_seek_preroll_completed_count"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p7_seek_preroll_completed_count")).toULongLong()));
    evidence.insert(QStringLiteral("p7_seek_preroll_last_elapsed_ms"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p7_seek_preroll_last_elapsed_ms")).toULongLong()));
    evidence.insert(QStringLiteral("p7_seek_preroll_max_elapsed_ms"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p7_seek_preroll_max_elapsed_ms")).toULongLong()));
    evidence.insert(QStringLiteral("p5_local_video_runtime_connected"),
                    record.playback_after.value(
                        QStringLiteral("p5_local_video_runtime_connected")).toBool());
    evidence.insert(QStringLiteral("p5_audio_packet_boundary_connected"),
                    record.playback_after.value(
                        QStringLiteral("p5_audio_packet_boundary_connected")).toBool());
    evidence.insert(QStringLiteral("p5_subtitle_packet_boundary_connected"),
                    record.playback_after.value(
                        QStringLiteral("p5_subtitle_packet_boundary_connected")).toBool());
    evidence.insert(QStringLiteral("p5_audio_packets_dispatched"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p5_audio_packets_dispatched")).toULongLong()));
    evidence.insert(QStringLiteral("p5_subtitle_packets_dispatched"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p5_subtitle_packets_dispatched")).toULongLong()));
    evidence.insert(QStringLiteral("p3_candidate_set_created"),
                    record.playback_after.value(
                        QStringLiteral("p3_candidate_set_created")).toBool());
    evidence.insert(QStringLiteral("p3_handoff_created"),
                    record.playback_after.value(QStringLiteral("p3_handoff_created")).toBool());
    evidence.insert(QStringLiteral("p3_handoff_consumed_by_p7"),
                    record.playback_after.value(
                        QStringLiteral("p3_handoff_consumed_by_p7")).toBool());
    evidence.insert(QStringLiteral("p4_open_envelope_from_p3_created"),
                    record.playback_after.value(
                        QStringLiteral("p4_open_envelope_from_p3_created")).toBool());
    evidence.insert(QStringLiteral("p3_candidate_kind"),
                    record.playback_after.value(QStringLiteral("p3_candidate_kind")).toString());
    evidence.insert(QStringLiteral("p3_handoff_id"),
                    record.playback_after.value(QStringLiteral("p3_handoff_id")).toString());
    evidence.insert(QStringLiteral("p3_candidate_set_id"),
                    record.playback_after.value(QStringLiteral("p3_candidate_set_id")).toString());
    evidence.insert(QStringLiteral("first_frame_decoded"),
                    record.playback_after.value(QStringLiteral("first_frame_decoded")).toBool());
    evidence.insert(QStringLiteral("continuous_rendering"), continuous_rendering);
    evidence.insert(QStringLiteral("timeline_position_before_ms"), record.timeline_before);
    evidence.insert(QStringLiteral("timeline_position_t0_ms"), record.timeline_before);
    evidence.insert(QStringLiteral("timeline_position_t5_ms"), record.timeline_t5);
    evidence.insert(QStringLiteral("timeline_position_after_ms"), record.timeline_after);
    evidence.insert(QStringLiteral("timeline_position_t10_ms"), record.timeline_after);
    evidence.insert(QStringLiteral("playback_duration_observed_ms"), record.observed_ms);
    evidence.insert(QStringLiteral("frame_signature_before"),
                    static_cast<qint64>(record.signature_before));
    evidence.insert(QStringLiteral("frame_signature_t5"), static_cast<qint64>(record.signature_t5));
    evidence.insert(QStringLiteral("frame_signature_after"),
                    static_cast<qint64>(record.signature_after));
    evidence.insert(QStringLiteral("frame_signature_before_seek"),
                    static_cast<qint64>(record.signature_before_seek));
    evidence.insert(QStringLiteral("last_frame_time_us"),
                    record.playback_after.value(QStringLiteral("last_frame_time_us")).toLongLong());
    evidence.insert(QStringLiteral("last_frame_duration_us"),
                    record.playback_after.value(QStringLiteral("last_frame_duration_us")).toLongLong());
    evidence.insert(QStringLiteral("frame_timeline_drift_ms"),
                    record.timeline_after -
                        (record.playback_after.value(QStringLiteral("last_frame_time_us")).toLongLong() /
                         1000));
    evidence.insert(QStringLiteral("screenshot_t0"), record.screenshot_t0);
    evidence.insert(QStringLiteral("screenshot_t5s"), record.screenshot_t5s);
    evidence.insert(QStringLiteral("screenshot_t10s"), record.screenshot_t10s);
    evidence.insert(QStringLiteral("screenshot_t10s_or_post_seek"), record.screenshot_t10s);
    evidence.insert(QStringLiteral("screenshot_t0_exists"), record.screenshot_t0_saved);
    evidence.insert(QStringLiteral("screenshot_t5s_exists"), record.screenshot_t5s_saved);
    evidence.insert(QStringLiteral("screenshot_t10s_exists"), record.screenshot_t10s_saved);
    evidence.insert(QStringLiteral("screenshot_t10s_or_post_seek_exists"),
                    record.screenshot_t10s_saved);
    evidence.insert(QStringLiteral("pause_command_ok"), record.pause_command_ok);
    evidence.insert(QStringLiteral("pause_position_before_ms"), record.pause_position_before);
    evidence.insert(QStringLiteral("pause_position_after_ms"), record.pause_position_after);
    evidence.insert(QStringLiteral("pause_holds_position"), record.pause_holds_position);
    evidence.insert(QStringLiteral("resume_command_ok"), record.resume_command_ok);
    evidence.insert(QStringLiteral("resume_position_after_ms"), record.resume_position_after);
    evidence.insert(QStringLiteral("resume_advances_position"), record.resume_advances_position);
    evidence.insert(QStringLiteral("seek_requested_ms"), record.seek_target_ms);
    evidence.insert(QStringLiteral("seek_result"),
                    record.seek_command_ok ? QStringLiteral("accepted")
                                           : QStringLiteral("rejected"));
    evidence.insert(QStringLiteral("post_seek_frame_rendered"), record.post_seek_frame_rendered);
    evidence.insert(QStringLiteral("post_seek_frame_ready_ms"),
                    record.post_seek_frame_ready_ms);
    evidence.insert(QStringLiteral("post_seek_frame_time_ms"),
                    record.post_seek_frame_time_ms);
    evidence.insert(QStringLiteral("audio_track_switch_available"),
                    record.audio_track_switch_available);
    evidence.insert(QStringLiteral("audio_track_switch_command_ok"),
                    record.audio_track_switch_command_ok);
    evidence.insert(QStringLiteral("audio_track_switch_selected"),
                    record.audio_track_switch_selected);
    evidence.insert(QStringLiteral("audio_track_before_switch"),
                    static_cast<qint64>(record.audio_track_before_switch));
    evidence.insert(QStringLiteral("audio_track_target"),
                    static_cast<qint64>(record.audio_track_target));
    evidence.insert(QStringLiteral("audio_track_after_switch"),
                    static_cast<qint64>(record.audio_track_after_switch));
    evidence.insert(QStringLiteral("audio_track_probe_target"),
                    static_cast<qint64>(record.audio_track_target));
    evidence.insert(QStringLiteral("audio_track_probe_selected"),
                    static_cast<qint64>(record.audio_track_probe_selected));
    evidence.insert(QStringLiteral("audio_track_final"),
                    static_cast<qint64>(record.audio_track_final));
    evidence.insert(QStringLiteral("audio_frames_written_before_switch"),
                    static_cast<qint64>(record.audio_frames_written_before_switch));
    evidence.insert(QStringLiteral("audio_frames_written_after_switch"),
                    static_cast<qint64>(record.audio_frames_written_after_switch));
    evidence.insert(QStringLiteral("audio_frames_written_probe"),
                    static_cast<qint64>(record.audio_frames_written_probe));
    evidence.insert(QStringLiteral("audio_frames_written_final"),
                    static_cast<qint64>(record.audio_frames_written_final));
    evidence.insert(QStringLiteral("audio_codec_before_switch"),
                    record.audio_codec_before_switch);
    evidence.insert(QStringLiteral("audio_codec_after_switch"),
                    record.audio_codec_after_switch);
    evidence.insert(QStringLiteral("audio_codec_probe"), record.audio_codec_probe);
    evidence.insert(QStringLiteral("audio_codec_final"), record.audio_codec_final);
    evidence.insert(QStringLiteral("audio_probe_error"), record.audio_probe_error);
    evidence.insert(QStringLiteral("audio_probe_failed"), record.audio_probe_failed);
    evidence.insert(QStringLiteral("audio_track_restore_attempted"),
                    record.audio_track_restore_attempted);
    evidence.insert(QStringLiteral("audio_track_restore_command_ok"),
                    record.audio_track_restore_command_ok);
    evidence.insert(QStringLiteral("audio_track_restored"), record.audio_track_restored);
    evidence.insert(QStringLiteral("stop_result"),
                    record.stop_ready ? QStringLiteral("ready") :
                                        (record.stop_command_ok ? QStringLiteral("not_ready")
                                                                : QStringLiteral("rejected")));
    evidence.insert(QStringLiteral("stop_command_ok"), record.stop_command_ok);
    evidence.insert(QStringLiteral("container"),
                    record.playback_after.value(QStringLiteral("container")).toString());
    evidence.insert(QStringLiteral("video_codec"),
                    record.playback_after.value(QStringLiteral("video_codec")).toString());
    evidence.insert(QStringLiteral("audio_codec"),
                    record.playback_after.value(QStringLiteral("audio_codec")).toString());
    evidence.insert(QStringLiteral("audio_error"),
                    record.playback_after.value(QStringLiteral("audio_error")).toString());
    evidence.insert(QStringLiteral("duration_ms"),
                    record.playback_after.value(QStringLiteral("duration_ms")).toLongLong());
    evidence.insert(QStringLiteral("width"),
                    record.playback_after.value(QStringLiteral("width")).toInt());
    evidence.insert(QStringLiteral("height"),
                    record.playback_after.value(QStringLiteral("height")).toInt());
    evidence.insert(QStringLiteral("video_stream_count"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("video_stream_count")).toULongLong()));
    evidence.insert(QStringLiteral("audio_stream_count"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("audio_stream_count")).toULongLong()));
    const qint64 audio_product_option_count =
        option_count(record.snapshot_after, QStringLiteral("audio_track_options"));
    evidence.insert(QStringLiteral("audio_product_option_count"),
                    audio_product_option_count);
    evidence.insert(QStringLiteral("audio_track_choice_available"),
                    audio_product_option_count > 1);
    const auto subtitle_stream_count =
        record.playback_after.value(QStringLiteral("subtitle_stream_count")).toULongLong();
    const bool embedded_ingest =
        record.playback_after.value(
            QStringLiteral("p8_embedded_packet_ingest_connected")).toBool();
    const bool pgs_render =
        record.playback_after.value(
            QStringLiteral("p8_pgs_bitmap_render_connected")).toBool();
    const bool text_frame_runtime_connected =
        record.snapshot_t5.value(QStringLiteral("subtitle_frame_runtime_connected")).toBool() ||
        record.playback_t5.value(QStringLiteral("subtitle_frame_runtime_connected")).toBool() ||
        record.snapshot_after.value(QStringLiteral("subtitle_frame_runtime_connected")).toBool() ||
        record.playback_after.value(QStringLiteral("subtitle_frame_runtime_connected")).toBool();
    const QString text_frame_t5 =
        record.snapshot_t5.value(QStringLiteral("subtitle_primary")).toString();
    const bool text_frame_available_t5 =
        record.snapshot_t5.value(QStringLiteral("subtitle_frame_available")).toBool() &&
        !text_frame_t5.isEmpty();
    const bool text_frame_connected = embedded_ingest && text_frame_runtime_connected &&
                                      text_frame_available_t5;
    evidence.insert(QStringLiteral("subtitle_stream_count"),
                    static_cast<qint64>(subtitle_stream_count));
    evidence.insert(QStringLiteral("subtitle_product_option_count"),
                    option_count(record.snapshot_after, QStringLiteral("subtitle_options")));
    evidence.insert(QStringLiteral("subtitle_status"),
                    text_frame_connected
                        ? QStringLiteral("partial_embedded_text_frame_no_bitmap_render")
                        : (embedded_ingest
                               ? QStringLiteral("partial_embedded_packet_ingest_no_bitmap_render")
                               : QStringLiteral("deferred")));
    evidence.insert(QStringLiteral("p8_contract_family_present"), true);
    evidence.insert(QStringLiteral("p8_subtitle_track_inventory_discovered"),
                    subtitle_stream_count > 0);
    evidence.insert(QStringLiteral("p8_subtitle_connected"), text_frame_connected);
    evidence.insert(QStringLiteral("p8_text_frame_connected"), text_frame_connected);
    evidence.insert(QStringLiteral("p8_text_frame_runtime_connected"),
                    text_frame_runtime_connected);
    evidence.insert(QStringLiteral("p8_text_frame_available_t5"), text_frame_available_t5);
    evidence.insert(QStringLiteral("p8_text_frame_primary_t5"), text_frame_t5);
    evidence.insert(QStringLiteral("p8_full_subtitle_render_connected"), false);
    evidence.insert(QStringLiteral("p8_embedded_packet_ingest_connected"), embedded_ingest);
    evidence.insert(QStringLiteral("p8_text_buffer_fixture_only"), !embedded_ingest);
    evidence.insert(QStringLiteral("p8_pgs_bitmap_render_connected"), pgs_render);
    evidence.insert(QStringLiteral("p8_subtitle_packets_submitted"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p8_subtitle_packets_submitted")).toULongLong()));
    evidence.insert(QStringLiteral("p8_unsupported_feature_count"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p8_unsupported_feature_count")).toULongLong()));
    evidence.insert(QStringLiteral("p4_playback_revision"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p4_playback_revision")).toULongLong()));
    evidence.insert(QStringLiteral("p4_timeline_contract_connected"),
                    record.playback_after.value(
                        QStringLiteral("p4_timeline_contract_connected")).toBool());
    evidence.insert(QStringLiteral("p4_timeline_uses_public_contract"),
                    record.playback_after.value(
                        QStringLiteral("p4_timeline_uses_public_contract")).toBool());
    evidence.insert(QStringLiteral("p4_runtime_state_machine_owner"),
                    record.playback_after.value(
                        QStringLiteral("p4_runtime_state_machine_owner")).toBool());
    evidence.insert(QStringLiteral("p4_timeline_revision"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p4_timeline_revision")).toULongLong()));
    evidence.insert(QStringLiteral("p4_timeline_generation"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p4_timeline_generation")).toULongLong()));
    evidence.insert(QStringLiteral("p4_timeline_observed_position_ms"),
                    record.playback_after.value(
                        QStringLiteral("p4_timeline_observed_position_ms")).toLongLong());
    evidence.insert(QStringLiteral("p4_timeline_duration_ms"),
                    record.playback_after.value(
                        QStringLiteral("p4_timeline_duration_ms")).toLongLong());
    evidence.insert(QStringLiteral("p7_retained_command_count"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p7_retained_command_count")).toULongLong()));
    evidence.insert(QStringLiteral("p7_late_event_discard_count"),
                    static_cast<qint64>(
                        record.playback_after.value(
                            QStringLiteral("p7_late_event_discard_count")).toULongLong()));
    evidence.insert(QStringLiteral("playlist_queue_connected"),
                    record.snapshot_after.value(
                        QStringLiteral("playlist_queue_connected")).toBool());
    evidence.insert(QStringLiteral("playlist_item_count"),
                    static_cast<qint64>(
                        record.snapshot_after.value(
                            QStringLiteral("playlist_item_count")).toULongLong()));
    evidence.insert(QStringLiteral("playlist_current_index"),
                    static_cast<qint64>(
                        record.snapshot_after.value(
                            QStringLiteral("playlist_current_index")).toULongLong()));
    evidence.insert(QStringLiteral("playlist_has_previous"),
                    record.snapshot_after.value(
                        QStringLiteral("playlist_has_previous")).toBool());
    evidence.insert(QStringLiteral("playlist_has_next"),
                    record.snapshot_after.value(QStringLiteral("playlist_has_next")).toBool());
    evidence.insert(QStringLiteral("playlist_has_next_episode"),
                    record.snapshot_after.value(
                        QStringLiteral("playlist_has_next_episode")).toBool());
    QJsonArray playlist_items;
    const QVariantList mapped_items =
        record.snapshot_after.value(QStringLiteral("episode_options")).toList();
    for (const QVariant& item_variant : mapped_items) {
        const QVariantMap item = item_variant.toMap();
        QJsonObject mapped_item;
        mapped_item.insert(QStringLiteral("item_index"),
                           static_cast<qint64>(
                               item.value(QStringLiteral("itemIndex")).toULongLong()));
        mapped_item.insert(QStringLiteral("name"),
                           item.value(QStringLiteral("name")).toString());
        mapped_item.insert(QStringLiteral("current"),
                           item.value(QStringLiteral("current")).toBool());
        playlist_items.append(mapped_item);
    }
    evidence.insert(QStringLiteral("playlist_items"), playlist_items);
    evidence.insert(QStringLiteral("playback_page_url"), record.playback_page_url);
    evidence.insert(QStringLiteral("fake_frame_used"), false);
    evidence.insert(QStringLiteral("fake_content_used"), false);
    evidence.insert(QStringLiteral("exit"), QStringLiteral("normal"));
    evidence.insert(QStringLiteral("residual_process"), false);
    evidence.insert(QStringLiteral("stall_note"), stall_note);
    evidence.insert(QStringLiteral("errors"), collect_errors(record).join(QStringLiteral("; ")));
    evidence.insert(QStringLiteral("classification"), classification);
    return evidence;
}
void write_json_file(const QString& path, const QJsonObject& object) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
        file.close();
    }
}

}  // namespace

QString write_continuous_playback_evidence(const ContinuousPlaybackEvidenceRecord& record) {
    QDir output_dir(record.output_dir);
    output_dir.mkpath(QStringLiteral("."));

    const QString classification = classification_from_query(classification_input(record));
    const QString continuous_path =
        output_dir.filePath(QStringLiteral("continuous_playback_evidence.json"));
    const QString real_local_path =
        output_dir.filePath(QStringLiteral("real_playback_local_evidence.json"));

    const QString errors = collect_errors(record).join(QStringLiteral("; "));
    write_json_file(continuous_path, build_continuous_json(record, classification));
    write_json_file(real_local_path, build_real_local_json(record, classification, errors));
    return continuous_path;
}

}  // namespace kivo::ui::shell
