import QtQuick 6.5
import "../theme"

Item {
    id: root

    property string primaryText: ""
    property string secondaryText: ""
    property bool controlsVisible: false
    property bool sheetOpen: false
    property bool statusActive: false

    anchors.fill: parent
    visible: primaryText.length > 0 || secondaryText.length > 0

    readonly property real safeBottom: parent ? parent.height * 0.11 : 86
    readonly property real liftedBottom: sheetOpen ? 250 : (controlsVisible || statusActive ? 176 : safeBottom)

    Column {
        id: subtitleColumn
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.liftedBottom
        width: Math.min(parent.width - 96, 920)
        spacing: 7

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            text: root.primaryText
            visible: text.length > 0
            color: Qt.rgba(1, 1, 1, 0.96)
            font.pixelSize: PlaybackTypography.subtitlePrimarySize
            font.weight: Font.Bold
            font.letterSpacing: 0
            font.family: PlaybackTypography.cjkFamily
            renderType: Text.NativeRendering
            lineHeight: 1.15
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            style: Text.Raised
            styleColor: Qt.rgba(0, 0, 0, 0.96)
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            text: root.secondaryText
            visible: text.length > 0
            color: Qt.rgba(1, 1, 1, 0.82)
            font.pixelSize: PlaybackTypography.subtitleSecondarySize
            font.weight: Font.Medium
            font.letterSpacing: 0
            font.family: PlaybackTypography.uiFamily
            renderType: Text.NativeRendering
            lineHeight: 1.18
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            style: Text.Raised
            styleColor: Qt.rgba(0, 0, 0, 0.88)
        }
    }
}
