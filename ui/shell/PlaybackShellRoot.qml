import QtQuick 6.5
import QtQuick.Controls 6.5

ApplicationWindow {
    id: shellWindow

    title: "Kivo Video Playback"
    width: 1280
    height: 720
    visible: true
    color: "#000000"

    Loader {
        id: playbackPageLoader
        anchors.fill: parent
        source: playbackPageUrl
        onLoaded: {
            if (!item)
                return
            item.runtimeAdapter = shellRuntimeAdapter
        }
    }
}