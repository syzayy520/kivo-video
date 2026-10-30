import QtQuick 6.5
import "../controls"

Item {
    id: root

    property real volume: 1.0
    property bool muted: false
    property bool compact: false
    property bool volumeAvailable: false

    signal volumeRequested(real value)
    signal muteRequested()
    signal interactionStarted()
    signal interactionEnded()

    width: compact ? 42 : 132
    height: 42

    PlaybackIconButton {
        id: volumeButton
        objectName: "volumeButton"
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        iconName: root.muted || root.volume <= 0 ? "muted" : "volume"
        tooltipText: root.muted ? qsTr("Unmute") : qsTr("Mute")
        visible: root.volumeAvailable
        buttonEnabled: root.volumeAvailable

        onClicked: {
            root.muteRequested()
        }
    }

    Item {
        id: lineWrap
        anchors.left: volumeButton.right
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: 20
        visible: opacity > 0
        opacity: root.volumeAvailable && !root.compact
                 && (volumeMouse.containsMouse || volumeMouse.pressed
                     || volumeButton.containsMouse) ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: 110
                easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: 3
            radius: 1.5
            color: Qt.rgba(1, 1, 1, 0.16)
        }

        Rectangle {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width * Math.max(0, Math.min(1, root.muted ? 0 : root.volume))
            height: 3
            radius: 1.5
            color: Qt.rgba(1, 1, 1, 0.70)
        }

        MouseArea {
            id: volumeMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor

            function requestAt(x) {
                root.volumeRequested(Math.max(0, Math.min(1, x / Math.max(1, width))))
            }

            onPressed: function(mouse) {
                root.interactionStarted()
                requestAt(mouse.x)
            }
            onPositionChanged: function(mouse) {
                if (pressed)
                    requestAt(mouse.x)
            }
            onReleased: root.interactionEnded()
            onCanceled: root.interactionEnded()
        }
    }
}
