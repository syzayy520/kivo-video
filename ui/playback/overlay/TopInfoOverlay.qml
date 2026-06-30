import QtQuick 6.5
import QtQuick.Layouts 6.5
import "../controls"
import "../theme"

Item {
    id: root

    property bool overlayVisible: false
    property string title: ""
    property string subtitle: ""
    property bool moreAvailable: false

    signal backRequested()
    signal moreRequested(var source)

    opacity: overlayVisible ? 1 : 0
    visible: opacity > 0
    height: 116

    Behavior on opacity {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }

    RowLayout {
        anchors.left: parent.left
        anchors.leftMargin: 38
        anchors.top: parent.top
        anchors.topMargin: 32
        spacing: 18

        PlaybackIconButton {
            iconName: "close"
            tooltipText: "返回"
            onClicked: root.backRequested()
            Layout.alignment: Qt.AlignVCenter
        }

        Column {
            id: titleColumn

            Layout.preferredWidth: Math.min(root.width - 360, 760)
            spacing: 4

            Text {
                width: parent.width
                text: root.title
                visible: text.length > 0
                color: Qt.rgba(1, 1, 1, 0.94)
                font.pixelSize: PlaybackTypography.titleSize
                font.weight: Font.DemiBold
                font.letterSpacing: 0
                font.family: PlaybackTypography.uiFamily
                renderType: Text.NativeRendering
                elide: Text.ElideRight
                style: Text.Raised
                styleColor: Qt.rgba(0, 0, 0, 0.72)
            }

            Text {
                width: parent.width
                text: root.subtitle
                visible: text.length > 0
                color: Qt.rgba(1, 1, 1, 0.56)
                font.pixelSize: PlaybackTypography.captionSize
                font.weight: Font.Medium
                font.letterSpacing: 0
                font.family: PlaybackTypography.uiFamily
                renderType: Text.NativeRendering
                elide: Text.ElideRight
                style: Text.Raised
                styleColor: Qt.rgba(0, 0, 0, 0.55)
            }
        }
    }

    RowLayout {
        anchors.right: parent.right
        anchors.rightMargin: 36
        anchors.top: parent.top
        anchors.topMargin: 28
        spacing: 6

        PlaybackIconButton {
            iconName: "more"
            tooltipText: "更多"
            visible: root.moreAvailable
            onClicked: function(source) { root.moreRequested(source) }
            Layout.preferredWidth: visible ? 42 : 0
        }
    }
}
