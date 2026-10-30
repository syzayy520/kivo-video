import QtQuick 6.5
import "../controls"

PlaybackSheet {
    id: root

    property var selectedSubtitle: ""
    property var subtitleOptions: []
    property real subtitleScale: 1.0
    property int subtitleDelayMs: 0

    signal subtitleRequested(var name)
    signal sizeRequested(real scale)
    signal positionRequested(string position)
    signal delayRequested(int delayMs)

    title: qsTr("Subtitles")

    Column {
        width: parent.width
        spacing: 8

        PlaybackTextButton {
            width: parent.width
            text: qsTr("Off")
            tooltipText: qsTr("Turn subtitles off")
            selected: Number(root.selectedSubtitle) === 0 || root.selectedSubtitle === ""

            onClicked: root.subtitleRequested(0)
        }

        Repeater {
            model: root.subtitleOptions

            PlaybackTextButton {
                property var subtitleId: modelData.trackId === undefined
                                        ? (modelData.id === undefined ? modelData : modelData.id)
                                        : modelData.trackId
                property string subtitleLabel: modelData.label === undefined
                                               ? (modelData.name === undefined ? String(modelData) : modelData.name)
                                               : modelData.label

                width: parent.width
                text: subtitleLabel
                tooltipText: subtitleLabel
                selected: root.selectedSubtitle === subtitleId || root.selectedSubtitle === subtitleLabel

                onClicked: root.subtitleRequested(subtitleId)
            }
        }
    }

}
