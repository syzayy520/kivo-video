#pragma once

#include <QString>

class QCommandLineParser;

namespace kivo::ui::shell {

struct ShellEvidenceOptions {
    bool ui_open_enabled{false};
    QString ui_open_dir;
    int ui_open_close_ms{1500};
    bool interaction_enabled{false};
    QString interaction_dir;
    int interaction_close_ms{1200};
    bool playback_enabled{false};
    QString playback_dir;
    QString media_path;
    bool normal_media_enabled{false};
    int playback_close_ms{1500};
    bool continuous_playback_enabled{false};
    QString continuous_playback_dir;
    int continuous_duration_ms{5000};
    int continuous_close_ms{500};
};

[[nodiscard]] ShellEvidenceOptions parse_shell_evidence_options(QCommandLineParser& parser);

}  // namespace kivo::ui::shell
