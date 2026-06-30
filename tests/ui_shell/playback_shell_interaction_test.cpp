#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>

static bool validate_evidence(const QJsonObject& evidence) {
    if (!evidence.value(QStringLiteral("shell_started")).toBool()) {
        return false;
    }
    if (!evidence.value(QStringLiteral("qml_loaded")).toBool()) {
        return false;
    }
    if (!evidence.value(QStringLiteral("playback_page_loaded_from_qrc")).toBool()) {
        return false;
    }
    if (evidence.value(QStringLiteral("fake_content_used")).toBool()) {
        return false;
    }
    if (evidence.value(QStringLiteral("fake_media_used")).toBool()) {
        return false;
    }
    if (!evidence.contains(QStringLiteral("snapshot_before"))) {
        return false;
    }
    if (!evidence.contains(QStringLiteral("snapshot_after"))) {
        return false;
    }
    const QJsonArray commands = evidence.value(QStringLiteral("commands_executed")).toArray();
    const QJsonArray results = evidence.value(QStringLiteral("command_results")).toArray();
    if (commands.isEmpty() || results.isEmpty()) {
        return false;
    }
    if (!evidence.value(QStringLiteral("screenshot_exists")).toBool()) {
        return false;
    }
    return evidence.value(QStringLiteral("exit")).toString() == QStringLiteral("normal");
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    const QString shell_exe =
        QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("kivo_video_playback_shell.exe"));
    if (!QFile::exists(shell_exe)) {
        return 1;
    }

    QTemporaryDir temp_dir;
    if (!temp_dir.isValid()) {
        return 2;
    }

    QProcess process;
    process.setProgram(shell_exe);
    process.setArguments(QStringList{
        QStringLiteral("--interaction-evidence"),
        temp_dir.path(),
        QStringLiteral("--interaction-close-ms"),
        QStringLiteral("800"),
    });
    process.setWorkingDirectory(QCoreApplication::applicationDirPath());
    process.start();
    if (!process.waitForStarted(5000)) {
        return 3;
    }
    if (!process.waitForFinished(30000)) {
        process.kill();
        return 4;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return 5;
    }

    QFile evidence_file(temp_dir.filePath(QStringLiteral("interaction_evidence.json")));
    if (!evidence_file.open(QIODevice::ReadOnly)) {
        return 6;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(evidence_file.readAll());
    if (!doc.isObject() || !validate_evidence(doc.object())) {
        return 7;
    }

    const QString screenshot_path = temp_dir.filePath(QStringLiteral("interaction_screenshot.png"));
    if (!QFile::exists(screenshot_path)) {
        return 8;
    }

    return 0;
}