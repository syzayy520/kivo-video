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
    property real volume: 1.0
    property bool muted: false
    property bool playbackAvailable: false
    property string versionLabel: ""
    property bool subtitleAvailable: false
    property bool audioAvailable: false
    property bool episodeAvailable: false
    property bool versionAvailable: false
    property bool moreAvailable: false
    property bool seekAvailable: false

    readonly property bool compact: width < 760
    readonly property bool mediaControlsVisible: root.playbackAvailable
    readonly property bool timelineVisible: root.seekAvailable
    readonly property bool volumeChromeVisible: root.mediaControlsVisible
                                               && (root.muted
                                                   || Math.abs(root.volume - 1.0) > 0.001)

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
    height: compact ? (seekAvailable ? 118 : 72) : (seekAvailable ? 126 : 78)
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
        objectName: "playbackTimeline"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.compact ? 22 : 48
        anchors.rightMargin: root.compact ? 22 : 48
        anchors.top: parent.top
        visible: root.timelineVisible
        height: root.timelineVisible ? 48 : 0
        timelineAvailable: root.timelineVisible
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
        anchors.leftMargin: root.compact ? 18 : 48
        anchors.rightMargin: root.compact ? 18 : 48
        anchors.top: timeline.bottom
        anchors.topMargin: root.timelineVisible ? -4 : 10
        height: 52

        Item {
            Layout.fillWidth: true
            Layout.minimumWidth: root.compact ? 42 : 118
            Layout.preferredWidth: root.compact ? 42 : 146

            VolumeInlineControl {
                objectName: "volumeInlineControl"
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                visible: root.volumeChromeVisible
                compact: root.compact
                volumeAvailable: root.volumeChromeVisible
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
            Layout.preferredWidth: root.compact ? 194 : 248
            height: 56

            TransportControls {
                objectName: "transportControls"
                anchors.centerIn: parent
                visible: root.mediaControlsVisible
                playbackAvailable: root.playbackAvailable
                paused: root.paused
                seekAvailable: root.seekAvailable

                onPreviousRequested: root.previousRequested()
                onRewindRequested: root.rewindRequested()
                onPlayPauseRequested: root.playPauseRequested()
                onForwardRequested: root.forwardRequested()
                onNextRequested: root.nextRequested()
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.minimumWidth: root.compact ? 72 : 126
            Layout.preferredWidth: root.compact ? 86 : 146
            height: 42

            PlaybackToolControls {
                objectName: "playbackToolControls"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                compact: root.compact
                fullscreen: root.fullscreen
                fullscreenAvailable: root.mediaControlsVisible
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
