import QtQuick 6.5

QtObject {
    id: root

    function actionForKey(key) {
        switch (key) {
        case Qt.Key_Space:
            return "playPause"
        case Qt.Key_Left:
            return "skipBackward"
        case Qt.Key_Right:
            return "skipForward"
        case Qt.Key_Up:
            return "volumeUp"
        case Qt.Key_Down:
            return "volumeDown"
        case Qt.Key_F:
            return "fullscreen"
        case Qt.Key_O:
            return "openFile"
        case Qt.Key_M:
            return "mute"
        case Qt.Key_S:
            return "subtitleSheet"
        case Qt.Key_A:
            return "audioSheet"
        case Qt.Key_C:
            return "episodeSheet"
        case Qt.Key_E:
            return "episodeSheet"
        case Qt.Key_V:
            return "versionSheet"
        case Qt.Key_Escape:
            return "escape"
        default:
            return ""
        }
    }
}
