import QtQuick 6.5
import "../states"
import "../sheets"

Item {
    id: root

    property alias stateModel: overlayState
    property string title: ""
    property string seasonEpisode: ""
    property string primarySubtitle: ""
    property string secondarySubtitle: ""
    property real duration: 0
    property real position: 0
    property real bufferedPosition: 0
    property real volume: 0
    property bool muted: false
    property bool fullscreen: false
    property bool autoHideEnabled: true
    property string audioTrackLabel: ""
    property string versionLabel: ""
    property string currentEpisode: ""
    property string selectedSubtitle: ""
    property int subtitleDelayMs: 0
    property var subtitleOptions: []
    property var audioTrackOptions: []
    property var episodeOptions: []
    property var versionOptions: []
    property var versionTechnicalRows: []
    property var moreActions: []
    property string errorMessage: ""

    signal playPauseRequested()
    signal rewindRequested()
    signal forwardRequested()
    signal previousRequested()
    signal nextRequested()
    signal seekRequested(real seconds)
    signal volumeRequested(real value)
    signal muteRequested()
    signal fullscreenRequested()
    signal backRequested()
    signal openFileRequested()
    signal retryRequested()
    signal sheetActionRequested(string actionName)
    signal subtitleTrackRequested(var trackId)
    signal subtitleSizeRequested(int size)
    signal subtitleDelayRequested(int delayMs)
    signal audioTrackRequested(var trackId)
    signal episodeRequested(var episodeId)
    signal versionRequested(var versionId)

    PlaybackOverlayState {
        id: overlayState
    }

    PlaybackOverlayTimer {
        overlayState: overlayState
        autoHideEnabled: root.autoHideEnabled
    }

    BottomGradientOverlay {
        overlayVisible: overlayState.overlayVisible
        z: 1
    }

    TopGradientOverlay {
        overlayVisible: overlayState.overlayVisible
        z: 1
    }

    TopInfoOverlay {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        overlayVisible: overlayState.overlayVisible
        title: root.title
        subtitle: root.seasonEpisode
        moreAvailable: root.moreActions.length > 0
        z: 4

        onBackRequested: root.backRequested()
        onMoreRequested: function(source) {
            overlayState.openSheet("more")
            if (source)
                source.forceActiveFocus()
        }
    }

    SubtitleOverlay {
        controlsVisible: overlayState.overlayVisible
        sheetOpen: overlayState.sheetOpen
        statusActive: overlayState.statusActive
        primaryText: root.primarySubtitle
        secondaryText: root.secondarySubtitle
        z: 3
    }

    MouseArea {
        anchors.fill: parent
        visible: overlayState.sheetOpen
        acceptedButtons: Qt.LeftButton
        z: 7

        onClicked: overlayState.closeSheet()
    }

    BottomControlOverlay {
        id: bottomControls
        overlayVisible: overlayState.overlayVisible
        paused: overlayState.paused
        fullscreen: root.fullscreen
        duration: root.duration
        position: root.position
        bufferedPosition: root.bufferedPosition
        volume: root.volume
        muted: root.muted
        versionLabel: root.versionLabel
        subtitleAvailable: root.subtitleOptions.length > 0
        audioAvailable: root.audioTrackOptions.length > 0
        episodeAvailable: root.episodeOptions.length > 0
        versionAvailable: root.versionOptions.length > 0
        moreAvailable: root.moreActions.length > 0
        z: 10

        onSeekRequested: function(seconds) { root.seekRequested(seconds) }
        onVolumeRequested: function(value) { root.volumeRequested(value) }
        onMuteRequested: root.muteRequested()
        onPlayPauseRequested: root.playPauseRequested()
        onRewindRequested: root.rewindRequested()
        onForwardRequested: root.forwardRequested()
        onPreviousRequested: root.previousRequested()
        onNextRequested: root.nextRequested()
        onSheetRequested: function(sheetName, source) {
            overlayState.toggleSheet(sheetName)
            if (source)
                source.forceActiveFocus()
        }
        onFullscreenRequested: root.fullscreenRequested()
        onInteractionStarted: overlayState.beginInteraction()
        onInteractionEnded: overlayState.endInteraction()
        onTimelineHoverStarted: overlayState.beginTimelineHover()
        onTimelineHoverEnded: overlayState.endInteraction()
        onTimelineDragStarted: overlayState.beginTimelineDrag()
        onTimelineDragEnded: overlayState.endTimelineDrag()
    }

    SubtitleSheet {
        objectName: "subtitleSheet"
        anchors.right: parent.right
        anchors.rightMargin: 38
        anchors.bottom: bottomControls.top
        anchors.bottomMargin: 18
        open: overlayState.activeSheet === "subtitle"
        selectedSubtitle: root.selectedSubtitle
        subtitleDelayMs: root.subtitleDelayMs
        subtitleOptions: root.subtitleOptions
        z: 30
        onCloseRequested: overlayState.closeSheet()
        onSubtitleRequested: function(trackId) { root.subtitleTrackRequested(trackId) }
        onSizeRequested: function(size) { root.subtitleSizeRequested(size) }
        onDelayRequested: function(delayMs) { root.subtitleDelayRequested(delayMs) }
    }

    AudioTrackSheet {
        objectName: "audioTrackSheet"
        anchors.right: parent.right
        anchors.rightMargin: 38
        anchors.bottom: bottomControls.top
        anchors.bottomMargin: 18
        open: overlayState.activeSheet === "audio"
        selectedTrack: root.audioTrackLabel
        trackOptions: root.audioTrackOptions
        z: 30
        onCloseRequested: overlayState.closeSheet()
        onTrackRequested: function(trackId) { root.audioTrackRequested(trackId) }
    }

    EpisodeSheet {
        objectName: "episodeSheet"
        anchors.right: parent.right
        anchors.rightMargin: 38
        anchors.bottom: bottomControls.top
        anchors.bottomMargin: 18
        open: overlayState.activeSheet === "episodes"
        currentEpisode: root.currentEpisode
        episodes: root.episodeOptions
        z: 30
        onCloseRequested: overlayState.closeSheet()
        onEpisodeRequested: function(episodeId) { root.episodeRequested(episodeId) }
    }

    VersionSheet {
        objectName: "versionSheet"
        anchors.right: parent.right
        anchors.rightMargin: 38
        anchors.bottom: bottomControls.top
        anchors.bottomMargin: 18
        open: overlayState.activeSheet === "version"
        selectedVersion: root.versionLabel
        versionOptions: root.versionOptions
        technicalRows: root.versionTechnicalRows
        z: 30
        onCloseRequested: overlayState.closeSheet()
        onVersionRequested: function(versionId) { root.versionRequested(versionId) }
    }

    MorePlaybackSheet {
        objectName: "morePlaybackSheet"
        anchors.right: parent.right
        anchors.rightMargin: 38
        anchors.bottom: bottomControls.top
        anchors.bottomMargin: 18
        open: overlayState.activeSheet === "more"
        actions: root.moreActions
        z: 30
        onCloseRequested: overlayState.closeSheet()
        onActionRequested: function(actionName) { root.sheetActionRequested(actionName) }
    }

    StatusOverlay {
        statusKind: overlayState.statusKind
        bufferingSeconds: overlayState.bufferingSeconds
        errorMessage: root.errorMessage
        z: 35

        onRetryRequested: root.retryRequested()
        onOpenFileRequested: root.openFileRequested()
    }
}
