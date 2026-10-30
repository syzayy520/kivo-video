import QtQuick 6.5
import "../controls"
import "../theme"

Item {
    id: root

    property string title: ""
    property bool open: false
    default property alias contentData: body.data
    readonly property real sheetMargin: 14
    readonly property real headerHeight: 32
    readonly property real bodySpacing: 12
    readonly property real maximumSheetHeight: parent ? parent.height * 0.62 : 500
    readonly property real naturalSheetHeight: sheetMargin * 2
                                               + headerHeight
                                               + bodySpacing
                                               + body.implicitHeight

    signal closeRequested()

    width: Math.min(parent ? parent.width - 48 : 392, 392)
    height: Math.min(maximumSheetHeight, naturalSheetHeight)
    visible: opacity > 0
    opacity: open ? 1 : 0
    clip: true
    transform: Translate {
        y: root.open ? 0 : 12

        Behavior on y {
            NumberAnimation {
                duration: 150
                easing.type: Easing.OutCubic
            }
        }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: 140
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: Qt.rgba(0.045, 0.048, 0.055, 0.88)
        border.color: Qt.rgba(1, 1, 1, 0.085)
        border.width: 1
    }

    Column {
        id: shell
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: root.sheetMargin
        spacing: root.bodySpacing

        Row {
            id: headerRow
            width: parent.width
            height: root.headerHeight

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
                tooltipText: qsTr("Close")
                onClicked: root.closeRequested()
            }
        }

        Item {
            id: viewport

            width: parent.width
            height: Math.max(0, shell.height - headerRow.height - shell.spacing)

            Flickable {
                id: scroller

                anchors.fill: parent
                clip: true
                contentWidth: width
                contentHeight: body.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                interactive: contentHeight > height

                Column {
                    id: body

                    width: scroller.width
                    spacing: 10
                }
            }

            Rectangle {
                anchors.right: parent.right
                width: 2
                height: scroller.contentHeight <= scroller.height
                        ? 0
                        : Math.max(24, parent.height * parent.height / scroller.contentHeight)
                radius: width / 2
                y: scroller.contentHeight <= scroller.height
                   ? 0
                   : scroller.contentY * (parent.height - height)
                     / Math.max(1, scroller.contentHeight - scroller.height)
                color: Qt.rgba(1, 1, 1, 0.20)
                opacity: scroller.moving || scroller.dragging ? 1 : 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: 110
                        easing.type: Easing.OutCubic
                    }
                }
            }
        }
    }
}
