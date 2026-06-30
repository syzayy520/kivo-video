#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QQmlEngine>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>

#include "playback_shell_interaction_evidence.hpp"
#include "playback_shell_qml_registration.hpp"
#include "playback_shell_qt_platform_bootstrap.hpp"
#include "playback_shell_runtime_context.hpp"

namespace {

struct ShellEvidenceOptions {
    bool ui_open_enabled{false};
    QString ui_open_dir;
    int ui_open_close_ms{1500};
    bool interaction_enabled{false};
    QString interaction_dir;
    int interaction_close_ms{1200};
};

[[nodiscard]] ShellEvidenceOptions parse_shell_evidence_options(QCommandLineParser& parser) {
    ShellEvidenceOptions options{};
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("ui-open-evidence")},
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
    parser.process(*QCoreApplication::instance());

    if (parser.isSet(QStringLiteral("ui-open-evidence"))) {
        options.ui_open_enabled = true;
        options.ui_open_dir = parser.value(QStringLiteral("ui-open-evidence"));
        options.ui_open_close_ms = parser.value(QStringLiteral("ui-open-close-ms")).toInt();
        if (options.ui_open_close_ms < 0) {
            options.ui_open_close_ms = 1500;
        }
    }
    if (parser.isSet(QStringLiteral("interaction-evidence"))) {
        options.interaction_enabled = true;
        options.interaction_dir = parser.value(QStringLiteral("interaction-evidence"));
        options.interaction_close_ms = parser.value(QStringLiteral("interaction-close-ms")).toInt();
        if (options.interaction_close_ms < 0) {
            options.interaction_close_ms = 1200;
        }
    }
    return options;
}

[[nodiscard]] QString resolve_source_root() {
    const QString app_dir = QCoreApplication::applicationDirPath();
    QDir dir(app_dir);
    for (int depth = 0; depth < 8; ++depth) {
        if (dir.exists("ui/playback/page/PlaybackPage.qml")) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return QStringLiteral("C:/kivo video");
}

void schedule_ui_open_evidence(QQuickWindow* const window, const ShellEvidenceOptions& options) {
    if (!options.ui_open_enabled) {
        return;
    }
    if (window == nullptr) {
        return;
    }

    QTimer::singleShot(800, window, [window, options]() {
        const QImage frame = window->grabWindow();
        QDir output_dir(options.ui_open_dir);
        output_dir.mkpath(".");
        const QString screenshot_path =
            output_dir.filePath(QStringLiteral("ui_open_screenshot.png"));
        const bool saved = frame.save(screenshot_path);
        qInfo().noquote() << "UI_OPEN_SCREENSHOT_PATH=" << screenshot_path;
        qInfo().noquote() << "UI_OPEN_SCREENSHOT_EXISTS=" << (saved ? "yes" : "no");
        qInfo().noquote() << "UI_OPEN_WINDOW_TITLE=" << window->title();
        qInfo().noquote() << "UI_OPEN_WINDOW_SIZE="
                          << window->width() << "x" << window->height();

        QTimer::singleShot(options.ui_open_close_ms, qApp, &QCoreApplication::quit);
    });
}

}  // namespace

int main(int argc, char* argv[]) {
    kivo::ui::shell::bootstrap_qt_platform_paths(argv[0]);
    QGuiApplication app(argc, argv);

    if (qEnvironmentVariableIsSet("QT_DEBUG_PLUGINS") ||
        qEnvironmentVariableIsSet("KIVO_SHELL_QT_DIAG")) {
        qInfo().noquote() << "KIVO exe_dir=" << QCoreApplication::applicationDirPath();
        qInfo().noquote() << "KIVO library_paths=" << QCoreApplication::libraryPaths();
        qInfo().noquote() << "KIVO PATH=" << qgetenv("PATH");
        qInfo().noquote() << "KIVO QT_PLUGIN_PATH=" << qgetenv("QT_PLUGIN_PATH");
    }
    QCoreApplication::setApplicationName(QStringLiteral("kivo_video_playback_shell"));
    QCoreApplication::setOrganizationName(QStringLiteral("KivoVideo"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Kivo Video Playback Shell"));
    parser.addHelpOption();
    const ShellEvidenceOptions evidence_options = parse_shell_evidence_options(parser);

    kivo::ui::shell::PlaybackShellRuntimeContext runtime_context;
    auto* const bridge = runtime_context.bridge();
    if (bridge == nullptr) {
        qCritical() << "Playback shell runtime bridge unavailable";
        return 2;
    }

    QQmlApplicationEngine engine;
    const QString source_root = resolve_source_root();
    const auto registration =
        kivo::ui::shell::configure_playback_shell_engine(engine, source_root);
    if (!registration.ok) {
        qCritical().noquote() << registration.error_message;
        return 3;
    }

    engine.rootContext()->setContextProperty(QStringLiteral("shellRuntimeAdapter"), bridge);
    engine.rootContext()->setContextProperty(QStringLiteral("playbackPageUrl"),
                                             registration.playback_page_url);

    const QUrl shell_url(QStringLiteral("qrc:/kivo/ui/shell/PlaybackShellRoot.qml"));
    QQmlComponent shell_component(&engine, shell_url);
    if (shell_component.isError()) {
        const auto errors = shell_component.errors();
        for (const QQmlError& error : errors) {
            qCritical().noquote() << error.toString();
        }
        return 4;
    }

    QObject* const root = shell_component.create();
    if (root == nullptr) {
        qCritical() << "Failed to instantiate PlaybackShellRoot.qml";
        return 4;
    }
    engine.setObjectOwnership(root, QQmlEngine::CppOwnership);

    auto* const window = qobject_cast<QQuickWindow*>(root);
    if (window == nullptr) {
        qCritical() << "Playback shell root is not a window";
        return 5;
    }

    schedule_ui_open_evidence(window, evidence_options);
    if (evidence_options.interaction_enabled) {
        kivo::ui::shell::InteractionEvidenceOptions interaction{};
        interaction.output_dir = evidence_options.interaction_dir;
        interaction.close_delay_ms = evidence_options.interaction_close_ms;
        kivo::ui::shell::schedule_interaction_evidence(
            window,
            bridge,
            runtime_context.adapter(),
            interaction,
            registration.playback_page_url);
    }
    if (!evidence_options.ui_open_enabled && !evidence_options.interaction_enabled) {
        QObject::connect(window, &QQuickWindow::visibilityChanged, &app,
                         [](const QWindow::Visibility visibility) {
                             if (visibility == QWindow::Hidden) {
                                 QCoreApplication::quit();
                             }
                         });
    }

    qInfo().noquote() << "UI_OPEN_QML_ENTRY=qrc:/kivo/ui/shell/PlaybackShellRoot.qml";
    qInfo().noquote() << "UI_OPEN_PLAYBACK_PAGE=" << registration.playback_page_url;
    return QGuiApplication::exec();
}