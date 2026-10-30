import QtQuick 6.5
import "../controls"

PlaybackSheet {
    id: root

    property var actions: []
    property bool subtitleAvailable: false
    property bool audioAvailable: false
    property bool episodeAvailable: false
    property bool versionAvailable: false
    property bool settingsAvailable: false

    function actionMeaningful(action) {
        if (!action)
            return false
        var label = action.label === undefined || action.label === null ? "" : String(action.label)
        var actionName = action.actionName === undefined || action.actionName === null
                ? "" : String(action.actionName)
        return label.length > 0 && actionName.length > 0
    }

    readonly property var hubActions: {
        var items = []
        if (root.subtitleAvailable)
            items.push({ label: qsTr("Subtitles"), iconName: "subtitle", actionName: "subtitle" })
        if (root.audioAvailable)
            items.push({ label: qsTr("Audio"), iconName: "audio", actionName: "audio" })
        if (root.episodeAvailable)
            items.push({ label: qsTr("Episodes"), iconName: "episodes", actionName: "episodes" })
        if (root.versionAvailable)
            items.push({ label: qsTr("Versions"), iconName: "versions", actionName: "version" })
        if (root.actions && root.actions.length !== undefined) {
            for (var i = 0; i < root.actions.length; ++i) {
                var action = root.actions[i]
                if (root.actionMeaningful(action))
                    items.push(action)
            }
        }
        if (root.settingsAvailable)
            items.push({ label: qsTr("Settings"), iconName: "settings", actionName: "settings" })
        return items
    }

    signal actionRequested(string actionName)

    title: qsTr("More")

    Repeater {
        model: root.hubActions

        Item {
            property string label: modelData.label || ""
            property string iconName: modelData.iconName || "more"
            property string actionName: modelData.actionName || label

            width: parent.width
            height: 42

            Rectangle {
                anchors.fill: parent
                radius: 8
                color: mouseArea.containsMouse ? Qt.rgba(1, 1, 1, 0.052) : "transparent"
            }

            PlaybackIconSet {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                iconName: iconName
                strokeColor: Qt.rgba(1, 1, 1, 0.62)
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 46
                anchors.right: parent.right
                anchors.rightMargin: 42
                anchors.verticalCenter: parent.verticalCenter
                text: label
                color: Qt.rgba(1, 1, 1, 0.82)
                font.pixelSize: 13
                font.weight: Font.Medium
                font.letterSpacing: 0
                elide: Text.ElideRight
            }

            PlaybackIconSet {
                anchors.right: parent.right
                anchors.rightMargin: 13
                anchors.verticalCenter: parent.verticalCenter
                iconName: "chevronRight"
                strokeColor: Qt.rgba(1, 1, 1, 0.34)
                strokeWidth: 1.4
                implicitWidth: 16
                implicitHeight: 16
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
