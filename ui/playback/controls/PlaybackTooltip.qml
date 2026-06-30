import QtQuick 6.5

Item {
    id: root

    property string text: ""
    property bool shown: false

    visible: opacity > 0
    opacity: shown && text.length > 0 ? 1 : 0
    width: label.implicitWidth + 16
    height: 24
    z: 100

    Behavior on opacity {
        NumberAnimation {
            duration: 110
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: Qt.rgba(0.04, 0.045, 0.052, 0.88)
        border.color: Qt.rgba(1, 1, 1, 0.08)
        border.width: 1
    }

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: Qt.rgba(1, 1, 1, 0.82)
        font.pixelSize: 11
        font.weight: Font.Medium
        font.letterSpacing: 0
    }
}
