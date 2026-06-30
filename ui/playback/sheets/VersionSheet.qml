import QtQuick 6.5
import "../controls"

PlaybackSheet {
    id: root

    property string selectedVersion: ""
    property var versionOptions: []
    property var technicalRows: []
    property bool technicalOpen: false

    signal versionRequested(string versionName)

    title: "版本"

    Repeater {
        model: root.versionOptions

        Item {
            property string quality: modelData.quality || ""
            property string detail: modelData.detail || ""
            property string value: modelData.value || quality

            width: parent.width
            height: 54

            Rectangle {
                anchors.fill: parent
                radius: 8
                color: mouseArea.containsMouse || root.selectedVersion === value ? Qt.rgba(1, 1, 1, 0.055) : "transparent"
                border.color: root.selectedVersion === value ? Qt.rgba(0.96, 0.92, 0.84, 0.24) : "transparent"
                border.width: 1
            }

            Column {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                Text {
                    text: quality
                    color: Qt.rgba(1, 1, 1, 0.88)
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0
                }

                Text {
                    text: detail
                    color: Qt.rgba(1, 1, 1, 0.55)
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    font.letterSpacing: 0
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor

                onClicked: root.versionRequested(value)
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        color: Qt.rgba(1, 1, 1, 0.08)
    }

    PlaybackTextButton {
        width: parent.width
        visible: root.technicalRows.length > 0
        text: root.technicalOpen ? "收起技术详情" : "技术详情"
        tooltipText: "技术详情"
        onClicked: root.technicalOpen = !root.technicalOpen
    }

    Column {
        width: parent.width
        visible: root.technicalOpen && root.technicalRows.length > 0
        spacing: 6

        Repeater {
            model: root.technicalRows

            Text {
                width: parent.width
                text: modelData
                color: Qt.rgba(1, 1, 1, 0.52)
                font.pixelSize: 12
                font.weight: Font.Medium
                font.letterSpacing: 0
                elide: Text.ElideRight
            }
        }
    }
}
