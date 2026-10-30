#pragma once

#include <QString>
#include <QVariantMap>
#include <QtGlobal>

namespace kivo::ui::shell {

struct ContinuousPlaybackEvidenceRecord {
    QString output_dir;
    QString media_path;
    QString selected_media_file;
    QString media_resolve_error;
    QString playback_page_url;
    QString screenshot_t0;
    QString screenshot_t5s;
    QString screenshot_t10s;
    bool media_exists{false};
    bool selected_media_exists{false};
    bool open_ok{false};
    bool play_ok{false};
    bool frame_ready{false};
    bool screenshot_t0_saved{false};
    bool screenshot_t5s_saved{false};
    bool screenshot_t10s_saved{false};
    bool pause_command_ok{false};
    bool pause_holds_position{false};
    bool resume_command_ok{false};
    bool resume_advances_position{false};
    bool seek_command_ok{false};
    bool post_seek_frame_rendered{false};
    bool audio_track_switch_available{false};
    bool audio_track_switch_command_ok{false};
    bool audio_track_switch_selected{false};
    bool stop_command_ok{false};
    bool stop_ready{false};
    qint64 timeline_before{0};
    qint64 timeline_t5{0};
    qint64 timeline_after{0};
    qint64 observed_ms{0};
    qint64 pause_position_before{0};
    qint64 pause_position_after{0};
    qint64 resume_position_after{0};
    qint64 seek_target_ms{0};
    qint64 post_seek_frame_ready_ms{-1};
    qint64 post_seek_frame_time_ms{0};
    quint64 signature_before{0};
    quint64 signature_t5{0};
    quint64 signature_after{0};
    quint64 signature_before_seek{0};
    quint64 audio_track_before_switch{0};
    quint64 audio_track_target{0};
    quint64 audio_track_after_switch{0};
    quint64 audio_track_probe_selected{0};
    quint64 audio_track_final{0};
    quint64 audio_frames_written_before_switch{0};
    quint64 audio_frames_written_after_switch{0};
    quint64 audio_frames_written_probe{0};
    quint64 audio_frames_written_final{0};
    QString audio_codec_before_switch;
    QString audio_codec_after_switch;
    QString audio_codec_probe;
    QString audio_codec_final;
    QString audio_probe_error;
    bool audio_probe_failed{false};
    bool audio_track_restore_attempted{false};
    bool audio_track_restore_command_ok{false};
    bool audio_track_restored{false};
    quint64 surface_refresh_attempt_count{0};
    quint64 surface_refresh_success_count{0};
    quint64 surface_refresh_miss_count{0};
    qint64 surface_refresh_last_elapsed_ms{0};
    qint64 surface_refresh_max_elapsed_ms{0};
    qint64 surface_refresh_total_elapsed_ms{0};
    quint64 surface_paint_count{0};
    qint64 surface_paint_last_elapsed_ms{0};
    qint64 surface_paint_max_elapsed_ms{0};
    qint64 surface_paint_total_elapsed_ms{0};
    QVariantMap playback_t5;
    QVariantMap snapshot_t5;
    QVariantMap playback_after;
    QVariantMap snapshot_after;
};

[[nodiscard]] QString write_continuous_playback_evidence(
    const ContinuousPlaybackEvidenceRecord& record);

}  // namespace kivo::ui::shell
