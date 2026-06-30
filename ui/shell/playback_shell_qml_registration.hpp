#pragma once

#include <QString>

class QQmlApplicationEngine;

namespace kivo::ui::shell {

struct PlaybackShellQmlRegistrationResult {
    bool ok{false};
    QString error_message;
    QString playback_page_url;
};

[[nodiscard]] PlaybackShellQmlRegistrationResult configure_playback_shell_engine(
    QQmlApplicationEngine& engine,
    const QString& source_root);

}  // namespace kivo::ui::shell