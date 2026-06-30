#pragma once

#include <QString>

class QQuickWindow;

namespace kivo::video::player_runtime_adapter {
class PlayerRuntimeAdapter;
}

namespace kivo::ui::shell {

class PlaybackShellRuntimeAdapterBridge;

struct InteractionEvidenceOptions {
    QString output_dir;
    int close_delay_ms{1200};
};

void schedule_interaction_evidence(
    QQuickWindow* window,
    PlaybackShellRuntimeAdapterBridge* bridge,
    kivo::video::player_runtime_adapter::PlayerRuntimeAdapter* adapter,
    const InteractionEvidenceOptions& options,
    const QString& playback_page_url);

}  // namespace kivo::ui::shell