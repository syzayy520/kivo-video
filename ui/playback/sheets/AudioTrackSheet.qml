import QtQuick 6.5
import "../controls"

PlaybackSheet {
    id: root

    property string selectedTrack: ""
    property var trackOptions: []

    signal trackRequested(string trackName)

    title: "音轨"

    Repeater {
        model: root.trackOptions

        PlaybackTextButton {
            property var trackId: modelData.trackId === undefined
                                  ? (modelData.id === undefined ? modelData : modelData.id)
                                  : modelData.trackId
            property string trackLabel: modelData.label === undefined
                                        ? (modelData.name === undefined ? String(modelData) : modelData.name)
                                        : modelData.label

            width: parent.width
            text: trackLabel
            tooltipText: trackLabel
            selected: root.selectedTrack === trackId || root.selectedTrack === trackLabel

            onClicked: root.trackRequested(trackId)
        }
    }
}
