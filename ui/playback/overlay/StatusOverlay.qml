import QtQuick 6.5
import QtQuick.Layouts 6.5
import "../controls"
import "../theme"

Item {
    id: root

    property string statusKind: ""
    property int bufferingSeconds: 0
    property string errorMessage: ""

    signal retryRequested()
    signal openFileRequested()

    anchors.fill: parent
    visible: statusKind.length > 0

    Item {
        id: centerStack
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 360)
        height: content.implicitHeight

        Column {
            id: content
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 14

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
                    if (root.statusKind === "buffering" && root.bufferingSeconds >= 6)
                        return "当前片源加载较慢，正在继续缓冲..."
                    if (root.statusKind === "buffering")
                        return "正在缓冲..."
                    if (root.statusKind === "recovering")
                        return "当前片源加载较慢\n正在恢复播放..."
                    if (root.statusKind === "recovered")
                        return "播放已恢复"
                    if (root.statusKind === "error")
                        return "暂时无法播放"
                    return ""
                }
                color: Qt.rgba(1, 1, 1, 0.88)
                font.pixelSize: root.statusKind === "error" ? 20 : 15
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
                anchors.horizontalCenter: parent.horizontalCenter
                width: centerStack.width
                visible: root.statusKind === "error" && root.errorMessage.length > 0
                text: root.errorMessage
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
                    text: "重试"
                    tooltipText: "重试播放"
                    onClicked: root.retryRequested()
                }

                PlaybackTextButton {
                    text: "打开视频"
                    tooltipText: "打开本地视频文件"
                    onClicked: root.openFileRequested()
                }
            }
        }
    }
}
