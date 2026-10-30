import QtQuick 6.5
import "../controls"

Item {
    id: root

    property string label: ""
    property string valueText: ""
    property real value: 0
    property real minimumValue: 0
    property real maximumValue: 1
    property real stepSize: 1

    signal valueRequested(real value)

    width: parent ? parent.width : 320
    height: 42

    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width - 172
        text: root.label
        color: Qt.rgba(1, 1, 1, 0.72)
        font.pixelSize: 13
        font.weight: Font.Medium
        font.letterSpacing: 0
        elide: Text.ElideRight
    }

    Row {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6

        PlaybackIconButton {
            iconName: "minus"
            tooltipText: qsTr("Decrease %1").arg(root.label)
            buttonEnabled: root.value > root.minimumValue
            quiet: true
            onClicked: root.valueRequested(Math.max(root.minimumValue, root.value - root.stepSize))
        }

        Text {
            width: 64
            height: 38
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: root.valueText
            color: Qt.rgba(1, 1, 1, 0.88)
            font.pixelSize: 13
            font.weight: Font.DemiBold
            font.letterSpacing: 0
        }

        PlaybackIconButton {
            iconName: "plus"
            tooltipText: qsTr("Increase %1").arg(root.label)
            buttonEnabled: root.value < root.maximumValue
            quiet: true
            onClicked: root.valueRequested(Math.min(root.maximumValue, root.value + root.stepSize))
        }
    }
}
