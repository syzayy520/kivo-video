import QtQuick 6.5
import "../theme"

Item {
    id: root

    property string text: ""
    property string tooltipText: ""
    property bool selected: false
    property bool compact: false
    property bool buttonEnabled: true

    signal clicked(var source)

    width: Math.max(compact ? 42 : 58, label.implicitWidth + 24)
    height: compact ? 42 : 38
    enabled: buttonEnabled
    scale: mouseArea.pressed ? 0.97 : 1.0

    Accessible.role: Accessible.Button
    Accessible.name: tooltipText.length > 0 ? tooltipText : text

    Behavior on scale {
        NumberAnimation {
            duration: 70
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: mouseArea.containsMouse || root.selected ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
        border.color: root.selected ? Qt.rgba(1, 1, 1, 0.22) : "transparent"
        border.width: 1
    }

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: mouseArea.containsMouse || root.selected ? Qt.rgba(1, 1, 1, 0.96) : Qt.rgba(1, 1, 1, 0.68)
        font.pixelSize: compact ? 12 : 13
        font.weight: Font.DemiBold
        font.letterSpacing: 0
        font.family: PlaybackTypography.uiFamily
        renderType: Text.NativeRendering
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.buttonEnabled
        cursorShape: Qt.PointingHandCursor

        onClicked: root.clicked(root)
    }

    PlaybackTooltip {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.top
        anchors.bottomMargin: 8
        text: root.tooltipText
        shown: mouseArea.containsMouse && root.tooltipText.length > 0
    }
}
