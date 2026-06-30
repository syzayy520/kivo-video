import QtQuick 6.5

PlaybackSheet {
    id: root

    property string currentEpisode: ""
    property var episodes: []

    signal episodeRequested(string episodeId)

    title: "选集"

    Repeater {
        model: root.episodes

        Item {
            property string episodeId: modelData.episodeId || ""
            property string episodeName: modelData.name || ""
            property string durationText: modelData.durationText || ""
            property real progress: modelData.progress === undefined ? 0 : modelData.progress

            width: parent.width
            height: 46

            Rectangle {
                anchors.fill: parent
                radius: 8
                color: mouseArea.containsMouse || root.currentEpisode === episodeId ? Qt.rgba(1, 1, 1, 0.055) : "transparent"
                border.color: root.currentEpisode === episodeId ? Qt.rgba(0.96, 0.92, 0.84, 0.24) : "transparent"
                border.width: 1
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: episodeId
                color: Qt.rgba(1, 1, 1, 0.74)
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 0
            }

            Column {
                anchors.left: parent.left
                anchors.leftMargin: 58
                anchors.right: parent.right
                anchors.rightMargin: 92
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                Text {
                    width: parent.width
                    text: episodeName
                    color: Qt.rgba(1, 1, 1, 0.88)
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    font.letterSpacing: 0
                    elide: Text.ElideRight
                }

                Rectangle {
                    width: parent.width
                    height: 2
                    radius: 1
                    color: Qt.rgba(1, 1, 1, 0.12)

                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width * progress
                        height: 2
                        radius: 1
                        color: Qt.rgba(0.96, 0.92, 0.84, 0.58)
                    }
                }
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: root.currentEpisode === episodeId ? "当前" : durationText
                color: root.currentEpisode === episodeId ? Qt.rgba(0.96, 0.92, 0.84, 0.92) : Qt.rgba(1, 1, 1, 0.50)
                font.pixelSize: 12
                font.weight: Font.Medium
                font.letterSpacing: 0
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor

                onClicked: root.episodeRequested(episodeId)
            }
        }
    }
}
