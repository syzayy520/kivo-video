import QtQuick 6.5
import "../controls"

PlaybackSheet {
    id: root

    property var actions: []

    signal actionRequested(string actionName)

    title: "更多"

    Repeater {
        model: root.actions

        Item {
            property string label: modelData.label || ""
            property string iconName: modelData.iconName || "more"
            property string actionName: modelData.actionName || label

            width: parent.width
            height: 38

            Rectangle {
                anchors.fill: parent
                radius: 8
                color: mouseArea.containsMouse ? Qt.rgba(1, 1, 1, 0.055) : "transparent"
            }

            PlaybackIconSet {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                iconName: iconName
                strokeColor: Qt.rgba(1, 1, 1, 0.64)
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 44
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: label
                color: Qt.rgba(1, 1, 1, 0.80)
                font.pixelSize: 13
                font.weight: Font.Medium
                font.letterSpacing: 0
                elide: Text.ElideRight
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor

                onClicked: root.actionRequested(actionName)
            }
        }
    }
}
