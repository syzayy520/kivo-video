#pragma once

#include <QString>

class QQuickWindow;

namespace kivo::video::player_runtime_adapter {
class PlayerRuntimeAdapter;
}

namespace kivo::ui::shell {

class PlaybackShellRuntimeAdapterBridge;

struct PlaybackEvidenceOptions {
    QString output_dir;
    QString media_path;
    QString selected_media_file;
    bool media_original_exists{false};
    bool media_is_directory{false};
    bool selected_media_exists{false};
    QString media_resolve_error;
    int close_delay_ms{1500};
};

void schedule_playback_evidence(
    QQuickWindow* window,
    PlaybackShellRuntimeAdapterBridge* bridge,
    kivo::video::player_runtime_adapter::PlayerRuntimeAdapter* adapter,
    const PlaybackEvidenceOptions& options,
    const QString& playback_page_url);

}  // namespace kivo::ui::shell