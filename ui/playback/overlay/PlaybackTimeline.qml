import QtQuick 6.5
import "../theme"

Item {
    id: root

    property real duration: 0
    property real position: 0
    property real bufferedPosition: 0
    property url previewImageSource: ""

    readonly property bool hovered: timelineMouse.containsMouse
    readonly property bool dragging: timelineMouse.pressed
    property real hoverX: 0

    signal seekRequested(real seconds)
    signal hoverStarted()
    signal hoverEnded()
    signal dragStarted()
    signal dragEnded()

    height: 58

    function clamp(value, minValue, maxValue) {
        return Math.max(minValue, Math.min(maxValue, value))
    }

    function ratioFromX(x) {
        if (barWrap.width <= 0)
            return 0
        return clamp(x / barWrap.width, 0, 1)
    }

    function secondsFromX(x) {
        return ratioFromX(x) * duration
    }

    function formatTime(seconds) {
        var bounded = Math.max(0, Math.floor(seconds))
        var hours = Math.floor(bounded / 3600)
        var minutes = Math.floor((bounded % 3600) / 60)
        var secs = bounded % 60
        if (hours > 0)
            return hours + ":" + (minutes < 10 ? "0" + minutes : minutes) + ":" + (secs < 10 ? "0" + secs : secs)
        return minutes + ":" + (secs < 10 ? "0" + secs : secs)
    }

    Text {
        id: currentTime
        anchors.left: parent.left
        anchors.verticalCenter: barWrap.verticalCenter
        width: root.duration >= 3600 ? 72 : 52
        text: root.formatTime(root.position)
        color: Qt.rgba(1, 1, 1, 0.70)
        font.pixelSize: PlaybackTypography.timeSize
        font.weight: Font.DemiBold
        font.letterSpacing: 0
        font.family: PlaybackTypography.uiFamily
        renderType: Text.NativeRendering
        horizontalAlignment: Text.AlignLeft
    }

    Text {
        id: totalTime
        anchors.right: parent.right
        anchors.verticalCenter: barWrap.verticalCenter
        width: root.duration >= 3600 ? 72 : 52
        text: root.formatTime(root.duration)
        color: Qt.rgba(1, 1, 1, 0.70)
        font.pixelSize: PlaybackTypography.timeSize
        font.weight: Font.DemiBold
        font.letterSpacing: 0
        font.family: PlaybackTypography.uiFamily
        renderType: Text.NativeRendering
        horizontalAlignment: Text.AlignRight
    }

    Item {
        id: barWrap
        anchors.left: currentTime.right
        anchors.leftMargin: 14
        anchors.right: totalTime.left
        anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        height: 28

        Rectangle {
            id: unplayed
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: root.hovered || root.dragging ? 5 : 2
            radius: height / 2
            color: Qt.rgba(1, 1, 1, 0.16)

            Behavior on height {
                NumberAnimation {
                    duration: 110
                    easing.type: Easing.OutCubic
                }
            }
        }

        Rectangle {
            anchors.left: unplayed.left
            anchors.verticalCenter: unplayed.verticalCenter
            width: unplayed.width * root.clamp(root.bufferedPosition / Math.max(1, root.duration), 0, 1)
            height: unplayed.height
            radius: height / 2
            color: Qt.rgba(1, 1, 1, 0.28)
        }

        Rectangle {
            anchors.left: unplayed.left
            anchors.verticalCenter: unplayed.verticalCenter
            width: unplayed.width * root.clamp(root.position / Math.max(1, root.duration), 0, 1)
            height: unplayed.height
            radius: height / 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#ffffff" }
                GradientStop { position: 1.0; color: "#d9dde4" }
            }
        }

        Rectangle {
            width: 7
            height: 7
            radius: 3.5
            x: root.clamp((root.position / Math.max(1, root.duration)) * unplayed.width - width / 2, -width / 2, unplayed.width - width / 2)
            anchors.verticalCenter: unplayed.verticalCenter
            opacity: root.hovered || root.dragging ? 1 : 0
            color: Qt.rgba(1, 1, 1, 0.96)

            Behavior on opacity {
                NumberAnimation {
                    duration: 90
                    easing.type: Easing.OutCubic
                }
            }
        }

        MouseArea {
            id: timelineMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor

            onEntered: root.hoverStarted()
            onExited: root.hoverEnded()
            onPressed: function(mouse) {
                root.hoverX = mouse.x
                root.dragStarted()
            }
            onPositionChanged: function(mouse) {
                root.hoverX = root.clamp(mouse.x, 0, width)
                if (pressed)
                    root.position = root.secondsFromX(mouse.x)
            }
            onReleased: function(mouse) {
                root.hoverX = root.clamp(mouse.x, 0, width)
                root.seekRequested(root.secondsFromX(mouse.x))
                root.dragEnded()
            }
        }

        Item {
            id: previewBubble
            visible: root.dragging
            width: root.previewImageSource == "" ? timeBubble.implicitWidth + 22 : 208
            height: root.previewImageSource == "" ? 30 : 148
            x: root.clamp(root.hoverX - width / 2, 0, Math.max(0, barWrap.width - width))
            y: -height - 8

            Rectangle {
                anchors.fill: parent
                radius: root.previewImageSource == "" ? 8 : 12
                color: Qt.rgba(0.035, 0.038, 0.045, 0.90)
                border.color: Qt.rgba(1, 1, 1, 0.10)
                border.width: 1
            }

            Image {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 8
                height: 108
                visible: root.previewImageSource != ""
                source: root.previewImageSource
                fillMode: Image.PreserveAspectCrop
                smooth: true
            }

            Text {
                id: timeBubble
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: root.previewImageSource == "" ? 7 : 10
                text: root.formatTime(root.secondsFromX(root.hoverX))
                color: Qt.rgba(1, 1, 1, 0.86)
                font.pixelSize: PlaybackTypography.captionSize
                font.weight: Font.DemiBold
                font.letterSpacing: 0
                font.family: PlaybackTypography.uiFamily
                renderType: Text.NativeRendering
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
