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
    property real volume: 1.0
    property bool muted: false
    property bool fullscreen: false
    property bool playbackAvailable: false
    property bool autoHideEnabled: true
    property var audioTrackLabel: ""
    property string versionLabel: ""
    property string currentEpisode: ""
    property var selectedSubtitle: ""
    property int subtitleDelayMs: 0
    property real subtitleSize: 1.0
    property int audioDelayMs: 0
    property real playbackSpeed: 1.0
    property string aspectMode: "fit"
    property string toneMappingMode: "auto"
    property bool autoPlayNext: false
    property bool playlistHasNext: false
    property bool playlistHasNextEpisode: false
    property var subtitleOptions: []
    property var audioTrackOptions: []
    property var episodeOptions: []
    property var versionOptions: []
    property var versionTechnicalRows: []
    property var moreActions: []
    property string errorMessage: ""
    readonly property int meaningfulMoreActionCount: countMeaningfulActions(root.moreActions)
    readonly property bool audioTrackChoiceAvailable: optionCount(root.audioTrackOptions) > 1
    readonly property bool audioAdjustmentAvailable: root.audioTrackChoiceAvailable
                                                    || root.audioDelayMs !== 0
    readonly property bool subtitleControlsAvailable: root.playbackAvailable
                                                    && (root.subtitleOptions.length > 0
                                                        || root.primarySubtitle.length > 0
                                                        || root.secondarySubtitle.length > 0)
    readonly property bool audioControlsAvailable: root.playbackAvailable
                                                 && root.audioAdjustmentAvailable
    readonly property bool autoPlayNextControlsAvailable: root.playbackAvailable
                                                       && (root.episodeOptions.length > 0
                                                           || root.playlistHasNext
                                                           || root.playlistHasNextEpisode)
    readonly property bool advancedSettingsAvailable: root.playbackAvailable
                                                    && (root.subtitleControlsAvailable
                                                        || root.audioControlsAvailable
                                                        || root.autoPlayNextControlsAvailable
                                                        || Math.abs(root.playbackSpeed - 1.0) > 0.001)
    readonly property bool moreHubAvailable: root.playbackAvailable
                                           && (root.subtitleOptions.length > 0
                                               || root.audioTrackChoiceAvailable
                                               || root.episodeOptions.length > 0
                                               || root.versionOptions.length > 0
                                               || root.meaningfulMoreActionCount > 0
                                               || root.advancedSettingsAvailable)
    readonly property bool chromeVisible: overlayState.overlayVisible && !overlayState.statusActive
    readonly property bool bottomChromeAvailable: root.playbackAvailable
                                                  || root.moreHubAvailable
                                                  || root.fullscreen
    readonly property bool bottomChromeVisible: root.chromeVisible && root.bottomChromeAvailable
    readonly property bool topInfoAvailable: root.title.length > 0 || root.seasonEpisode.length > 0
    readonly property bool topChromeVisible: root.chromeVisible && root.topInfoAvailable
    readonly property bool seekAvailable: root.playbackAvailable && root.duration > 0

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
    signal subtitleSizeRequested(real size)
    signal subtitleDelayRequested(int delayMs)
    signal audioDelayRequested(int delayMs)
    signal playbackSpeedRequested(real speed)
    signal aspectModeRequested(string mode)
    signal toneMappingModeRequested(string mode)
    signal autoPlayNextRequested(bool enabled)
    signal audioTrackRequested(var trackId)
    signal episodeRequested(var episodeId)
    signal versionRequested(var versionId)

    function optionCount(options) {
        return options && options.length !== undefined ? options.length : 0
    }

    function actionMeaningful(action) {
        if (!action)
            return false
        var label = action.label === undefined || action.label === null ? "" : String(action.label)
        var actionName = action.actionName === undefined || action.actionName === null
                ? "" : String(action.actionName)
        return label.length > 0 && actionName.length > 0
    }

    function countMeaningfulActions(actions) {
        if (!actions || actions.length === undefined)
            return 0
        var count = 0
        for (var i = 0; i < actions.length; ++i) {
            if (actionMeaningful(actions[i]))
                count += 1
        }
        return count
    }

    function sheetAvailable(sheetName) {
        if (sheetName === "subtitle")
            return optionCount(root.subtitleOptions) > 0
        if (sheetName === "audio")
            return root.audioTrackChoiceAvailable
        if (sheetName === "episodes")
            return optionCount(root.episodeOptions) > 0
        if (sheetName === "version")
            return optionCount(root.versionOptions) > 0
        if (sheetName === "settings")
            return root.advancedSettingsAvailable
        if (sheetName === "more")
            return root.moreHubAvailable
        return false
    }

    function openSheetIfAvailable(sheetName) {
        if (!sheetAvailable(sheetName))
            return false
        overlayState.openSheet(sheetName)
        return true
    }

    function toggleSheetIfAvailable(sheetName) {
        if (!sheetAvailable(sheetName))
            return false
        overlayState.toggleSheet(sheetName)
        return true
    }

    PlaybackOverlayState {
        id: overlayState
    }

    PlaybackOverlayTimer {
        overlayState: overlayState
        autoHideEnabled: root.autoHideEnabled
    }

    BottomGradientOverlay {
        objectName: "bottomGradientOverlay"
        overlayVisible: root.bottomChromeVisible
        z: 1
    }

    TopGradientOverlay {
        objectName: "topGradientOverlay"
        overlayVisible: root.topChromeVisible
        z: 1
    }

    TopInfoOverlay {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        objectName: "topInfoOverlay"
        overlayVisible: root.topChromeVisible
        title: root.title
        subtitle: root.seasonEpisode
        moreAvailable: false
        z: 4

        onBackRequested: root.backRequested()
        onMoreRequested: function(source) {
            root.openSheetIfAvailable("more")
            if (source)
                source.forceActiveFocus()
        }
    }

    SubtitleOverlay {
        objectName: "subtitleOverlay"
        controlsVisible: root.chromeVisible
        sheetOpen: overlayState.sheetOpen
        statusActive: overlayState.statusActive
        primaryText: root.primarySubtitle
        secondaryText: root.secondarySubtitle
        textScale: root.subtitleSize
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
        objectName: "bottomControlOverlay"
        overlayVisible: root.bottomChromeVisible
        paused: overlayState.paused
        fullscreen: root.fullscreen
        duration: root.duration
        position: root.position
        bufferedPosition: root.bufferedPosition
        volume: root.volume
        muted: root.muted
        playbackAvailable: root.playbackAvailable
        versionLabel: root.versionLabel
        subtitleAvailable: root.subtitleOptions.length > 0
        audioAvailable: root.audioTrackChoiceAvailable
        episodeAvailable: root.episodeOptions.length > 0
        versionAvailable: root.versionOptions.length > 0
        moreAvailable: root.moreHubAvailable
        seekAvailable: root.seekAvailable
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
            if (!root.toggleSheetIfAvailable(sheetName))
                return
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
        anchors.rightMargin: 32
        anchors.bottom: bottomControls.top
        anchors.bottomMargin: 14
        open: overlayState.activeSheet === "subtitle"
        selectedSubtitle: root.selectedSubtitle
        subtitleDelayMs: root.subtitleDelayMs
        subtitleScale: root.subtitleSize
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
        anchors.rightMargin: 32
        anchors.bottom: bottomControls.top
        anchors.bottomMargin: 14
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
        anchors.rightMargin: 32
        anchors.bottom: bottomControls.top
        anchors.bottomMargin: 14
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
        anchors.rightMargin: 32
        anchors.bottom: bottomControls.top
        anchors.bottomMargin: 14
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
        anchors.rightMargin: 32
        anchors.bottom: bottomControls.top
        anchors.bottomMargin: 14
        open: overlayState.activeSheet === "more"
        actions: root.moreActions
        subtitleAvailable: root.subtitleOptions.length > 0
        audioAvailable: root.audioTrackChoiceAvailable
        episodeAvailable: root.episodeOptions.length > 0
        versionAvailable: root.versionOptions.length > 0
        settingsAvailable: root.advancedSettingsAvailable
        z: 30
        onCloseRequested: overlayState.closeSheet()
        onActionRequested: function(actionName) {
            if (actionName === "subtitle" || actionName === "audio"
                    || actionName === "episodes" || actionName === "version"
                    || actionName === "settings") {
                root.openSheetIfAvailable(actionName)
            } else {
                root.sheetActionRequested(actionName)
            }
        }
    }

    SettingsSheet {
        objectName: "settingsSheet"
        anchors.right: parent.right
        anchors.rightMargin: 32
        anchors.bottom: bottomControls.top
        anchors.bottomMargin: 14
        open: overlayState.activeSheet === "settings"
        playbackSpeed: root.playbackSpeed
        subtitleScale: root.subtitleSize
        subtitleDelayMs: root.subtitleDelayMs
        audioDelayMs: root.audioDelayMs
        aspectMode: root.aspectMode
        toneMappingMode: root.toneMappingMode
        autoPlayNext: root.autoPlayNext
        subtitleControlsAvailable: root.subtitleControlsAvailable
        audioControlsAvailable: root.audioControlsAvailable
        playbackSpeedControlsAvailable: root.playbackAvailable
        autoPlayNextControlsAvailable: root.autoPlayNextControlsAvailable
        z: 30
        onCloseRequested: overlayState.closeSheet()
        onPlaybackSpeedRequested: function(speed) { root.playbackSpeedRequested(speed) }
        onSubtitleScaleRequested: function(scale) { root.subtitleSizeRequested(scale) }
        onSubtitleDelayRequested: function(delayMs) { root.subtitleDelayRequested(delayMs) }
        onAudioDelayRequested: function(delayMs) { root.audioDelayRequested(delayMs) }
        onAspectModeRequested: function(mode) { root.aspectModeRequested(mode) }
        onToneMappingModeRequested: function(mode) { root.toneMappingModeRequested(mode) }
        onAutoPlayNextRequested: function(enabled) { root.autoPlayNextRequested(enabled) }
    }

    StatusOverlay {
        objectName: "statusOverlay"
        statusKind: overlayState.statusKind
        bufferingSeconds: overlayState.bufferingSeconds
        errorMessage: root.errorMessage
        retryAvailable: root.playbackAvailable
        z: 35

        onRetryRequested: root.retryRequested()
        onOpenFileRequested: root.openFileRequested()
    }
}
