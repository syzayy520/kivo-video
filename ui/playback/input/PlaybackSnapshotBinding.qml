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
            isOpening: raw.isOpening !== undefined ? raw.isOpening : !!raw.is_opening,
            bufferReason: raw.bufferReason || raw.buffer_reason || "none",
            rebufferCount: raw.rebufferCount !== undefined ? raw.rebufferCount
                           : (raw.rebuffer_count || 0),
            playbackState: raw.playbackState || raw.playback_state || "",
            isRecovering: raw.isRecovering !== undefined ? raw.isRecovering : !!raw.is_recovering,
            hasError: raw.hasError !== undefined ? raw.hasError : (raw.playbackState === "Failed"),
            closed: !!raw.closed,
            fullscreenLike: raw.fullscreenLike !== undefined ? raw.fullscreenLike : !!raw.fullscreen_like,
            autoPlayNext: raw.autoPlayNext !== undefined ? raw.autoPlayNext : !!raw.auto_play_next,
            playlistQueueConnected: raw.playlistQueueConnected !== undefined
                                    ? raw.playlistQueueConnected
                                    : !!raw.playlist_queue_connected,
            playlistHasPrevious: raw.playlistHasPrevious !== undefined
                                 ? raw.playlistHasPrevious
                                 : !!raw.playlist_has_previous,
            playlistHasNext: raw.playlistHasNext !== undefined
                             ? raw.playlistHasNext
                             : !!raw.playlist_has_next,
            playlistHasNextEpisode: raw.playlistHasNextEpisode !== undefined
                                    ? raw.playlistHasNextEpisode
                                    : !!raw.playlist_has_next_episode,
            episodeOptions: raw.episodeOptions !== undefined
                            ? raw.episodeOptions
                            : (raw.episode_options || []),
            currentEpisode: raw.currentEpisode || raw.current_episode || "",
            versionOptions: raw.versionOptions !== undefined
                            ? raw.versionOptions
                            : (raw.version_options || []),
            versionTechnicalRows: raw.versionTechnicalRows !== undefined
                                  ? raw.versionTechnicalRows
                                  : (raw.version_technical_rows || []),
            volume: raw.volume !== undefined ? raw.volume : 1.0,
            muted: raw.muted !== undefined ? raw.muted : false,
            subtitlePrimary: raw.subtitlePrimary || raw.subtitle_primary || "",
            subtitleSecondary: raw.subtitleSecondary || raw.subtitle_secondary || "",
            subtitleEnabled: raw.subtitleEnabled !== undefined ? raw.subtitleEnabled
                              : (raw.subtitle_enabled !== undefined ? raw.subtitle_enabled : false),
            selectedSubtitleTrack: raw.selectedSubtitleTrack !== undefined
                                   ? raw.selectedSubtitleTrack
                                   : (raw.selected_subtitle_track || ""),
            subtitleOptions: raw.subtitleOptions !== undefined
                             ? raw.subtitleOptions
                             : (raw.subtitle_options || []),
            subtitleDelayMs: raw.subtitleDelayMs !== undefined ? raw.subtitleDelayMs
                             : (raw.subtitle_delay_ms || 0),
            selectedAudioTrack: raw.selectedAudioTrack !== undefined
                                ? raw.selectedAudioTrack
                                : (raw.selected_audio_track || ""),
            audioTrackOptions: raw.audioTrackOptions !== undefined
                               ? raw.audioTrackOptions
                               : (raw.audio_track_options || []),
            audioDelayMs: raw.audioDelayMs !== undefined ? raw.audioDelayMs : (raw.audio_delay_ms || 0),
            subtitleSize: raw.subtitleSize !== undefined ? raw.subtitleSize
                          : (raw.subtitle_size !== undefined ? raw.subtitle_size : 1.0),
            playbackSpeed: raw.playbackSpeed !== undefined ? raw.playbackSpeed
                           : (raw.playback_speed !== undefined ? raw.playback_speed : 1.0),
            aspectMode: raw.aspectMode || raw.aspect_mode || "fit",
            toneMappingMode: raw.toneMappingMode || raw.tone_mapping_mode || "auto"
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
        root.target.isOpening = snap.isOpening
        root.target.bufferReason = snap.bufferReason
        root.target.rebufferCount = snap.rebufferCount
        root.target.playbackState = snap.playbackState
        root.target.isRecovering = snap.isRecovering
        root.target.hasError = snap.hasError
        root.target.fullscreenLike = snap.fullscreenLike
        root.target.autoPlayNext = snap.autoPlayNext
        root.target.playlistQueueConnected = snap.playlistQueueConnected
        root.target.playlistHasPrevious = snap.playlistHasPrevious
        root.target.playlistHasNext = snap.playlistHasNext
        root.target.playlistHasNextEpisode = snap.playlistHasNextEpisode
        root.target.episodeOptions = snap.episodeOptions
        root.target.currentEpisode = snap.episodeOptions.length > 0 ? snap.currentEpisode : ""
        root.target.versionOptions = snap.versionOptions
        root.target.versionTechnicalRows = snap.versionOptions.length > 0 ? snap.versionTechnicalRows : []
        root.target.volume = snap.volume
        root.target.isMuted = snap.muted
        root.target.subtitlePrimary = snap.subtitlePrimary
        root.target.subtitleSecondary = snap.subtitleSecondary
        root.target.subtitleEnabled = snap.subtitleEnabled
        root.target.selectedSubtitle = snap.selectedSubtitleTrack
        root.target.subtitleOptions = snap.subtitleOptions
        root.target.subtitleDelayMs = snap.subtitleDelayMs
        root.target.audioTrackLabel = snap.selectedAudioTrack
        root.target.audioTrackOptions = snap.audioTrackOptions
        root.target.audioDelayMs = snap.audioDelayMs
        root.target.subtitleSize = snap.subtitleSize
        root.target.playbackSpeed = snap.playbackSpeed
        root.target.aspectMode = snap.aspectMode
        root.target.toneMappingMode = snap.toneMappingMode
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
