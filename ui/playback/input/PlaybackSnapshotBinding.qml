import QtQuick 6.5

Item {
    id: root

    visible: false
    width: 0
    height: 0

    property var runtimeAdapter: null
    property var target: null
    property var dispatch: null
    property bool skipRefresh: false

    function normalizeSnapshot(raw) {
        if (!raw)
            return null

        var buffered = 0
        if (raw.bufferedRanges && raw.bufferedRanges.length > 0) {
            var range = raw.bufferedRanges[0]
            var endMs = range.endMs !== undefined ? range.endMs : range.end_ms
            var startMs = range.startMs !== undefined ? range.startMs : range.start_ms
            var duration = raw.durationMs !== undefined ? raw.durationMs : raw.duration_ms
            if (duration > 0)
                buffered = Math.max(0, Math.min(1, (endMs - startMs) / duration))
        } else if (raw.buffered !== undefined) {
            buffered = raw.buffered
        }

        return {
            positionMs: raw.positionMs !== undefined ? raw.positionMs : (raw.position_ms || 0),
            durationMs: raw.durationMs !== undefined ? raw.durationMs : (raw.duration_ms || 0),
            buffered: buffered,
            isPlaying: raw.isPlaying !== undefined ? raw.isPlaying : !!raw.is_playing,
            isPaused: raw.isPaused !== undefined ? raw.isPaused : !!raw.is_paused,
            isBuffering: raw.isBuffering !== undefined ? raw.isBuffering : !!raw.is_buffering,
            isRecovering: raw.isRecovering !== undefined ? raw.isRecovering : !!raw.is_recovering,
            hasError: raw.hasError !== undefined ? raw.hasError : (raw.playbackState === "Failed"),
            closed: !!raw.closed,
            volume: raw.volume !== undefined ? raw.volume : 1.0,
            muted: raw.muted !== undefined ? raw.muted : false,
            subtitlePrimary: raw.subtitlePrimary || raw.subtitle_primary || "",
            subtitleSecondary: raw.subtitleSecondary || raw.subtitle_secondary || "",
            subtitleEnabled: raw.subtitleEnabled !== undefined ? raw.subtitleEnabled
                              : (raw.subtitle_enabled !== undefined ? raw.subtitle_enabled : false),
            subtitleDelayMs: raw.subtitleDelayMs !== undefined ? raw.subtitleDelayMs
                             : (raw.subtitle_delay_ms || 0),
            audioDelayMs: raw.audioDelayMs !== undefined ? raw.audioDelayMs : (raw.audio_delay_ms || 0)
        }
    }

    function refresh() {
        if (!root.runtimeAdapter || root.skipRefresh || !root.target || !root.dispatch)
            return

        var raw = root.dispatch.pullSnapshot(root.runtimeAdapter)
        var snap = normalizeSnapshot(raw)
        if (!snap || snap.closed)
            return

        root.target.currentTimeMs = snap.positionMs
        root.target.durationMs = snap.durationMs
        root.target.buffered = snap.buffered
        root.target.isPaused = snap.isPaused
        root.target.isBuffering = snap.isBuffering
        root.target.isRecovering = snap.isRecovering
        root.target.hasError = snap.hasError
        root.target.volume = snap.volume
        root.target.isMuted = snap.muted
        root.target.subtitlePrimary = snap.subtitlePrimary
        root.target.subtitleSecondary = snap.subtitleSecondary
        root.target.subtitleEnabled = snap.subtitleEnabled
        root.target.subtitleDelayMs = snap.subtitleDelayMs
    }

    function scheduleRefresh() {
        Qt.callLater(refresh)
    }

    Timer {
        interval: 250
        running: root.runtimeAdapter !== null
        repeat: true
        onTriggered: root.refresh()
    }

    Connections {
        target: root.runtimeAdapter
        ignoreUnknownSignals: true
        function onSnapshotChanged() {
            root.scheduleRefresh()
        }
    }

    onRuntimeAdapterChanged: {
        if (runtimeAdapter)
            scheduleRefresh()
    }

    onSkipRefreshChanged: {
        if (!skipRefresh)
            scheduleRefresh()
    }
}