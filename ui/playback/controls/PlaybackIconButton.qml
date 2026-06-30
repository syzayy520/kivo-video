import QtQuick 6.5

Item {
    id: root

    property string iconName: "play"
    property string tooltipText: ""
    property bool checked: false
    property bool prominent: false
    property bool buttonEnabled: true
    property bool quiet: true
    property color baseColor: Qt.rgba(1, 1, 1, 0.66)
    property color hoverColor: Qt.rgba(1, 1, 1, 0.96)

    signal clicked(var source)

    width: prominent ? 52 : 44
    height: 44
    enabled: buttonEnabled
    scale: mouseArea.pressed ? 0.97 : 1.0

    Accessible.role: Accessible.Button
    Accessible.name: tooltipText

    Behavior on scale {
        NumberAnimation {
            duration: 70
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: {
            if (!root.quiet)
                return mouseArea.containsMouse || root.checked ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(1, 1, 1, 0.04)
            return mouseArea.containsMouse || root.checked ? Qt.rgba(1, 1, 1, 0.07) : "transparent"
        }
        border.color: "transparent"
        border.width: 1
    }

    PlaybackIconSet {
        anchors.centerIn: parent
        width: root.prominent ? 26 : 21
        height: root.prominent ? 26 : 21
        iconName: root.iconName
        strokeColor: root.prominent
                     ? Qt.rgba(1, 1, 1, mouseArea.containsMouse ? 1.0 : 0.92)
                     : (mouseArea.containsMouse || root.checked ? root.hoverColor : root.baseColor)
        strokeWidth: root.prominent ? 1.65 : 1.65
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
        shown: mouseArea.containsMouse
    }
}
