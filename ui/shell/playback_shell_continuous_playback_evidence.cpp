#include "playback_shell_continuous_playback_evidence.hpp"

#include <QDir>
#include <QElapsedTimer>
#include <QQuickWindow>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include <cstdlib>

#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"
#include "playback_shell_continuous_evidence_record.hpp"
#include "playback_shell_runtime_adapter_bridge.hpp"
#include "playback_shell_video_surface_item.hpp"
#include "playback_shell_video_launch_support.hpp"

namespace kivo::ui::shell {
namespace {

using kivo::video::player_runtime_adapter::PlayerRuntimeAdapter;

[[nodiscard]] qint64 safe_seek_target_ms(const qint64 duration_ms) noexcept {
    if (duration_ms > 70000) {
        return 60000;
    }
    if (duration_ms > 4000) {
        return duration_ms / 2;
    }
    return 1000;
}

[[nodiscard]] quint64 alternate_audio_track(const QVariantMap& snapshot) {
    const quint64 selected =
        snapshot.value(QStringLiteral("selected_audio_track")).toULongLong();
    const QVariantList options =
        snapshot.value(QStringLiteral("audio_track_options")).toList();
    for (const QVariant& option_variant : options) {
        const QVariantMap option = option_variant.toMap();
        const quint64 track_id = option.value(QStringLiteral("trackId")).toULongLong();
        if (track_id != 0 && track_id != selected) {
            return track_id;
        }
    }
    return 0;
}

[[nodiscard]] quint64 first_subtitle_track(const QVariantMap& snapshot) {
    const QVariantList options =
        snapshot.value(QStringLiteral("subtitle_options")).toList();
    for (const QVariant& option_variant : options) {
        const QVariantMap option = option_variant.toMap();
        const quint64 track_id = option.value(QStringLiteral("trackId")).toULongLong();
        if (track_id != 0) {
            return track_id;
        }
    }
    return 0;
}

}  // namespace

void schedule_continuous_playback_evidence(
    QQuickWindow* const window,
    PlaybackShellRuntimeAdapterBridge* const bridge,
    PlayerRuntimeAdapter* const adapter,
    const ContinuousPlaybackEvidenceOptions& options,
    const QString& playback_page_url) {
    if (window == nullptr || bridge == nullptr || adapter == nullptr ||
        options.output_dir.isEmpty() || options.media_path.isEmpty()) {
        return;
    }

    QTimer::singleShot(500, window, [window, bridge, adapter, options, playback_page_url]() {
        QDir output_dir(options.output_dir);
        output_dir.mkpath(QStringLiteral("."));

        QObject* const page = wait_for_playback_page(window);
        pump_events(200);

        const bool media_exists = options.media_original_exists;
        const QString playback_file =
            options.selected_media_exists ? options.selected_media_file : QString{};
        const bool open_ok =
            options.selected_media_exists && bridge->openLocalMediaFile(playback_file);
        pump_events(120);

        const QVariantMap snapshot_after_open = bridge->pullSnapshot();
        const quint64 subtitle_track_target =
            open_ok ? first_subtitle_track(snapshot_after_open) : 0;
        if (subtitle_track_target != 0) {
            (void)bridge->selectSubtitleTrack(subtitle_track_target);
            pump_events(120);
        }

        const bool play_ok = open_ok && bridge->play();
        pump_events(200);

        const bool frame_ready = open_ok && play_ok && wait_for_first_frame(bridge);

        PlaybackShellVideoSurfaceItem* surface = nullptr;
        if (page != nullptr && frame_ready) {
            surface = attach_video_surface(page, bridge, true);
            pump_events(300);
        }

        QVariantMap playback_before = bridge->queryLocalMediaPlayback();
        const QVariantMap snapshot_before = bridge->pullSnapshot();
        const qint64 timeline_before =
            snapshot_before.value(QStringLiteral("position_ms")).toLongLong();
        const quint64 signature_before =
            playback_before.value(QStringLiteral("last_frame_signature")).toULongLong();

        const QString screenshot_t0 = output_dir.filePath(QStringLiteral("playback_t0.png"));
        const bool screenshot_t0_saved = window->grabWindow().save(screenshot_t0);

        const int duration_ms = options.duration_ms > 0 ? options.duration_ms : 5000;
        QElapsedTimer observed_timer;
        observed_timer.start();
        pump_events(duration_ms >= 5000 ? 5000 : duration_ms);

        QVariantMap playback_t5 = bridge->queryLocalMediaPlayback();
        const QVariantMap snapshot_t5 = bridge->pullSnapshot();
        const qint64 timeline_t5 =
            snapshot_t5.value(QStringLiteral("position_ms")).toLongLong();
        const quint64 signature_t5 =
            playback_t5.value(QStringLiteral("last_frame_signature")).toULongLong();

        const QString screenshot_t5s = output_dir.filePath(QStringLiteral("playback_t5s.png"));
        const bool screenshot_t5s_saved = window->grabWindow().save(screenshot_t5s);

        if (surface != nullptr) {
            surface->stopContinuousRefresh();
            (void)surface->refreshFrame();
            pump_events(100);
        }

        const bool pause_command_ok = frame_ready && bridge->pause();
        const qint64 pause_position_before = snapshot_position_ms(bridge);
        pump_events(1000);
        const qint64 pause_position_after = snapshot_position_ms(bridge);
        const bool pause_holds_position =
            pause_command_ok && std::llabs(pause_position_after - pause_position_before) <= 120;

        const bool resume_command_ok = pause_command_ok && bridge->togglePlayPause();
        if (surface != nullptr && resume_command_ok) {
            surface->startContinuousRefresh(33);
        }
        pump_events(1200);
        const qint64 resume_position_after = snapshot_position_ms(bridge);
        const bool resume_advances_position =
            resume_command_ok && resume_position_after > pause_position_after;

        const QVariantMap playback_before_seek = bridge->queryLocalMediaPlayback();
        const quint64 signature_before_seek =
            playback_before_seek.value(QStringLiteral("last_frame_signature")).toULongLong();
        const auto decoded_before_seek =
            playback_before_seek.value(QStringLiteral("decoded_frame_count")).toULongLong();
        const qint64 seek_target_ms =
            safe_seek_target_ms(playback_before_seek.value(QStringLiteral("duration_ms")).toLongLong());
        const bool seek_command_ok = resume_command_ok && bridge->seek(seek_target_ms);
        QElapsedTimer post_seek_timer;
        post_seek_timer.start();
        pump_events(200);

        bool post_seek_frame_rendered = false;
        qint64 post_seek_frame_ready_ms = -1;
        qint64 post_seek_frame_time_ms = 0;
        QVariantMap playback_after_seek;
        for (int attempt = 0; attempt < 80; ++attempt) {
            pump_events(50);
            playback_after_seek = bridge->queryLocalMediaPlayback();
            const auto decoded_now =
                playback_after_seek.value(QStringLiteral("decoded_frame_count")).toULongLong();
            const auto signature_now =
                playback_after_seek.value(QStringLiteral("last_frame_signature")).toULongLong();
            const qint64 position_now = snapshot_position_ms(bridge);
            const qint64 frame_time_now_ms =
                playback_after_seek.value(QStringLiteral("last_frame_time_us")).toLongLong() / 1000;
            if (playback_after_seek.value(QStringLiteral("frame_pixels_valid")).toBool() &&
                frame_time_now_ms >= seek_target_ms &&
                (decoded_now > decoded_before_seek || signature_now != signature_before_seek ||
                 position_now >= seek_target_ms)) {
                post_seek_frame_rendered = true;
                post_seek_frame_ready_ms = post_seek_timer.elapsed();
                post_seek_frame_time_ms = frame_time_now_ms;
                break;
            }
        }
        if (surface != nullptr) {
            (void)surface->refreshFrame();
        }

        const int remaining_ms =
            duration_ms > observed_timer.elapsed()
                ? static_cast<int>(duration_ms - observed_timer.elapsed())
                : 0;
        if (remaining_ms > 0) {
            pump_events(remaining_ms);
        }
        const qint64 observed_ms = observed_timer.elapsed();

        const QVariantMap snapshot_before_track_switch = bridge->pullSnapshot();
        const QVariantMap playback_before_track_switch = bridge->queryLocalMediaPlayback();
        const quint64 audio_track_before_switch =
            snapshot_before_track_switch.value(QStringLiteral("selected_audio_track"))
                .toULongLong();
        const quint64 audio_track_target =
            alternate_audio_track(snapshot_before_track_switch);
        const bool audio_track_switch_available = audio_track_target != 0;
        const bool audio_track_switch_command_ok =
            audio_track_switch_available && bridge->selectAudioTrack(audio_track_target);
        if (surface != nullptr && audio_track_switch_command_ok) {
            surface->startContinuousRefresh(33);
        }
        if (audio_track_switch_command_ok) {
            pump_events(1200);
        }
        const QVariantMap playback_after_track_switch = bridge->queryLocalMediaPlayback();
        const QVariantMap snapshot_after_track_switch = bridge->pullSnapshot();
        const quint64 audio_track_after_switch =
            snapshot_after_track_switch.value(QStringLiteral("selected_audio_track"))
                .toULongLong();
        const bool audio_track_switch_selected =
            audio_track_switch_command_ok && audio_track_after_switch == audio_track_target;

        QVariantMap playback_after = playback_after_track_switch;
        QVariantMap snapshot_after = snapshot_after_track_switch;
        const bool had_audio_before_switch =
            playback_before_track_switch.value(QStringLiteral("audio_output_started")).toBool() ||
            playback_before_track_switch.value(QStringLiteral("audio_frames_written")).toULongLong() > 0;
        const QString audio_probe_error =
            playback_after_track_switch.value(QStringLiteral("audio_error")).toString();
        const bool switched_audio_failed =
            audio_track_switch_command_ok &&
            (!audio_probe_error.isEmpty() ||
             (had_audio_before_switch &&
              !playback_after_track_switch.value(QStringLiteral("audio_output_started")).toBool()));
        bool audio_track_restore_attempted = false;
        bool audio_track_restore_command_ok = false;
        if (switched_audio_failed && audio_track_before_switch != 0) {
            audio_track_restore_attempted = true;
            audio_track_restore_command_ok = bridge->selectAudioTrack(audio_track_before_switch);
            if (surface != nullptr) {
                surface->startContinuousRefresh(33);
            }
            if (audio_track_restore_command_ok) {
                pump_events(1200);
            }
            playback_after = bridge->queryLocalMediaPlayback();
            snapshot_after = bridge->pullSnapshot();
        }
        const quint64 audio_track_final =
            snapshot_after.value(QStringLiteral("selected_audio_track")).toULongLong();
        const bool audio_track_restored =
            audio_track_restore_attempted && audio_track_restore_command_ok &&
            audio_track_final == audio_track_before_switch;
        const qint64 timeline_after =
            snapshot_after.value(QStringLiteral("position_ms")).toLongLong();
        const quint64 signature_after =
            playback_after.value(QStringLiteral("last_frame_signature")).toULongLong();
        const auto rendered_after =
            playback_after.value(QStringLiteral("rendered_frame_count")).toULongLong();
        const qint64 last_frame_time_after_ms =
            playback_after.value(QStringLiteral("last_frame_time_us")).toLongLong() / 1000;

        const QString screenshot_t10s = output_dir.filePath(QStringLiteral("playback_t10s.png"));
        const bool screenshot_t10s_saved = window->grabWindow().save(screenshot_t10s);
        if (!post_seek_frame_rendered && seek_command_ok && screenshot_t10s_saved &&
            timeline_after >= seek_target_ms && last_frame_time_after_ms >= seek_target_ms &&
            rendered_after > 0) {
            post_seek_frame_rendered = true;
            post_seek_frame_ready_ms = post_seek_timer.elapsed();
            post_seek_frame_time_ms = last_frame_time_after_ms;
        }

        if (surface != nullptr) {
            surface->stopContinuousRefresh();
            (void)surface->refreshFrame();
            pump_events(100);
        }
        const bool stop_command_ok = bridge->stop();
        pump_events(100);
        const bool stop_ready = stop_command_ok && snapshot_ready_after_stop(bridge);

        const auto decoded_count =
            playback_after.value(QStringLiteral("decoded_frame_count")).toULongLong();
        const auto rendered_count =
            playback_after.value(QStringLiteral("rendered_frame_count")).toULongLong();
        ContinuousPlaybackEvidenceRecord record;
        record.output_dir = options.output_dir;
        record.media_path = options.media_path;
        record.selected_media_file = playback_file;
        record.media_resolve_error = options.media_resolve_error;
        record.playback_page_url = playback_page_url;
        record.screenshot_t0 = screenshot_t0;
        record.screenshot_t5s = screenshot_t5s;
        record.screenshot_t10s = screenshot_t10s;
        record.media_exists = media_exists;
        record.selected_media_exists = options.selected_media_exists;
        record.open_ok = open_ok;
        record.play_ok = play_ok;
        record.frame_ready = frame_ready;
        record.screenshot_t0_saved = screenshot_t0_saved;
        record.screenshot_t5s_saved = screenshot_t5s_saved;
        record.screenshot_t10s_saved = screenshot_t10s_saved;
        record.pause_command_ok = pause_command_ok;
        record.pause_holds_position = pause_holds_position;
        record.resume_command_ok = resume_command_ok;
        record.resume_advances_position = resume_advances_position;
        record.seek_command_ok = seek_command_ok;
        record.post_seek_frame_rendered = post_seek_frame_rendered;
        record.audio_track_switch_available = audio_track_switch_available;
        record.audio_track_switch_command_ok = audio_track_switch_command_ok;
        record.audio_track_switch_selected = audio_track_switch_selected;
        record.stop_command_ok = stop_command_ok;
        record.stop_ready = stop_ready;
        record.timeline_before = timeline_before;
        record.timeline_t5 = timeline_t5;
        record.timeline_after = timeline_after;
        record.observed_ms = observed_ms;
        record.pause_position_before = pause_position_before;
        record.pause_position_after = pause_position_after;
        record.resume_position_after = resume_position_after;
        record.seek_target_ms = seek_target_ms;
        record.post_seek_frame_ready_ms = post_seek_frame_ready_ms;
        record.post_seek_frame_time_ms = post_seek_frame_time_ms;
        record.signature_before = signature_before;
        record.signature_t5 = signature_t5;
        record.signature_after = signature_after;
        record.signature_before_seek = signature_before_seek;
        record.audio_track_before_switch = audio_track_before_switch;
        record.audio_track_target = audio_track_target;
        record.audio_track_after_switch = audio_track_final;
        record.audio_track_probe_selected = audio_track_after_switch;
        record.audio_track_final = audio_track_final;
        record.audio_frames_written_before_switch =
            playback_before_track_switch.value(QStringLiteral("audio_frames_written"))
                .toULongLong();
        record.audio_frames_written_after_switch =
            playback_after.value(QStringLiteral("audio_frames_written"))
                .toULongLong();
        record.audio_frames_written_probe =
            playback_after_track_switch.value(QStringLiteral("audio_frames_written"))
                .toULongLong();
        record.audio_frames_written_final =
            playback_after.value(QStringLiteral("audio_frames_written"))
                .toULongLong();
        record.audio_codec_before_switch =
            playback_before_track_switch.value(QStringLiteral("audio_codec")).toString();
        record.audio_codec_after_switch =
            playback_after.value(QStringLiteral("audio_codec")).toString();
        record.audio_codec_probe =
            playback_after_track_switch.value(QStringLiteral("audio_codec")).toString();
        record.audio_codec_final =
            playback_after.value(QStringLiteral("audio_codec")).toString();
        record.audio_probe_error = audio_probe_error;
        record.audio_probe_failed = switched_audio_failed;
        record.audio_track_restore_attempted = audio_track_restore_attempted;
        record.audio_track_restore_command_ok = audio_track_restore_command_ok;
        record.audio_track_restored = audio_track_restored;
        if (surface != nullptr) {
            const auto surface_stats = surface->stats();
            record.surface_refresh_attempt_count =
                surface_stats.refresh_attempt_count;
            record.surface_refresh_success_count =
                surface_stats.refresh_success_count;
            record.surface_refresh_miss_count = surface_stats.refresh_miss_count;
            record.surface_refresh_last_elapsed_ms =
                surface_stats.refresh_last_elapsed_ms;
            record.surface_refresh_max_elapsed_ms =
                surface_stats.refresh_max_elapsed_ms;
            record.surface_refresh_total_elapsed_ms =
                surface_stats.refresh_total_elapsed_ms;
            record.surface_paint_count = surface_stats.paint_count;
            record.surface_paint_last_elapsed_ms =
                surface_stats.paint_last_elapsed_ms;
            record.surface_paint_max_elapsed_ms =
                surface_stats.paint_max_elapsed_ms;
            record.surface_paint_total_elapsed_ms =
                surface_stats.paint_total_elapsed_ms;
        }
        record.playback_t5 = playback_t5;
        record.snapshot_t5 = snapshot_t5;
        record.playback_after = playback_after;
        record.snapshot_after = snapshot_after;
        const QString evidence_path = write_continuous_playback_evidence(record);

        qInfo().noquote() << "CONTINUOUS_PLAYBACK_EVIDENCE_JSON=" << evidence_path;
        qInfo().noquote() << "CONTINUOUS_PLAYBACK_SCREENSHOT_T0=" << screenshot_t0;
        qInfo().noquote() << "CONTINUOUS_PLAYBACK_SCREENSHOT_T5S=" << screenshot_t5s;
        qInfo().noquote() << "CONTINUOUS_PLAYBACK_SCREENSHOT_T10S=" << screenshot_t10s;
        qInfo().noquote() << "CONTINUOUS_PLAYBACK_DECODED_FRAMES=" << decoded_count;
        qInfo().noquote() << "CONTINUOUS_PLAYBACK_RENDERED_FRAMES=" << rendered_count;

        QTimer::singleShot(options.close_delay_ms, qApp, &QCoreApplication::quit);
    });
}

}  // namespace kivo::ui::shell
