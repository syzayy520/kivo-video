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
    height: 86

    Behavior on opacity {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }

    RowLayout {
        anchors.left: parent.left
        anchors.leftMargin: 34
        anchors.top: parent.top
        anchors.topMargin: 22
        spacing: 14

        PlaybackIconButton {
            iconName: "chevronLeft"
            tooltipText: qsTr("Back")
            quiet: true
            baseColor: Qt.rgba(1, 1, 1, 0.52)
            onClicked: root.backRequested()
            Layout.alignment: Qt.AlignVCenter
        }

        Column {
            id: titleColumn

            Layout.preferredWidth: Math.min(root.width - 320, 680)
            spacing: 2

            Text {
                width: parent.width
                text: root.title
                visible: text.length > 0
                color: Qt.rgba(1, 1, 1, 0.88)
                font.pixelSize: Math.max(16, PlaybackTypography.titleSize - 2)
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
                color: Qt.rgba(1, 1, 1, 0.48)
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
        anchors.rightMargin: 34
        anchors.top: parent.top
        anchors.topMargin: 22
        spacing: 6

        PlaybackIconButton {
            iconName: "more"
            tooltipText: qsTr("More")
            visible: root.moreAvailable
            quiet: true
            baseColor: Qt.rgba(1, 1, 1, 0.52)
            onClicked: function(source) { root.moreRequested(source) }
            Layout.preferredWidth: visible ? 42 : 0
        }
    }
}
