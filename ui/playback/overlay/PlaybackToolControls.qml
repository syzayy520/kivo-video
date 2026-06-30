import QtQuick 6.5
import QtQuick.Layouts 6.5
import "../controls"

RowLayout {
    id: root

    property bool compact: false
    property bool fullscreen: false
    property string versionLabel: ""
    property bool subtitleAvailable: false
    property bool audioAvailable: false
    property bool episodeAvailable: false
    property bool versionAvailable: false
    property bool moreAvailable: false

    signal sheetRequested(string sheetName, var source)
    signal fullscreenRequested()

    spacing: compact ? 4 : 6

    PlaybackTextButton {
        text: "CC"
        tooltipText: "字幕"
        visible: root.subtitleAvailable
        compact: true
        onClicked: function(source) { root.sheetRequested("subtitle", source) }
        Layout.preferredWidth: visible ? 42 : 0
    }

    PlaybackIconButton {
        iconName: "audio"
        tooltipText: "音轨"
        visible: !root.compact && root.audioAvailable
        onClicked: function(source) { root.sheetRequested("audio", source) }
        Layout.preferredWidth: visible ? 42 : 0
    }

    PlaybackIconButton {
        iconName: "episodes"
        tooltipText: "选集"
        visible: !root.compact && root.episodeAvailable
        onClicked: function(source) { root.sheetRequested("episodes", source) }
        Layout.preferredWidth: visible ? 42 : 0
    }

    PlaybackTextButton {
        id: versionButton

        text: root.versionLabel.length > 0 ? root.versionLabel : "版本"
        tooltipText: "版本"
        visible: !root.compact && root.versionAvailable
        onClicked: function(source) { root.sheetRequested("version", source) }
        Layout.preferredWidth: visible ? Math.max(56, implicitWidth + 18) : 0
    }

    PlaybackIconButton {
        iconName: "more"
        tooltipText: "更多"
        visible: root.moreAvailable
        onClicked: function(source) { root.sheetRequested("more", source) }
        Layout.preferredWidth: visible ? 42 : 0
    }

    PlaybackIconButton {
        iconName: "fullscreen"
        tooltipText: root.fullscreen ? "退出全屏" : "全屏"
        onClicked: root.fullscreenRequested()
        Layout.preferredWidth: 42
    }
}
