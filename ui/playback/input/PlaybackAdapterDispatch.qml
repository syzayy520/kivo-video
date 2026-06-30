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

    function openMediaId(adapter, mediaId, actionName, snapshotBinding) {
        var result
        if (invoke(adapter, "openMediaId", [mediaId]))
            result = { ok: true, actionName: actionName || "openMediaId" }
        else
            result = dispatchCommand(adapter, "open", [{ sourceId: mediaId }], actionName || "open")

        if (result.ok && snapshotBinding)
            snapshotBinding.scheduleRefresh()
        return result
    }

    function dispatchShortcut(adapter, rawName, snapshotBinding) {
        var resolved = resolveShortcutName(rawName)
        var result
        if (invoke(adapter, "handleShortcutName", [resolved]))
            result = { ok: true, actionName: resolved }
        else
            result = dispatchCommand(adapter, "handleShortcut", [resolved], resolved)

        if (result.ok && snapshotBinding)
            snapshotBinding.scheduleRefresh()
        return result
    }

    function dispatchRuntimeCommand(adapter, methodName, args, actionName, snapshotBinding) {
        var result = dispatchCommand(adapter, methodName, args, actionName)
        if (result.ok && snapshotBinding)
            snapshotBinding.scheduleRefresh()
        return result
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
}