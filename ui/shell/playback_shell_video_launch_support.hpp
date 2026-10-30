#pragma once

#include <QtGlobal>

class QObject;
class QQuickWindow;

namespace kivo::ui::shell {

class PlaybackShellRuntimeAdapterBridge;
class PlaybackShellVideoSurfaceItem;

void pump_events(int ms);

[[nodiscard]] QObject* wait_for_playback_page(QQuickWindow* window);

[[nodiscard]] PlaybackShellVideoSurfaceItem* attach_video_surface(
    QObject* page,
    PlaybackShellRuntimeAdapterBridge* bridge,
    bool start_refresh);

[[nodiscard]] bool wait_for_first_frame(PlaybackShellRuntimeAdapterBridge* bridge);

[[nodiscard]] qint64 snapshot_position_ms(PlaybackShellRuntimeAdapterBridge* bridge);

[[nodiscard]] bool snapshot_ready_after_stop(PlaybackShellRuntimeAdapterBridge* bridge);

}  // namespace kivo::ui::shell
