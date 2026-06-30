import QtQuick 6.5

QtObject {
    id: root

    readonly property var shortcutNames: ({
        "togglePlayPause": "togglePlayPause",
        "playPause": "togglePlayPause",
        "subtitle": "subtitle",
        "audioTrack": "audioTrack",
        "rewind10": "seekBackward",
        "forward10": "seekForward",
        "volumeUp": "volumeUp",
        "volumeDown": "volumeDown",
        "mute": "mute",
        "setMuted": "mute",
        "enterFullscreen": "fullscreen",
        "exitFullscreen": "exitFullscreen",
        "fullscreen": "fullscreen"
    })

    function resolveShortcutName(rawName) {
        if (!rawName)
            return ""
        return shortcutNames[rawName] || rawName
    }

    function invoke(adapter, methodName, args) {
        if (!adapter || typeof adapter[methodName] !== "function")
            return false
        adapter[methodName].apply(adapter, args || [])
        return true
    }

    function dispatchCommand(adapter, methodName, args, actionName) {
        return {
            ok: invoke(adapter, methodName, args),
            actionName: actionName || methodName
        }
    }

    function openMediaId(adapter, mediaId, actionName) {
        if (invoke(adapter, "openMediaId", [mediaId]))
            return { ok: true, actionName: actionName || "openMediaId" }
        return dispatchCommand(adapter, "open", [{ sourceId: mediaId }], actionName || "open")
    }

    function dispatchShortcut(adapter, rawName) {
        var resolved = resolveShortcutName(rawName)
        if (invoke(adapter, "handleShortcutName", [resolved]))
            return { ok: true, actionName: resolved }
        return dispatchCommand(adapter, "handleShortcut", [resolved], resolved)
    }

    function pullSnapshot(adapter) {
        if (!adapter)
            return null
        if (typeof adapter.pullSnapshot === "function")
            return adapter.pullSnapshot()
        if (typeof adapter.snapshot === "function")
            return adapter.snapshot()
        return null
    }

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
}