import QtQuick 6.5
import QtQuick.Layouts 6.5
import "../controls"

RowLayout {
    id: root

    property bool paused: false

    signal previousRequested()
    signal rewindRequested()
    signal playPauseRequested()
    signal forwardRequested()
    signal nextRequested()
    signal interactionStarted()
    signal interactionEnded()

    spacing: 14

    PlaybackIconButton {
        iconName: "previous"
        tooltipText: "上一段"
        onClicked: root.previousRequested()
        visible: false
        Layout.preferredWidth: 0
    }

    PlaybackIconButton {
        iconName: "rewind10"
        tooltipText: "快退 10 秒"
        onClicked: root.rewindRequested()
        baseColor: Qt.rgba(1, 1, 1, 0.56)
        Layout.preferredWidth: 44
    }

    PlaybackIconButton {
        iconName: root.paused ? "play" : "pause"
        tooltipText: root.paused ? "播放" : "暂停"
        prominent: true
        onClicked: root.playPauseRequested()
        Layout.preferredWidth: 52
    }

    PlaybackIconButton {
        iconName: "forward10"
        tooltipText: "快进 10 秒"
        onClicked: root.forwardRequested()
        baseColor: Qt.rgba(1, 1, 1, 0.56)
        Layout.preferredWidth: 44
    }

    PlaybackIconButton {
        iconName: "next"
        tooltipText: "下一段"
        onClicked: root.nextRequested()
        visible: false
        Layout.preferredWidth: 0
    }
}
