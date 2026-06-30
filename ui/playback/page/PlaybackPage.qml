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
    property string audioTrackLabel: ""
    property string versionLabel: ""
    property string currentEpisode: ""
    property string selectedSubtitle: ""
    property var subtitleOptions: []
    property var audioTrackOptions: []
    property var episodeOptions: []
    property var versionOptions: []
    property var versionTechnicalRows: []
    property var moreActions: []
    property bool isPlaying: !isPaused
    property bool isPaused: false
    property bool isBuffering: false
    property int bufferingSeconds: 0
    property bool isRecovering: false
    property bool hasError: false
    property string errorUserMessage: "暂时无法播放"
    property string activeSheet: ""
    property real volume: 1.0
    property bool isMuted: false
    property bool fullscreenLike: false
    property bool autoHideEnabled: true
    property bool showPlaceholderVideo: true
    property var runtimeAdapter: null
    readonly property bool timelineDragging: overlay.stateModel.mode === 5
    property bool adapterSnapshotBound: runtimeAdapter !== null

    readonly property real durationSeconds: durationMs / 1000
    readonly property real positionSeconds: currentTimeMs / 1000
    readonly property real bufferedSeconds: durationSeconds * buffered
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
        if (root.fullscreenLike)
            dispatchRuntimeCommand("exitFullscreen", [], "exitFullscreen")
        else
            dispatchRuntimeCommand("enterFullscreen", [], "enterFullscreen")
    }

    function requestSheetShortcut(shortcutName, sheetName) {
        dispatchShortcut(shortcutName)
        overlay.stateModel.toggleSheet(sheetName)
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
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPositionChanged: overlay.stateModel.showVisible()
        onPressed: overlay.stateModel.showVisible()
        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                overlay.stateModel.showVisible()
                contextMenu.popup(mouse.x, mouse.y)
                return
            }

            if (overlay.stateModel.sheetOpen) {
                overlay.stateModel.closeSheet()
                return
            }

            root.dispatchRuntimeCommand("togglePlayPause", [], "playPause")
        }
        onDoubleClicked: {
            overlay.stateModel.showVisible()
            root.requestFullscreenThroughAdapter()
        }
    }

    PlaybackOverlay {
        id: overlay
        anchors.fill: parent
        title: root.title
        seasonEpisode: root.seasonEpisode
        primarySubtitle: root.subtitleEnabled ? root.subtitlePrimary : ""
        secondarySubtitle: root.subtitleEnabled ? root.subtitleSecondary : ""
        fullscreen: root.fullscreenLike
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
            root.playbackActionRequested(actionName)
        }
        onSubtitleTrackRequested: function(trackId) {
            root.dispatchRuntimeCommand("selectSubtitleTrack", [trackId], "selectSubtitleTrack")
        }
        onSubtitleSizeRequested: function(size) {
            root.dispatchRuntimeCommand("setSubtitleSize", [size], "setSubtitleSize")
        }
        onSubtitleDelayRequested: function(delayMs) {
            root.dispatchRuntimeCommand("setSubtitleDelay", [delayMs], "setSubtitleDelay")
        }
        onAudioTrackRequested: function(trackId) {
            root.dispatchRuntimeCommand("selectAudioTrack", [trackId], "selectAudioTrack")
        }
        onEpisodeRequested: function(episodeId) {
            root.dispatchOpenMediaId(episodeId, "openEpisode")
        }
        onVersionRequested: function(versionId) {
            root.dispatchOpenMediaId(versionId, "openVersion")
        }
        onRetryRequested: root.dispatchRuntimeCommand("retry", [], "retry")
    }

    PlaybackInputController {
        id: inputController
        overlayState: overlay.stateModel

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

        onRequestOpenFile: root.requestOpenFile()
        onRequestPlayPause: {
            root.dispatchRuntimeCommand("togglePlayPause", [], "playPause")
        }
        onRequestSkipBackward: function(ms) {
            root.dispatchRuntimeCommand("stepBackward", [ms], "rewind10")
        }
        onRequestSkipForward: function(ms) {
            root.dispatchRuntimeCommand("stepForward", [ms], "forward10")
        }
        onRequestSubtitleSheet: {
            overlay.stateModel.openSheet("subtitle")
            root.dispatchShortcut("subtitle")
        }
        onRequestAudioTrackSheet: {
            overlay.stateModel.openSheet("audio")
            root.dispatchShortcut("audioTrack")
        }
        onRequestEpisodeSheet: {
            overlay.stateModel.openSheet("episodes")
            root.requestEpisodeSheet()
        }
        onRequestVersionSheet: {
            overlay.stateModel.openSheet("version")
            root.requestVersionSheet()
        }
    }

    onIsBufferingChanged: {
        if (isBuffering)
            overlay.stateModel.setBuffering(bufferingSeconds)
        else if (overlay.stateModel.statusKind === "buffering")
            overlay.stateModel.clearStatus()
    }
    onBufferingSecondsChanged: if (isBuffering) overlay.stateModel.setBuffering(bufferingSeconds)
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
