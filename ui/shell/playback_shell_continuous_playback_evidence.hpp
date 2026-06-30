#pragma once

#include <QString>

class QQuickWindow;

namespace kivo::video::player_runtime_adapter {
class PlayerRuntimeAdapter;
}

namespace kivo::ui::shell {

class PlaybackShellRuntimeAdapterBridge;

struct ContinuousPlaybackEvidenceOptions {
    QString output_dir;
    QString media_path;
    QString selected_media_file;
    bool media_original_exists{false};
    bool media_is_directory{false};
    bool selected_media_exists{false};
    QString media_resolve_error;
    int duration_ms{5000};
    int close_delay_ms{500};
};

void schedule_continuous_playback_evidence(
    QQuickWindow* window,
    PlaybackShellRuntimeAdapterBridge* bridge,
    kivo::video::player_runtime_adapter::PlayerRuntimeAdapter* adapter,
    const ContinuousPlaybackEvidenceOptions& options,
    const QString& playback_page_url);

}  // namespace kivo::ui::shell