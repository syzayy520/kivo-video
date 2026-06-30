import QtQuick 6.5
import "../controls"
import "../theme"

Item {
    id: root

    property string title: ""
    property bool open: false
    default property alias contentData: body.data

    signal closeRequested()

    width: Math.min(parent ? parent.width - 48 : 420, 420)
    height: Math.min(parent ? parent.height * 0.68 : 520, shell.implicitHeight + 32)
    visible: opacity > 0
    opacity: open ? 1 : 0
    clip: true

    Behavior on opacity {
        NumberAnimation {
            duration: 140
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 14
        color: Qt.rgba(0.055, 0.058, 0.066, 0.86)
        border.color: Qt.rgba(1, 1, 1, 0.10)
        border.width: 1
    }

    Column {
        id: shell
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 14

        Row {
            width: parent.width
            height: 32

            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 42
                text: root.title
                color: Qt.rgba(1, 1, 1, 0.88)
                font.pixelSize: 15
                font.weight: Font.DemiBold
                font.letterSpacing: 0
                font.family: PlaybackTypography.uiFamily
                renderType: Text.NativeRendering
                elide: Text.ElideRight
            }

            PlaybackIconButton {
                anchors.verticalCenter: parent.verticalCenter
                iconName: "close"
                tooltipText: "关闭"
                onClicked: root.closeRequested()
            }
        }

        Column {
            id: body
            width: parent.width
            spacing: 10
        }
    }
}
