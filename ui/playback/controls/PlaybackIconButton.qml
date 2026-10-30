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
    readonly property bool containsMouse: mouseArea.containsMouse

    signal clicked(var source)

    width: prominent ? 56 : 42
    height: prominent ? 56 : 42
    enabled: buttonEnabled
    scale: mouseArea.pressed ? 0.97 : 1.0
    opacity: buttonEnabled ? 1.0 : 0.36

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
        anchors.margins: root.prominent ? 1 : 3
        radius: width / 2
        color: root.prominent
               ? Qt.rgba(1, 1, 1, mouseArea.containsMouse ? 0.18 : 0.12)
               : Qt.rgba(1, 1, 1, mouseArea.containsMouse || root.checked
                                ? (root.quiet ? 0.055 : 0.09)
                                : 0)
        border.color: root.prominent ? Qt.rgba(1, 1, 1, 0.16) : "transparent"
        border.width: root.prominent ? 1 : 0

        Behavior on color {
            ColorAnimation {
                duration: 100
                easing.type: Easing.OutCubic
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: root.prominent ? 48 : 0
        height: width
        radius: width / 2
        color: "transparent"
        border.color: Qt.rgba(1, 1, 1, 0.08)
        border.width: root.prominent ? 1 : 0
        opacity: root.prominent ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: 90
                easing.type: Easing.OutCubic
            }
        }
    }

    PlaybackIconSet {
        anchors.centerIn: parent
        width: root.prominent ? 30 : 22
        height: root.prominent ? 30 : 22
        iconName: root.iconName
        strokeColor: root.prominent
                     ? Qt.rgba(1, 1, 1, mouseArea.containsMouse ? 1.0 : 0.92)
                     : (mouseArea.containsMouse || root.checked ? root.hoverColor : root.baseColor)
        strokeWidth: root.prominent ? 1.9 : 1.65
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
