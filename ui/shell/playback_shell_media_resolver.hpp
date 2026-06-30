#pragma once

#include <QString>

namespace kivo::ui::shell {

struct ResolvedMediaPath {
    QString original_path;
    QString selected_file;
    bool original_exists{false};
    bool is_directory{false};
    bool selected_exists{false};
    QString error;
};

[[nodiscard]] ResolvedMediaPath resolve_playback_media_path(const QString& media_path);

}  // namespace kivo::ui::shell