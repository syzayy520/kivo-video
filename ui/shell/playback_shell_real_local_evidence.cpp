#include "playback_shell_real_local_evidence.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QStringLiteral>

#include "playback_shell_backend_layer_status.hpp"

namespace kivo::ui::shell {
namespace {

[[nodiscard]] qint64 option_count(const QVariantMap& snapshot, const QString& key) {
    return static_cast<qint64>(snapshot.value(key).toList().size());
}

}  // namespace

QJsonObject build_real_local_json(
    const ContinuousPlaybackEvidenceRecord& record,
    const QString& continuous_classification,
    const QString& errors) {
    const bool pass_video = continuous_classification == QStringLiteral("PASS_CONTINUOUS_PLAYBACK");
    const bool p2_connected =
        record.playback_after.value(QStringLiteral("source_session_connected")).toBool();
    const bool pass_audio =
        record.playback_after.value(QStringLiteral("audio_frames_written")).toULongLong() > 0;
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

    QJsonObject routing;
    routing.insert(QStringLiteral("qml_route"),
                   QStringLiteral("QML PlayerPage -> PlayerRuntimeAdapter"));
    routing.insert(QStringLiteral("adapter_route"),
                   QStringLiteral("PlayerRuntimeAdapter -> P7 PlaybackSession"));
    routing.insert(QStringLiteral("p7_runtime_route"),
                   QStringLiteral("P7 local_media continuous playback"));
    routing.insert(QStringLiteral("qml_direct_lower_phase"), false);
    routing.insert(QStringLiteral("adapter_uses_p7_public_api"), true);

    QJsonObject real;
    real.insert(QStringLiteral("classification"),
                pass_video && p2_connected && pass_audio
                    ? QStringLiteral("PASS_LOCAL_REAL_PLAYBACK_VIDEO_AUDIO")
                    : (pass_video && p2_connected
                           ? QStringLiteral("PASS_LOCAL_REAL_PLAYBACK_VIDEO_ONLY_AUDIO_DEFERRED")
                           : continuous_classification));
    real.insert(QStringLiteral("continuous_playback_classification"), continuous_classification);
    real.insert(QStringLiteral("routing"), routing);
    real.insert(QStringLiteral("layer_status"), build_backend_layer_status(record.playback_after));
    real.insert(QStringLiteral("video_real_frames"), pass_video);
    real.insert(QStringLiteral("audio_status"),
                pass_audio ? QStringLiteral("connected_pcm_wasapi")
                           : QStringLiteral("deferred_or_failed"));
    real.insert(QStringLiteral("audio_codec"),
                record.playback_after.value(QStringLiteral("audio_codec")).toString());
    real.insert(QStringLiteral("audio_track_final"),
                static_cast<qint64>(record.audio_track_final));
    real.insert(QStringLiteral("audio_codec_final"), record.audio_codec_final);
    real.insert(QStringLiteral("audio_track_probe_target"),
                static_cast<qint64>(record.audio_track_target));
    real.insert(QStringLiteral("audio_track_probe_selected"),
                static_cast<qint64>(record.audio_track_probe_selected));
    real.insert(QStringLiteral("audio_codec_probe"), record.audio_codec_probe);
    real.insert(QStringLiteral("audio_probe_failed"), record.audio_probe_failed);
    real.insert(QStringLiteral("audio_track_restored"), record.audio_track_restored);
    real.insert(QStringLiteral("audio_flush_count"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("audio_flush_count")).toULongLong()));
    real.insert(QStringLiteral("audio_writer_reset_count"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("audio_writer_reset_count")).toULongLong()));
    real.insert(QStringLiteral("audio_stale_packets_discarded"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("audio_stale_packets_discarded")).toULongLong()));
    real.insert(QStringLiteral("surface_refresh_attempt_count"),
                static_cast<qint64>(record.surface_refresh_attempt_count));
    real.insert(QStringLiteral("surface_refresh_success_count"),
                static_cast<qint64>(record.surface_refresh_success_count));
    real.insert(QStringLiteral("surface_refresh_miss_count"),
                static_cast<qint64>(record.surface_refresh_miss_count));
    real.insert(QStringLiteral("surface_refresh_last_elapsed_ms"),
                record.surface_refresh_last_elapsed_ms);
    real.insert(QStringLiteral("surface_refresh_max_elapsed_ms"),
                record.surface_refresh_max_elapsed_ms);
    real.insert(QStringLiteral("surface_refresh_total_elapsed_ms"),
                record.surface_refresh_total_elapsed_ms);
    real.insert(QStringLiteral("surface_paint_count"),
                static_cast<qint64>(record.surface_paint_count));
    real.insert(QStringLiteral("surface_paint_last_elapsed_ms"),
                record.surface_paint_last_elapsed_ms);
    real.insert(QStringLiteral("surface_paint_max_elapsed_ms"),
                record.surface_paint_max_elapsed_ms);
    real.insert(QStringLiteral("surface_paint_total_elapsed_ms"),
                record.surface_paint_total_elapsed_ms);
    real.insert(QStringLiteral("d3d11_uploaded"),
                record.playback_after.value(QStringLiteral("d3d11_uploaded")).toBool());
    real.insert(QStringLiteral("d3d11_upload_available"),
                record.playback_after.value(
                    QStringLiteral("d3d11_upload_available")).toBool());
    real.insert(QStringLiteral("d3d11_upload_initialized"),
                record.playback_after.value(
                    QStringLiteral("d3d11_upload_initialized")).toBool());
    real.insert(QStringLiteral("d3d11_frames_uploaded"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("d3d11_frames_uploaded")).toULongLong()));
    real.insert(QStringLiteral("d3d11_upload_failures"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("d3d11_upload_failures")).toULongLong()));
    real.insert(QStringLiteral("d3d11_upload_last_elapsed_ms"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("d3d11_upload_last_elapsed_ms")).toULongLong()));
    real.insert(QStringLiteral("d3d11_upload_max_elapsed_ms"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("d3d11_upload_max_elapsed_ms")).toULongLong()));
    real.insert(QStringLiteral("d3d11_device_type"),
                record.playback_after.value(QStringLiteral("d3d11_device_type")).toString());
    real.insert(QStringLiteral("d3d11_upload_error"),
                record.playback_after.value(QStringLiteral("d3d11_upload_error")).toString());
    real.insert(QStringLiteral("p5_continuous_d3d11_upload_connected"),
                record.playback_after.value(QStringLiteral("d3d11_uploaded")).toBool() &&
                    record.playback_after.value(
                        QStringLiteral("d3d11_frames_uploaded")).toULongLong() > 0);
    real.insert(QStringLiteral("p5_presentation_path"),
                QStringLiteral("qt_scenegraph_texture"));
    real.insert(QStringLiteral("p5_hardware_presenter_connected"), false);
    real.insert(QStringLiteral("p7_background_pump_count"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("p7_background_pump_count")).toULongLong()));
    real.insert(QStringLiteral("p7_background_pump_last_elapsed_ms"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("p7_background_pump_last_elapsed_ms")).toULongLong()));
    real.insert(QStringLiteral("p7_background_pump_max_elapsed_ms"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("p7_background_pump_max_elapsed_ms")).toULongLong()));
    real.insert(QStringLiteral("p7_background_pump_total_elapsed_ms"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("p7_background_pump_total_elapsed_ms")).toULongLong()));
    real.insert(QStringLiteral("p7_background_pump_last_new_frames"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("p7_background_pump_last_new_frames")).toULongLong()));
    real.insert(QStringLiteral("p7_background_pump_packet_budget"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("p7_background_pump_packet_budget")).toULongLong()));
    const qint64 audio_product_option_count =
        option_count(record.snapshot_after, QStringLiteral("audio_track_options"));
    real.insert(QStringLiteral("audio_product_option_count"),
                audio_product_option_count);
    real.insert(QStringLiteral("audio_track_choice_available"),
                audio_product_option_count > 1);
    real.insert(QStringLiteral("audio_error"),
                record.playback_after.value(QStringLiteral("audio_error")).toString());
    real.insert(QStringLiteral("p4_timeline_contract_connected"),
                record.playback_after.value(
                    QStringLiteral("p4_timeline_contract_connected")).toBool());
    real.insert(QStringLiteral("p4_timeline_uses_public_contract"),
                record.playback_after.value(
                    QStringLiteral("p4_timeline_uses_public_contract")).toBool());
    real.insert(QStringLiteral("p4_runtime_state_machine_owner"),
                record.playback_after.value(
                    QStringLiteral("p4_runtime_state_machine_owner")).toBool());
    real.insert(QStringLiteral("subtitle_status"),
                text_frame_connected
                    ? QStringLiteral("partial_embedded_text_frame_no_bitmap_render")
                    : (embedded_ingest
                           ? QStringLiteral("partial_embedded_packet_ingest_no_bitmap_render")
                           : QStringLiteral("deferred_no_real_embedded_packet_or_pgs_bitmap_path")));
    real.insert(QStringLiteral("subtitle_stream_count"),
                static_cast<qint64>(subtitle_stream_count));
    real.insert(QStringLiteral("subtitle_product_option_count"),
                option_count(record.snapshot_after, QStringLiteral("subtitle_options")));
    real.insert(QStringLiteral("p8_contract_family_present"), true);
    real.insert(QStringLiteral("p8_subtitle_track_inventory_discovered"),
                subtitle_stream_count > 0);
    real.insert(QStringLiteral("p8_subtitle_connected"), text_frame_connected);
    real.insert(QStringLiteral("p8_text_frame_connected"), text_frame_connected);
    real.insert(QStringLiteral("p8_text_frame_runtime_connected"),
                text_frame_runtime_connected);
    real.insert(QStringLiteral("p8_text_frame_available_t5"), text_frame_available_t5);
    real.insert(QStringLiteral("p8_text_frame_primary_t5"), text_frame_t5);
    real.insert(QStringLiteral("p8_full_subtitle_render_connected"), false);
    real.insert(QStringLiteral("p8_embedded_packet_ingest_connected"), embedded_ingest);
    real.insert(QStringLiteral("p8_text_buffer_fixture_only"), !embedded_ingest);
    real.insert(QStringLiteral("p8_pgs_bitmap_render_connected"), pgs_render);
    real.insert(QStringLiteral("p8_subtitle_packets_submitted"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("p8_subtitle_packets_submitted")).toULongLong()));
    real.insert(QStringLiteral("p8_unsupported_feature_count"),
                static_cast<qint64>(
                    record.playback_after.value(
                        QStringLiteral("p8_unsupported_feature_count")).toULongLong()));
    real.insert(QStringLiteral("playlist_queue_connected"),
                record.snapshot_after.value(
                    QStringLiteral("playlist_queue_connected")).toBool());
    real.insert(QStringLiteral("playlist_item_count"),
                static_cast<qint64>(
                    record.snapshot_after.value(
                        QStringLiteral("playlist_item_count")).toULongLong()));
    real.insert(QStringLiteral("playlist_current_index"),
                static_cast<qint64>(
                    record.snapshot_after.value(
                        QStringLiteral("playlist_current_index")).toULongLong()));
    real.insert(QStringLiteral("playlist_has_previous"),
                record.snapshot_after.value(QStringLiteral("playlist_has_previous")).toBool());
    real.insert(QStringLiteral("playlist_has_next"),
                record.snapshot_after.value(QStringLiteral("playlist_has_next")).toBool());
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
    real.insert(QStringLiteral("playlist_items"), playlist_items);
    real.insert(QStringLiteral("media_path"), record.media_path);
    real.insert(QStringLiteral("selected_media_file"), record.selected_media_file);
    real.insert(QStringLiteral("timeline_position_after_ms"), record.timeline_after);
    real.insert(QStringLiteral("fake_frame_used"), false);
    real.insert(QStringLiteral("fake_content_used"), false);
    real.insert(QStringLiteral("errors"), errors);
    return real;
}

}  // namespace kivo::ui::shell
