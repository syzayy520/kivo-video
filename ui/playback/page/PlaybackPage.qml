import QtQuick 6.5
import QtQuick.Controls 6.5
import "../overlay"
import "../input"
import "../menu"

FocusScope {
    id: root

    property string title: ""
    property string seasonEpisode: ""
    property int currentTimeMs: 0
    property int durationMs: 0
    property real progress: durationMs > 0 ? currentTimeMs / durationMs : 0
    property real buffered: 0
    property string subtitlePrimary: ""
    property string subtitleSecondary: ""
    property bool subtitleEnabled: false
    property int subtitleDelayMs: 0
    property var audioTrackLabel: ""
    property string versionLabel: ""
    property string currentEpisode: ""
    property var selectedSubtitle: ""
    property var subtitleOptions: []
    property var audioTrackOptions: []
    property var episodeOptions: []
    property var versionOptions: []
    property var versionTechnicalRows: []
    property var moreActions: []
    property bool isPlaying: !isPaused
    property bool isPaused: false
    property bool isBuffering: false
    property bool isOpening: false
    property string bufferReason: "none"
    property int rebufferCount: 0
    property string playbackState: ""
    property int bufferingSeconds: 0
    property bool isRecovering: false
    property bool hasError: false
    property string errorUserMessage: qsTr("Playback unavailable")
    property string activeSheet: ""
    property real volume: 1.0
    property bool isMuted: false
    property int audioDelayMs: 0
    property real subtitleSize: 1.0
    property real playbackSpeed: 1.0
    property string aspectMode: "fit"
    property string toneMappingMode: "auto"
    property bool fullscreenLike: false
    property bool autoPlayNext: false
    property bool playlistQueueConnected: false
    property bool playlistHasPrevious: false
    property bool playlistHasNext: false
    property bool playlistHasNextEpisode: false
    property bool autoHideEnabled: true
    property bool showPlaceholderVideo: true
    property var runtimeAdapter: null
    readonly property bool timelineDragging: overlay.stateModel.mode === 5
    property bool adapterSnapshotBound: runtimeAdapter !== null

    readonly property real durationSeconds: durationMs / 1000
    readonly property real positionSeconds: currentTimeMs / 1000
    readonly property real bufferedSeconds: durationSeconds * buffered
    readonly property bool contextMenuOpen: contextMenu.visible
    readonly property bool playbackStateClosed: playbackState === "NotCreated"
                                                   || playbackState === "Closed"
    readonly property bool playbackStateHasMedia: playbackState.length > 0
                                                   && playbackState !== "NotCreated"
                                                   && playbackState !== "Closed"
    readonly property bool mediaContextAvailable: !playbackStateClosed
                                                  && (durationMs > 0
                                                  || title.length > 0
                                                  || playbackStateHasMedia)
    property alias overlayState: overlay.stateModel

    signal requestPlayPause()
    signal requestSeek(int positionMs)
    signal requestSkipBackward(int ms)
    signal requestSkipForward(int ms)
    signal requestVolume(real volume)
    signal requestMuteToggle()
    signal requestSubtitleSheet()
    signal requestAudioTrackSheet()
    signal requestEpisodeSheet()
    signal requestVersionSheet()
    signal requestMoreSheet()
    signal requestFullscreenToggle()
    signal requestRetry()
    signal requestChooseVersion()
    signal requestShowReason()
    signal requestOpenFile()
    signal requestOpenFolder()
    signal backRequested()
    signal playbackActionRequested(string actionName)

    PlaybackAdapterDispatch {
        id: adapterDispatch
    }

    PlaybackSnapshotBinding {
        id: snapshotBinding
        runtimeAdapter: root.runtimeAdapter
        target: root
        dispatch: adapterDispatch
        skipRefresh: root.timelineDragging
    }

    function reportDispatchResult(result, fallbackActionName) {
        root.playbackActionRequested(result.ok ? result.actionName
                                               : ("adapterMissing:" + fallbackActionName))
        return result.ok
    }

    function dispatchRuntimeCommand(methodName, args, actionName) {
        return reportDispatchResult(
            adapterDispatch.dispatchRuntimeCommand(root.runtimeAdapter, methodName, args,
                                                   actionName, snapshotBinding),
            actionName)
    }

    function dispatchOpenMediaId(mediaId, actionName) {
        return reportDispatchResult(
            adapterDispatch.openMediaId(root.runtimeAdapter, mediaId, actionName, snapshotBinding),
            actionName)
    }

    function dispatchShortcut(rawName) {
        return reportDispatchResult(
            adapterDispatch.dispatchShortcut(root.runtimeAdapter, rawName, snapshotBinding),
            rawName)
    }

    function requestFullscreenThroughAdapter() {
        if (!root.mediaContextAvailable && !root.fullscreenLike)
            return false
        if (root.fullscreenLike)
            dispatchRuntimeCommand("exitFullscreen", [], "exitFullscreen")
        else
            dispatchRuntimeCommand("enterFullscreen", [], "enterFullscreen")
        return true
    }

    function requestFullscreenGesture() {
        if (overlay.stateModel.sheetOpen || overlay.stateModel.statusActive)
            return false
        if (!root.mediaContextAvailable && !root.fullscreenLike)
            return false
        overlay.stateModel.showVisible()
        return root.requestFullscreenThroughAdapter()
    }

    function syncLoadingState() {
        if (root.isOpening || root.isBuffering)
            overlay.stateModel.setBuffering(root.bufferingSeconds)
        else if (overlay.stateModel.statusKind === "buffering")
            overlay.stateModel.clearStatus()
    }

    function optionCount(options) {
        return options && options.length !== undefined ? options.length : 0
    }

    function sheetAvailable(sheetName) {
        if (!root.mediaContextAvailable)
            return false
        if (sheetName === "subtitle" || sheetName === "audio")
            return overlay.sheetAvailable(sheetName)
        if (sheetName === "episodes")
            return optionCount(root.episodeOptions) > 0
        if (sheetName === "version")
            return optionCount(root.versionOptions) > 0
        if (sheetName === "settings" || sheetName === "more")
            return overlay.sheetAvailable(sheetName)
        return false
    }

    function requestSheetShortcut(shortcutName, sheetName) {
        if (!sheetAvailable(sheetName))
            return false
        dispatchShortcut(shortcutName)
        overlay.stateModel.toggleSheet(sheetName)
        return true
    }

    function requestContextMenu(x, y) {
        if (overlay.stateModel.sheetOpen) {
            overlay.stateModel.closeSheet()
            return false
        }
        if (overlay.stateModel.statusActive)
            return false
        overlay.stateModel.showVisible()
        contextMenu.popup(x, y)
        return true
    }

    function closeContextMenu() {
        contextMenu.close()
    }

    width: 1280
    height: 720
    focus: true

    Keys.onPressed: function(event) {
        inputController.handle(event)
    }

    VideoLayer {
        anchors.fill: parent
        visible: root.showPlaceholderVideo
        z: 0
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        z: 5

        onPositionChanged: overlay.stateModel.showVisible()
        onPressed: overlay.stateModel.showVisible()
        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                root.requestContextMenu(mouse.x, mouse.y)
                return
            }

            if (overlay.stateModel.sheetOpen) {
                overlay.stateModel.closeSheet()
                return
            }

            overlay.stateModel.toggleVisible()
        }
        onDoubleClicked: {
            root.requestFullscreenGesture()
        }
    }

    PlaybackOverlay {
        id: overlay
        anchors.fill: parent
        z: 10
        title: root.title
        seasonEpisode: root.seasonEpisode
        primarySubtitle: root.subtitleEnabled ? root.subtitlePrimary : ""
        secondarySubtitle: root.subtitleEnabled ? root.subtitleSecondary : ""
        fullscreen: root.fullscreenLike
        playbackAvailable: root.mediaContextAvailable
        autoHideEnabled: root.autoHideEnabled
        volume: root.volume
        muted: root.isMuted
        duration: root.durationSeconds
        position: root.positionSeconds
        bufferedPosition: root.bufferedSeconds
        audioTrackLabel: root.audioTrackLabel
        versionLabel: root.versionLabel
        currentEpisode: root.currentEpisode
        selectedSubtitle: root.selectedSubtitle
        subtitleDelayMs: root.subtitleDelayMs
        subtitleSize: root.subtitleSize
        audioDelayMs: root.audioDelayMs
        playbackSpeed: root.playbackSpeed
        aspectMode: root.aspectMode
        toneMappingMode: root.toneMappingMode
        autoPlayNext: root.autoPlayNext
        playlistHasNext: root.playlistHasNext
        playlistHasNextEpisode: root.playlistHasNextEpisode
        subtitleOptions: root.subtitleOptions
        audioTrackOptions: root.audioTrackOptions
        episodeOptions: root.episodeOptions
        versionOptions: root.versionOptions
        versionTechnicalRows: root.versionTechnicalRows
        moreActions: root.moreActions
        errorMessage: root.errorUserMessage

        onSeekRequested: function(seconds) {
            root.dispatchRuntimeCommand("seek", [Math.round(seconds * 1000)], "seek")
        }
        onVolumeRequested: function(value) {
            root.dispatchRuntimeCommand("setVolume", [Math.max(0, Math.min(1, value))], "setVolume")
        }
        onMuteRequested: {
            root.dispatchRuntimeCommand("setMuted", [!root.isMuted], "setMuted")
        }
        onPlayPauseRequested: {
            root.dispatchRuntimeCommand("togglePlayPause", [], "playPause")
        }
        onRewindRequested: {
            root.dispatchRuntimeCommand("stepBackward", [10000], "rewind10")
        }
        onForwardRequested: {
            root.dispatchRuntimeCommand("stepForward", [10000], "forward10")
        }
        onBackRequested: root.backRequested()
        onOpenFileRequested: root.requestOpenFile()
        onPreviousRequested: root.dispatchRuntimeCommand("previous", [], "previous")
        onNextRequested: root.dispatchRuntimeCommand("next", [], "next")
        onFullscreenRequested: {
            root.requestFullscreenThroughAdapter()
        }
        onSheetActionRequested: function(actionName) {
            if (actionName === "settings")
                overlay.stateModel.openSheet("settings")
            else
                root.playbackActionRequested(actionName)
        }
        onSubtitleTrackRequested: function(trackId) {
            if (Number(trackId) === 0)
                root.dispatchRuntimeCommand("disableSubtitle", [], "disableSubtitle")
            else
                root.dispatchRuntimeCommand("selectSubtitleTrack", [trackId], "selectSubtitleTrack")
        }
        onSubtitleSizeRequested: function(size) {
            root.dispatchRuntimeCommand("setSubtitleSize", [size], "setSubtitleSize")
        }
        onSubtitleDelayRequested: function(delayMs) {
            root.dispatchRuntimeCommand("setSubtitleDelay", [delayMs], "setSubtitleDelay")
        }
        onAudioDelayRequested: function(delayMs) {
            root.dispatchRuntimeCommand("setAudioDelay", [delayMs], "setAudioDelay")
        }
        onPlaybackSpeedRequested: function(speed) {
            root.dispatchRuntimeCommand("setPlaybackSpeed", [speed], "setPlaybackSpeed")
        }
        onAspectModeRequested: function(mode) {
            root.dispatchRuntimeCommand("setAspectMode", [mode], "setAspectMode")
        }
        onToneMappingModeRequested: function(mode) {
            root.dispatchRuntimeCommand("setToneMappingMode", [mode], "setToneMappingMode")
        }
        onAutoPlayNextRequested: function(enabled) {
            root.dispatchRuntimeCommand("setAutoPlayNext", [enabled], "setAutoPlayNext")
        }
        onAudioTrackRequested: function(trackId) {
            root.dispatchRuntimeCommand("selectAudioTrack", [trackId], "selectAudioTrack")
        }
        onEpisodeRequested: function(episodeId) {
            root.dispatchRuntimeCommand("playPlaylistItem", [episodeId], "playPlaylistItem")
        }
        onVersionRequested: function(versionId) {
            root.dispatchOpenMediaId(versionId, "openVersion")
        }
        onRetryRequested: root.dispatchRuntimeCommand("retry", [], "retry")
    }

    PlaybackInputController {
        id: inputController
        objectName: "playbackInputController"
        overlayState: overlay.stateModel
        playbackAvailable: root.mediaContextAvailable
        seekAvailable: root.mediaContextAvailable && root.durationMs > 0
        subtitleAvailable: root.subtitleOptions.length > 0
        audioAvailable: overlay.audioTrackChoiceAvailable
        episodeAvailable: root.episodeOptions.length > 0
        versionAvailable: root.versionOptions.length > 0

        onRequestPlayPause: {
            root.dispatchRuntimeCommand("togglePlayPause", [], "playPause")
        }
        onRequestSkipBackward: function(ms) {
            root.dispatchRuntimeCommand("stepBackward", [ms], "rewind10")
        }
        onRequestSkipForward: function(ms) {
            root.dispatchRuntimeCommand("stepForward", [ms], "forward10")
        }
        onRequestVolumeStep: function(delta) {
            root.dispatchRuntimeCommand("setVolume",
                                        [Math.max(0, Math.min(1, root.volume + delta))],
                                        delta > 0 ? "volumeUp" : "volumeDown")
        }
        onRequestMuteToggle: {
            root.dispatchRuntimeCommand("setMuted", [!root.isMuted], "setMuted")
        }
        onRequestSubtitleSheet: root.requestSheetShortcut("subtitle", "subtitle")
        onRequestAudioTrackSheet: root.requestSheetShortcut("audioTrack", "audio")
        onRequestEpisodeSheet: root.requestSheetShortcut("episode", "episodes")
        onRequestVersionSheet: root.requestSheetShortcut("version", "version")
        onRequestFullscreenToggle: {
            root.requestFullscreenThroughAdapter()
        }
        onRequestOpenFile: root.requestOpenFile()
        onRequestBack: {
            if (root.fullscreenLike) {
                root.dispatchRuntimeCommand("exitFullscreen", [], "exitFullscreen")
            } else {
                overlay.stateModel.showVisible()
            }
        }
    }

    PlaybackContextMenu {
        id: contextMenu
        objectName: "playbackContextMenu"
        seekAvailable: root.mediaContextAvailable && root.durationMs > 0
        subtitleAvailable: root.subtitleOptions.length > 0
        audioAvailable: overlay.audioTrackChoiceAvailable
        episodeAvailable: root.episodeOptions.length > 0
        versionAvailable: root.versionOptions.length > 0
        playbackAvailable: root.mediaContextAvailable
        moreAvailable: overlay.moreHubAvailable
        openActionsAvailable: !root.mediaContextAvailable

        onRequestOpenFile: root.requestOpenFile()
        onRequestOpenFolder: root.requestOpenFolder()
        onRequestPlayPause: {
            root.dispatchRuntimeCommand("togglePlayPause", [], "playPause")
        }
        onRequestSkipBackward: function(ms) {
            if (!root.mediaContextAvailable || root.durationMs <= 0)
                return
            root.dispatchRuntimeCommand("stepBackward", [ms], "rewind10")
        }
        onRequestSkipForward: function(ms) {
            if (!root.mediaContextAvailable || root.durationMs <= 0)
                return
            root.dispatchRuntimeCommand("stepForward", [ms], "forward10")
        }
        onRequestMoreSheet: {
            if (!overlay.openSheetIfAvailable("more"))
                return
            root.requestMoreSheet()
        }
    }

    onIsBufferingChanged: syncLoadingState()
    onIsOpeningChanged: syncLoadingState()
    onBufferingSecondsChanged: if (isOpening || isBuffering) overlay.stateModel.setBuffering(bufferingSeconds)
    onIsRecoveringChanged: if (isRecovering) overlay.stateModel.setRecovering()
    onHasErrorChanged: if (hasError) overlay.stateModel.setError()
    onIsPausedChanged: overlay.stateModel.setPaused(isPaused)

    Component.onCompleted: {
        if (root.isPaused)
            overlay.stateModel.setPaused(true)
        forceActiveFocus()
    }

    Component.onDestruction: {
        dispatchRuntimeCommand("releaseSurface", [], "releaseSurface")
        dispatchRuntimeCommand("close", [], "close")
    }
}
