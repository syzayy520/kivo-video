#pragma once

#include <QString>

class QQuickWindow;

namespace kivo::ui::shell {

class PlaybackShellRuntimeAdapterBridge;

struct NormalMediaLaunchOptions {
    QString media_path;
    QString selected_media_file;
    bool media_original_exists{false};
    bool selected_media_exists{false};
    QString media_resolve_error;
};

void schedule_normal_media_launch(
    QQuickWindow* window,
    PlaybackShellRuntimeAdapterBridge* bridge,
    const NormalMediaLaunchOptions& options);

}  // namespace kivo::ui::shell
