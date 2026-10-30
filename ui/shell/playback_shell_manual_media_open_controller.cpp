#include "playback_shell_manual_media_open_controller.hpp"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

#include "playback_shell_media_resolver.hpp"
#include "playback_shell_runtime_adapter_bridge.hpp"
#include "playback_shell_video_launch_support.hpp"
#include "playback_shell_video_surface_item.hpp"

namespace kivo::ui::shell {
namespace {

[[nodiscard]] QString local_path_from_url(const QUrl& url) {
    if (url.isEmpty()) {
        return {};
    }
    if (url.isLocalFile()) {
        return QDir::toNativeSeparators(url.toLocalFile());
    }
    return QDir::toNativeSeparators(url.toString(QUrl::PreferLocalFile));
}

void apply_page_media_label(QObject* const playback_page, const QString& media_path) {
    if (playback_page == nullptr) {
        return;
    }
    const QFileInfo info(media_path);
    const QString label = info.completeBaseName().isEmpty()
                              ? info.fileName()
                              : info.completeBaseName();
    if (!label.isEmpty()) {
        (void)playback_page->setProperty("title", label);
    }
    (void)playback_page->setProperty("seasonEpisode", QString{});
}

}  // namespace

PlaybackShellManualMediaOpenController::PlaybackShellManualMediaOpenController(
    PlaybackShellRuntimeAdapterBridge* const bridge,
    QObject* const parent)
    : QObject(parent), bridge_(bridge) {}

bool PlaybackShellManualMediaOpenController::openLocalMediaUrl(
    QObject* const playback_page,
    const QUrl& media_url) {
    return openLocalMediaPath(playback_page, local_path_from_url(media_url));
}

bool PlaybackShellManualMediaOpenController::openLocalMediaPath(
    QObject* const playback_page,
    const QString& media_path) {
    if (bridge_ == nullptr || playback_page == nullptr || media_path.isEmpty()) {
        return false;
    }

    const QString native_path = QDir::toNativeSeparators(media_path);
    const ResolvedMediaPath resolved = resolve_playback_media_path(native_path);
    if (!resolved.error.isEmpty() || !resolved.selected_exists) {
        return false;
    }

    const bool open_ok = bridge_->openLocalMediaFile(resolved.selected_file);
    const bool play_ok = open_ok && bridge_->play();
    if (!play_ok) {
        return false;
    }

    if (!surface_.isNull()) {
        surface_->stopContinuousRefresh();
        surface_->deleteLater();
        surface_.clear();
    }

    surface_ = attach_video_surface(playback_page, bridge_, true);
    apply_page_media_label(playback_page, resolved.selected_file);
    return !surface_.isNull();
}

}  // namespace kivo::ui::shell
