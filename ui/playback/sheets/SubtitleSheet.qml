import QtQuick 6.5
import "../controls"

PlaybackSheet {
    id: root

    property string selectedSubtitle: ""
    property var subtitleOptions: []
    property int subtitleSize: 31
    property int subtitleDelayMs: 0

    signal subtitleRequested(string name)
    signal sizeRequested(int size)
    signal positionRequested(string position)
    signal delayRequested(int delayMs)

    title: "字幕"

    Column {
        width: parent.width
        spacing: 8

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

    Rectangle {
        width: parent.width
        height: 1
        color: Qt.rgba(1, 1, 1, 0.08)
    }

    Column {
        width: parent.width
        visible: root.subtitleOptions.length > 0
        spacing: 8

        Text {
            text: "样式"
            color: Qt.rgba(1, 1, 1, 0.58)
            font.pixelSize: 12
            font.weight: Font.Medium
            font.letterSpacing: 0
        }

        Row {
            spacing: 8

            PlaybackTextButton {
                text: "字号 -"
                tooltipText: "减小字号"
                onClicked: {
                    root.sizeRequested(Math.max(24, root.subtitleSize - 1))
                }
            }

            PlaybackTextButton {
                text: "字号 +"
                tooltipText: "增大字号"
                onClicked: {
                    root.sizeRequested(Math.min(38, root.subtitleSize + 1))
                }
            }

            PlaybackTextButton {
                text: "位置"
                tooltipText: "调整字幕位置"
                onClicked: root.positionRequested("safe-bottom")
            }

            PlaybackTextButton {
                text: "延迟"
                tooltipText: "调整字幕延迟"
                onClicked: root.delayRequested(root.subtitleDelayMs)
            }
        }
    }
}
