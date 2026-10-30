import QtQuick 6.5

Item {
    id: root

    property var overlayState: null
    property int skipStepMs: 10000
    property real volumeStep: 0.05
    property bool playbackAvailable: false
    property bool seekAvailable: false
    property bool subtitleAvailable: false
    property bool audioAvailable: false
    property bool episodeAvailable: false
    property bool versionAvailable: false

    visible: false

    signal requestPlayPause()
    signal requestSkipBackward(int ms)
    signal requestSkipForward(int ms)
    signal requestVolumeStep(real delta)
    signal requestMuteToggle()
    signal requestSubtitleSheet()
    signal requestAudioTrackSheet()
    signal requestEpisodeSheet()
    signal requestVersionSheet()
    signal requestFullscreenToggle()
    signal requestOpenFile()
    signal requestBack()

    PlaybackShortcutMap {
        id: shortcutMap
    }

    function revealsChrome(action) {
        return actionAvailable(action)
                && (action === "subtitleSheet"
                || action === "audioSheet"
                || action === "episodeSheet"
                || action === "versionSheet")
    }

    function actionAvailable(action) {
        if (overlayState && overlayState.statusActive
                && action !== "escape" && action !== "openFile")
            return false
        if (overlayState && overlayState.sheetOpen && action !== "escape")
            return false
        if ((action === "playPause" || action === "volumeUp"
                || action === "volumeDown" || action === "mute")
                && !root.playbackAvailable)
            return false
        if (action === "skipBackward" || action === "skipForward")
            return root.seekAvailable
        if (action === "subtitleSheet")
            return root.playbackAvailable && root.subtitleAvailable
        if (action === "audioSheet")
            return root.playbackAvailable && root.audioAvailable
        if (action === "episodeSheet")
            return root.playbackAvailable && root.episodeAvailable
        if (action === "versionSheet")
            return root.playbackAvailable && root.versionAvailable
        return true
    }

    function handle(event) {
        if (!event)
            return false

        var action = shortcutMap.actionForKey(event.key)
        if (action.length === 0)
            return false

        if (!actionAvailable(action))
            return false

        if (overlayState && revealsChrome(action))
            overlayState.showVisible()

        if (action === "playPause") {
            requestPlayPause()
        } else if (action === "skipBackward") {
            requestSkipBackward(skipStepMs)
        } else if (action === "skipForward") {
            requestSkipForward(skipStepMs)
        } else if (action === "volumeUp") {
            requestVolumeStep(volumeStep)
        } else if (action === "volumeDown") {
            requestVolumeStep(-volumeStep)
        } else if (action === "fullscreen") {
            requestFullscreenToggle()
        } else if (action === "openFile") {
            requestOpenFile()
        } else if (action === "mute") {
            requestMuteToggle()
        } else if (action === "subtitleSheet") {
            requestSubtitleSheet()
        } else if (action === "audioSheet") {
            requestAudioTrackSheet()
        } else if (action === "episodeSheet") {
            requestEpisodeSheet()
        } else if (action === "versionSheet") {
            requestVersionSheet()
        } else if (action === "escape") {
            if (overlayState && overlayState.sheetOpen)
                overlayState.closeSheet()
            else
                requestBack()
        }

        event.accepted = true
        return true
    }
}
