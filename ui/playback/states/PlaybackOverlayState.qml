import QtQuick 6.5

QtObject {
    id: root

    enum Mode {
        Hidden,
        Visible,
        Interacting,
        Paused,
        TimelineHover,
        TimelineDragging,
        SheetOpen,
        Buffering,
        Recovering,
        Error
    }

    property int mode: PlaybackOverlayState.Hidden
    property bool paused: false
    property string activeSheet: ""
    property string statusKind: ""
    property int bufferingSeconds: 0
    property bool recoveredNotice: false

    readonly property bool sheetOpen: activeSheet.length > 0
    readonly property bool statusActive: statusKind.length > 0
    readonly property bool overlayVisible: mode !== PlaybackOverlayState.Hidden || paused || sheetOpen || statusActive
    readonly property bool shouldAutoHide: overlayVisible
                                           && !paused
                                           && !sheetOpen
                                           && !statusActive
                                           && mode !== PlaybackOverlayState.Interacting
                                           && mode !== PlaybackOverlayState.TimelineDragging

    signal autoHidePolicyChanged()

    onModeChanged: autoHidePolicyChanged()
    onPausedChanged: autoHidePolicyChanged()
    onActiveSheetChanged: autoHidePolicyChanged()
    onStatusKindChanged: autoHidePolicyChanged()
    onShouldAutoHideChanged: autoHidePolicyChanged()

    function showVisible() {
        if (sheetOpen || statusActive || paused)
            return
        mode = PlaybackOverlayState.Visible
    }

    function beginInteraction() {
        if (statusActive)
            return
        mode = PlaybackOverlayState.Interacting
    }

    function endInteraction() {
        if (statusActive || sheetOpen)
            return
        mode = paused ? PlaybackOverlayState.Paused : PlaybackOverlayState.Visible
    }

    function beginTimelineHover() {
        if (statusActive || sheetOpen)
            return
        mode = PlaybackOverlayState.TimelineHover
    }

    function beginTimelineDrag() {
        if (statusActive)
            return
        mode = PlaybackOverlayState.TimelineDragging
    }

    function endTimelineDrag() {
        if (statusActive || sheetOpen)
            return
        mode = paused ? PlaybackOverlayState.Paused : PlaybackOverlayState.Visible
    }

    function setPaused(value) {
        paused = value
        if (statusActive)
            clearStatus()
        mode = paused ? PlaybackOverlayState.Paused : PlaybackOverlayState.Visible
    }

    function openSheet(name) {
        if (statusKind === "error")
            return
        activeSheet = name
        mode = PlaybackOverlayState.SheetOpen
    }

    function closeSheet() {
        activeSheet = ""
        mode = paused ? PlaybackOverlayState.Paused : PlaybackOverlayState.Visible
    }

    function toggleSheet(name) {
        if (activeSheet === name)
            closeSheet()
        else
            openSheet(name)
    }

    function hideIfAllowed() {
        if (shouldAutoHide)
            mode = PlaybackOverlayState.Hidden
    }

    function setBuffering(seconds) {
        activeSheet = ""
        recoveredNotice = false
        bufferingSeconds = seconds
        statusKind = "buffering"
        mode = PlaybackOverlayState.Buffering
    }

    function setRecovering() {
        activeSheet = ""
        recoveredNotice = false
        statusKind = "recovering"
        mode = PlaybackOverlayState.Recovering
    }

    function setRecoveredNotice() {
        activeSheet = ""
        recoveredNotice = true
        statusKind = "recovered"
        mode = PlaybackOverlayState.Recovering
    }

    function setError() {
        activeSheet = ""
        recoveredNotice = false
        statusKind = "error"
        mode = PlaybackOverlayState.Error
    }

    function clearStatus() {
        statusKind = ""
        bufferingSeconds = 0
        recoveredNotice = false
        mode = paused ? PlaybackOverlayState.Paused : PlaybackOverlayState.Visible
    }
}
