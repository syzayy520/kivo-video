import QtQuick 6.5

Rectangle {
    id: root

    property bool overlayVisible: false

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: parent ? parent.height * 0.24 : 190
    visible: opacity > 0
    opacity: overlayVisible ? 1 : 0

    gradient: Gradient {
        GradientStop { position: 0.00; color: Qt.rgba(0, 0, 0, 0.58) }
        GradientStop { position: 0.48; color: Qt.rgba(0, 0, 0, 0.24) }
        GradientStop { position: 1.00; color: Qt.rgba(0, 0, 0, 0.00) }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: 160
            easing.type: Easing.OutCubic
        }
    }
}
