import QtQuick 6.5
import QtQuick.Layouts 6.5
import "../controls"

RowLayout {
    id: root

    property bool compact: false
    property bool fullscreen: false
    property bool fullscreenAvailable: false
    property string versionLabel: ""
    property bool subtitleAvailable: false
    property bool audioAvailable: false
    property bool episodeAvailable: false
    property bool versionAvailable: false
    property bool moreAvailable: false

    signal sheetRequested(string sheetName, var source)
    signal fullscreenRequested()

    spacing: compact ? 2 : 4

    PlaybackIconButton {
        iconName: "subtitle"
        tooltipText: qsTr("Subtitles")
        visible: false
        baseColor: Qt.rgba(1, 1, 1, 0.54)
        onClicked: function(source) { root.sheetRequested("subtitle", source) }
        Layout.preferredWidth: visible ? 42 : 0
    }

    PlaybackIconButton {
        iconName: "audio"
        tooltipText: qsTr("Audio")
        visible: false
        onClicked: function(source) { root.sheetRequested("audio", source) }
        Layout.preferredWidth: visible ? 42 : 0
    }

    PlaybackIconButton {
        iconName: "episodes"
        tooltipText: qsTr("Episodes")
        visible: false
        onClicked: function(source) { root.sheetRequested("episodes", source) }
        Layout.preferredWidth: visible ? 42 : 0
    }

    PlaybackTextButton {
        id: versionButton

        text: root.versionLabel.length > 0 ? root.versionLabel : qsTr("Versions")
        tooltipText: qsTr("Versions")
        visible: false
        onClicked: function(source) { root.sheetRequested("version", source) }
        Layout.preferredWidth: visible ? Math.max(56, implicitWidth + 18) : 0
    }

    PlaybackIconButton {
        objectName: "moreToolButton"
        iconName: "more"
        tooltipText: qsTr("More")
        visible: root.moreAvailable
        baseColor: Qt.rgba(1, 1, 1, 0.54)
        onClicked: function(source) { root.sheetRequested("more", source) }
        Layout.preferredWidth: visible ? 42 : 0
    }

    PlaybackIconButton {
        objectName: "fullscreenToolButton"
        iconName: "fullscreen"
        tooltipText: root.fullscreen ? qsTr("Exit Full Screen") : qsTr("Full Screen")
        visible: root.fullscreenAvailable || root.fullscreen
        baseColor: Qt.rgba(1, 1, 1, 0.54)
        onClicked: root.fullscreenRequested()
        Layout.preferredWidth: visible ? 42 : 0
    }
}
