#include "playback_shell_normal_media_launch.hpp"

#include <QQuickWindow>
#include <QTimer>

#include "playback_shell_manual_media_open_controller.hpp"
#include "playback_shell_runtime_adapter_bridge.hpp"
#include "playback_shell_video_launch_support.hpp"

namespace kivo::ui::shell {

void schedule_normal_media_launch(
    QQuickWindow* const window,
    PlaybackShellRuntimeAdapterBridge* const bridge,
    const NormalMediaLaunchOptions& options) {
    if (window == nullptr || bridge == nullptr || options.media_path.isEmpty()) {
        return;
    }

    QTimer::singleShot(500, window, [window, bridge, options]() {
        QObject* const page = wait_for_playback_page(window);
        if (!options.media_resolve_error.isEmpty()) {
            qWarning().noquote() << "NORMAL_MEDIA_RESOLVE_ERROR=" << options.media_resolve_error;
        }
        PlaybackShellManualMediaOpenController manual_controller(bridge, window);
        const bool open_ok = options.selected_media_exists &&
                             manual_controller.openLocalMediaPath(page, options.media_path);
        const bool play_ok = open_ok;
        const bool frame_ready = play_ok && wait_for_first_frame(bridge);
        qInfo().noquote() << "NORMAL_MEDIA_OPEN=" << (open_ok ? "accepted" : "rejected");
        qInfo().noquote() << "NORMAL_MEDIA_PLAY=" << (play_ok ? "accepted" : "rejected");
        qInfo().noquote() << "NORMAL_MEDIA_FIRST_FRAME=" << (frame_ready ? "yes" : "no");
    });
}

}  // namespace kivo::ui::shell
