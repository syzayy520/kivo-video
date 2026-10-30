import QtQuick 6.5
import "../controls"

Column {
    id: root

    property string label: ""
    property var options: []
    property string selectedValue: ""

    signal optionRequested(string value)

    width: parent ? parent.width : 320
    spacing: 8

    Text {
        width: parent.width
        text: root.label
        color: Qt.rgba(1, 1, 1, 0.58)
        font.pixelSize: 12
        font.weight: Font.Medium
        font.letterSpacing: 0
        elide: Text.ElideRight
    }

    Flow {
        width: parent.width
        spacing: 8

        Repeater {
            model: root.options

            PlaybackTextButton {
                property string optionLabel: modelData.label || ""
                property string optionValue: modelData.value || optionLabel

                text: optionLabel
                tooltipText: optionLabel
                selected: root.selectedValue === optionValue
                onClicked: root.optionRequested(optionValue)
            }
        }
    }
}
