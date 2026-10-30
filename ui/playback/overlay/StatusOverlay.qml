import QtQuick 6.5
import QtQuick.Layouts 6.5
import "../controls"
import "../theme"

Item {
    id: root

    property string statusKind: ""
    property int bufferingSeconds: 0
    property string errorMessage: ""
    property bool retryAvailable: false

    readonly property string errorTitle: qsTr("Playback unavailable")
    readonly property string errorDetailText: root.errorMessage === undefined
                                                 || root.errorMessage === null
                                             ? ""
                                             : String(root.errorMessage).trim()
    readonly property bool errorDetailVisible: root.statusKind === "error"
                                               && root.errorDetailText.length > 0
                                               && root.errorDetailText !== root.errorTitle

    signal retryRequested()
    signal openFileRequested()

    anchors.fill: parent
    visible: statusKind.length > 0

    Item {
        id: centerStack
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 340)
        height: content.implicitHeight

        Column {
            id: content
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 12

            Item {
                id: spinner
                anchors.horizontalCenter: parent.horizontalCenter
                width: 22
                height: 22
                visible: root.statusKind === "buffering" || root.statusKind === "recovering"

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: "transparent"
                    border.color: Qt.rgba(1, 1, 1, 0.22)
                    border.width: 2
                }

                Rectangle {
                    width: 4
                    height: 4
                    radius: 2
                    color: Qt.rgba(0.96, 0.92, 0.84, 0.82)
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                }

                RotationAnimator on rotation {
                    running: spinner.visible
                    loops: Animation.Infinite
                    from: 0
                    to: 360
                    duration: 900
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: centerStack.width
                visible: root.statusKind !== "buffering" || root.bufferingSeconds >= 2
                text: {
                    if (root.statusKind === "buffering")
                        return root.bufferingSeconds >= 6 ? qsTr("Still buffering") : qsTr("Buffering")
                    if (root.statusKind === "recovering")
                        return qsTr("Recovering playback")
                    if (root.statusKind === "recovered")
                        return qsTr("Playback recovered")
                    if (root.statusKind === "error")
                        return root.errorTitle
                    return ""
                }
                color: Qt.rgba(1, 1, 1, 0.88)
                font.pixelSize: root.statusKind === "error" ? 18 : 14
                font.weight: Font.DemiBold
                font.letterSpacing: 0
                font.family: PlaybackTypography.uiFamily
                renderType: Text.NativeRendering
                horizontalAlignment: Text.AlignHCenter
                lineHeight: 1.24
                style: Text.Raised
                styleColor: Qt.rgba(0, 0, 0, 0.72)
            }

            Text {
                objectName: "statusErrorDetailText"
                anchors.horizontalCenter: parent.horizontalCenter
                width: centerStack.width
                visible: root.errorDetailVisible
                text: root.errorDetailText
                color: Qt.rgba(1, 1, 1, 0.52)
                font.pixelSize: PlaybackTypography.captionSize
                font.weight: Font.Medium
                font.letterSpacing: 0
                font.family: PlaybackTypography.uiFamily
                renderType: Text.NativeRendering
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                lineHeight: 1.24
            }

            RowLayout {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: root.statusKind === "error"
                spacing: 8

                PlaybackTextButton {
                    objectName: "retryButton"
                    text: qsTr("Retry")
                    tooltipText: qsTr("Retry playback")
                    visible: root.retryAvailable
                    onClicked: root.retryRequested()
                }

                PlaybackTextButton {
                    objectName: "openVideoButton"
                    text: qsTr("Open Video")
                    tooltipText: qsTr("Open a local video file")
                    visible: !root.retryAvailable
                    onClicked: root.openFileRequested()
                }
            }
        }
    }
}
