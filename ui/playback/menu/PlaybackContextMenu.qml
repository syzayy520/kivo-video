import QtQuick 6.5
import QtQuick.Controls 6.5

Menu {
    id: root

    property bool subtitleAvailable: false
    property bool audioAvailable: false
    property bool episodeAvailable: false
    property bool versionAvailable: false
    property bool seekAvailable: false
    property bool playbackAvailable: false
    property bool moreAvailable: false
    property bool openActionsAvailable: true
    readonly property bool playbackCommandsAvailable: playbackAvailable || seekAvailable

    signal requestOpenFile()
    signal requestOpenFolder()
    signal requestPlayPause()
    signal requestSkipBackward(int ms)
    signal requestSkipForward(int ms)
    signal requestMoreSheet()

    MenuItem {
        objectName: "openVideoMenuItem"
        text: qsTr("Open Video")
        visible: root.openActionsAvailable
        enabled: root.openActionsAvailable
        onTriggered: root.requestOpenFile()
    }

    MenuItem {
        objectName: "openFolderMenuItem"
        text: qsTr("Open Folder")
        visible: root.openActionsAvailable
        enabled: root.openActionsAvailable
        onTriggered: root.requestOpenFolder()
    }

    MenuSeparator {
        objectName: "playbackMenuSeparator"
        visible: root.openActionsAvailable
                 && (root.playbackCommandsAvailable || root.moreAvailable)
    }

    MenuItem {
        objectName: "playPauseMenuItem"
        text: qsTr("Play / Pause")
        visible: root.playbackAvailable
        enabled: root.playbackAvailable
        onTriggered: root.requestPlayPause()
    }

    MenuItem {
        objectName: "skipBackwardMenuItem"
        text: qsTr("Back 10 seconds")
        visible: root.seekAvailable
        enabled: root.seekAvailable
        onTriggered: root.requestSkipBackward(10000)
    }

    MenuItem {
        objectName: "skipForwardMenuItem"
        text: qsTr("Forward 10 seconds")
        visible: root.seekAvailable
        enabled: root.seekAvailable
        onTriggered: root.requestSkipForward(10000)
    }

    MenuItem {
        objectName: "moreMenuItem"
        text: qsTr("More")
        visible: root.moreAvailable
        enabled: root.moreAvailable
        onTriggered: root.requestMoreSheet()
    }
}
