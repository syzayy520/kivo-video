#include "playback_shell_evidence_options.hpp"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QStringList>

namespace kivo::ui::shell {
namespace {

[[nodiscard]] int non_negative_or_default(const QString& value, const int fallback) {
    const int parsed = value.toInt();
    return parsed < 0 ? fallback : parsed;
}

[[nodiscard]] QString media_path_from_parser(QCommandLineParser& parser) {
    if (parser.isSet(QStringLiteral("media"))) {
        return parser.value(QStringLiteral("media"));
    }
    if (qEnvironmentVariableIsSet("KIVO_P10_MEDIA")) {
        return qEnvironmentVariable("KIVO_P10_MEDIA");
    }
    return {};
}

[[nodiscard]] QString resolve_source_root_for_evidence() {
    QDir dir(QCoreApplication::applicationDirPath());
    for (int depth = 0; depth < 8; ++depth) {
        if (dir.exists(QStringLiteral("ui/playback/page/PlaybackPage.qml"))) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return QDir::currentPath();
}

[[nodiscard]] QString default_ui_open_dir() {
    return QDir(resolve_source_root_for_evidence())
        .filePath(QStringLiteral("artifacts/p10/ui_open_manual"));
}

[[nodiscard]] bool is_ui_open_dir_option(const QString& argument) {
    return argument == QStringLiteral("-ui-open") ||
           argument == QStringLiteral("--ui-open") ||
           argument == QStringLiteral("-ui-open-evidence") ||
           argument == QStringLiteral("--ui-open-evidence");
}

[[nodiscard]] bool argument_needs_default_dir(const QStringList& arguments, const int index) {
    return index + 1 >= arguments.size() || arguments.at(index + 1).startsWith(QLatin1Char('-'));
}

[[nodiscard]] QStringList normalize_optional_ui_open_dir(QStringList arguments) {
    const QString fallback_dir = default_ui_open_dir();
    for (int index = 1; index < arguments.size(); ++index) {
        if (is_ui_open_dir_option(arguments.at(index)) &&
            argument_needs_default_dir(arguments, index)) {
            arguments.insert(index + 1, fallback_dir);
            ++index;
        }
    }
    return arguments;
}

}  // namespace

ShellEvidenceOptions parse_shell_evidence_options(QCommandLineParser& parser) {
    ShellEvidenceOptions options{};
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("ui-open-evidence"), QStringLiteral("ui-open")},
        QStringLiteral("Capture UI_OPEN screenshot evidence to directory"),
        QStringLiteral("dir")));
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("ui-open-close-ms")},
        QStringLiteral("Auto-close delay after screenshot"),
        QStringLiteral("ms"),
        QStringLiteral("1500")));
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("interaction-evidence")},
        QStringLiteral("Capture shell runtime interaction evidence to directory"),
        QStringLiteral("dir")));
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("interaction-close-ms")},
        QStringLiteral("Auto-close delay after interaction evidence"),
        QStringLiteral("ms"),
        QStringLiteral("1200")));
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("playback-evidence")},
        QStringLiteral("Capture real local media playback evidence to directory"),
        QStringLiteral("dir")));
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("media")},
        QStringLiteral("Local media file path for playback"),
        QStringLiteral("path")));
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("playback-close-ms")},
        QStringLiteral("Auto-close delay after playback evidence"),
        QStringLiteral("ms"),
        QStringLiteral("1500")));
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("continuous-playback-evidence")},
        QStringLiteral("Capture continuous local media playback evidence to directory"),
        QStringLiteral("dir")));
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("duration-ms")},
        QStringLiteral("Continuous playback observation duration"),
        QStringLiteral("ms"),
        QStringLiteral("5000")));
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("continuous-close-ms")},
        QStringLiteral("Auto-close delay after continuous playback evidence"),
        QStringLiteral("ms"),
        QStringLiteral("500")));
    parser.process(normalize_optional_ui_open_dir(QCoreApplication::arguments()));

    options.media_path = media_path_from_parser(parser);

    if (parser.isSet(QStringLiteral("ui-open-evidence")) ||
        parser.isSet(QStringLiteral("ui-open"))) {
        options.ui_open_enabled = true;
        options.ui_open_dir = parser.isSet(QStringLiteral("ui-open-evidence"))
                                  ? parser.value(QStringLiteral("ui-open-evidence"))
                                  : parser.value(QStringLiteral("ui-open"));
        if (options.ui_open_dir.isEmpty()) {
            options.ui_open_dir = default_ui_open_dir();
        }
        options.ui_open_close_ms =
            non_negative_or_default(parser.value(QStringLiteral("ui-open-close-ms")), 1500);
    }
    if (parser.isSet(QStringLiteral("interaction-evidence"))) {
        options.interaction_enabled = true;
        options.interaction_dir = parser.value(QStringLiteral("interaction-evidence"));
        options.interaction_close_ms =
            non_negative_or_default(parser.value(QStringLiteral("interaction-close-ms")), 1200);
    }
    if (parser.isSet(QStringLiteral("playback-evidence"))) {
        options.playback_enabled = true;
        options.playback_dir = parser.value(QStringLiteral("playback-evidence"));
        options.playback_close_ms =
            non_negative_or_default(parser.value(QStringLiteral("playback-close-ms")), 1500);
    }
    if (parser.isSet(QStringLiteral("continuous-playback-evidence"))) {
        options.continuous_playback_enabled = true;
        options.continuous_playback_dir =
            parser.value(QStringLiteral("continuous-playback-evidence"));
        options.continuous_duration_ms =
            non_negative_or_default(parser.value(QStringLiteral("duration-ms")), 5000);
        options.continuous_close_ms =
            non_negative_or_default(parser.value(QStringLiteral("continuous-close-ms")), 500);
    }

    options.normal_media_enabled =
        parser.isSet(QStringLiteral("media")) && !options.ui_open_enabled &&
        !options.interaction_enabled && !options.playback_enabled &&
        !options.continuous_playback_enabled;
    return options;
}

}  // namespace kivo::ui::shell
