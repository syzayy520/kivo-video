import QtQuick 6.5

Item {
    id: root

    property var overlayState: null
    property int skipStepMs: 10000
    property real volumeStep: 0.05

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

    function handle(event) {
        if (!event)
            return false

        var action = shortcutMap.actionForKey(event.key)
        if (action.length === 0)
            return false

        if (overlayState)
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
