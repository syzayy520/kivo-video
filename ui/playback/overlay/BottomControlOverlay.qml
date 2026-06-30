import QtQuick 6.5
import QtQuick.Layouts 6.5

Item {
    id: root

    property bool overlayVisible: false
    property bool paused: false
    property bool fullscreen: false
    property real duration: 0
    property real position: 0
    property real bufferedPosition: 0
    property real volume: 0
    property bool muted: false
    property string versionLabel: ""
    property bool subtitleAvailable: false
    property bool audioAvailable: false
    property bool episodeAvailable: false
    property bool versionAvailable: false
    property bool moreAvailable: false

    readonly property bool compact: width < 760

    signal seekRequested(real seconds)
    signal volumeRequested(real value)
    signal muteRequested()
    signal playPauseRequested()
    signal rewindRequested()
    signal forwardRequested()
    signal previousRequested()
    signal nextRequested()
    signal sheetRequested(string sheetName, var source)
    signal fullscreenRequested()
    signal interactionStarted()
    signal interactionEnded()
    signal timelineHoverStarted()
    signal timelineHoverEnded()
    signal timelineDragStarted()
    signal timelineDragEnded()

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    height: compact ? 142 : 150
    opacity: overlayVisible ? 1 : 0
    visible: opacity > 0

    Behavior on opacity {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }

    PlaybackTimeline {
        id: timeline
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.compact ? 24 : 40
        anchors.rightMargin: root.compact ? 24 : 40
        anchors.top: parent.top
        duration: root.duration
        position: root.position
        bufferedPosition: root.bufferedPosition

        onSeekRequested: function(seconds) { root.seekRequested(seconds) }
        onHoverStarted: root.timelineHoverStarted()
        onHoverEnded: root.timelineHoverEnded()
        onDragStarted: root.timelineDragStarted()
        onDragEnded: root.timelineDragEnded()
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.compact ? 20 : 40
        anchors.rightMargin: root.compact ? 20 : 40
        anchors.top: timeline.bottom
        anchors.topMargin: 2
        height: 52

        Item {
            Layout.fillWidth: true
            Layout.minimumWidth: root.compact ? 42 : 196
            Layout.preferredWidth: root.compact ? 42 : 218

            VolumeInlineControl {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                compact: root.compact
                volume: root.volume
                muted: root.muted

                onVolumeRequested: function(value) { root.volumeRequested(value) }
                onMuteRequested: root.muteRequested()
                onInteractionStarted: root.interactionStarted()
                onInteractionEnded: root.interactionEnded()
            }
        }

        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: root.compact ? 220 : 286
            height: 48

            TransportControls {
                anchors.centerIn: parent
                paused: root.paused

                onPreviousRequested: root.previousRequested()
                onRewindRequested: root.rewindRequested()
                onPlayPauseRequested: root.playPauseRequested()
                onForwardRequested: root.forwardRequested()
                onNextRequested: root.nextRequested()
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.minimumWidth: root.compact ? 72 : 210
            Layout.preferredWidth: root.compact ? 86 : 238
            height: 42

            PlaybackToolControls {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                compact: root.compact
                fullscreen: root.fullscreen
                versionLabel: root.versionLabel
                subtitleAvailable: root.subtitleAvailable
                audioAvailable: root.audioAvailable
                episodeAvailable: root.episodeAvailable
                versionAvailable: root.versionAvailable
                moreAvailable: root.moreAvailable

                onSheetRequested: function(sheetName, source) { root.sheetRequested(sheetName, source) }
                onFullscreenRequested: root.fullscreenRequested()
            }
        }
    }
}
