import QtQuick 6.5
import QtQuick.Layouts 6.5
import "../controls"

RowLayout {
    id: root

    property bool paused: false
    property bool playbackAvailable: false
    property bool seekAvailable: false

    signal previousRequested()
    signal rewindRequested()
    signal playPauseRequested()
    signal forwardRequested()
    signal nextRequested()
    signal interactionStarted()
    signal interactionEnded()

    spacing: 10

    PlaybackIconButton {
        iconName: "previous"
        tooltipText: qsTr("Previous")
        onClicked: root.previousRequested()
        visible: false
        Layout.preferredWidth: 0
    }

    PlaybackIconButton {
        objectName: "rewindButton"
        iconName: "rewind10"
        tooltipText: qsTr("Back 10 seconds")
        visible: root.seekAvailable
        buttonEnabled: root.seekAvailable
        onClicked: root.rewindRequested()
        baseColor: Qt.rgba(1, 1, 1, 0.50)
        Layout.preferredWidth: visible ? 44 : 0
    }

    PlaybackIconButton {
        objectName: "playPauseButton"
        iconName: root.paused ? "play" : "pause"
        tooltipText: root.paused ? qsTr("Play") : qsTr("Pause")
        visible: root.playbackAvailable
        prominent: true
        onClicked: root.playPauseRequested()
        Layout.preferredWidth: visible ? 56 : 0
    }

    PlaybackIconButton {
        objectName: "forwardButton"
        iconName: "forward10"
        tooltipText: qsTr("Forward 10 seconds")
        visible: root.seekAvailable
        buttonEnabled: root.seekAvailable
        onClicked: root.forwardRequested()
        baseColor: Qt.rgba(1, 1, 1, 0.50)
        Layout.preferredWidth: visible ? 44 : 0
    }

    PlaybackIconButton {
        iconName: "next"
        tooltipText: qsTr("Next")
        onClicked: root.nextRequested()
        visible: false
        Layout.preferredWidth: 0
    }
}
