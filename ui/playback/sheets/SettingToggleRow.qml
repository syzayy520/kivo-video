import QtQuick 6.5
import QtQuick.Layouts 6.5

RowLayout {
    id: root

    property string label: ""
    property bool checked: false

    signal checkedRequested(bool checked)

    width: parent ? parent.width : implicitWidth
    height: 34
    spacing: 14

    function requestToggle() {
        root.checkedRequested(!root.checked)
    }

    Text {
        text: root.label
        color: Qt.rgba(1, 1, 1, 0.62)
        font.pixelSize: 12
        Layout.fillWidth: true
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    Item {
        id: toggle

        Layout.preferredWidth: 42
        Layout.preferredHeight: 22
        activeFocusOnTab: true

        Accessible.role: Accessible.CheckBox
        Accessible.name: root.label
        Accessible.checked: root.checked

        Keys.onSpacePressed: root.requestToggle()
        Keys.onReturnPressed: root.requestToggle()
        Keys.onEnterPressed: root.requestToggle()

        Rectangle {
            anchors.fill: parent
            radius: 11
            color: root.checked ? Qt.rgba(0.93, 0.91, 0.84, 0.22)
                                : Qt.rgba(1, 1, 1, 0.08)
            border.color: root.checked ? Qt.rgba(0.95, 0.92, 0.84, 0.42)
                                       : Qt.rgba(1, 1, 1, 0.16)
            border.width: toggle.activeFocus ? 2 : 1
        }

        Rectangle {
            width: 16
            height: 16
            radius: 8
            x: root.checked ? parent.width - width - 3 : 3
            anchors.verticalCenter: parent.verticalCenter
            color: root.checked ? Qt.rgba(0.96, 0.94, 0.88, 0.95)
                                : Qt.rgba(1, 1, 1, 0.42)

            Behavior on x {
                NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                toggle.forceActiveFocus()
                root.requestToggle()
            }
        }
    }
}
