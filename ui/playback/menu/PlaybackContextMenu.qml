import QtQuick 6.5
import QtQuick.Controls 6.5

Menu {
    id: root

    signal requestOpenFile()
    signal requestPlayPause()
    signal requestSkipBackward(int ms)
    signal requestSkipForward(int ms)
    signal requestSubtitleSheet()
    signal requestAudioTrackSheet()
    signal requestEpisodeSheet()
    signal requestVersionSheet()

    MenuItem {
        text: "打开视频"
        onTriggered: root.requestOpenFile()
    }

    MenuSeparator {}

    MenuItem {
        text: "播放 / 暂停"
        onTriggered: root.requestPlayPause()
    }

    MenuItem {
        text: "快退 10 秒"
        onTriggered: root.requestSkipBackward(10000)
    }

    MenuItem {
        text: "快进 10 秒"
        onTriggered: root.requestSkipForward(10000)
    }

    MenuSeparator {}

    MenuItem {
        text: "字幕"
        onTriggered: root.requestSubtitleSheet()
    }

    MenuItem {
        text: "音轨"
        onTriggered: root.requestAudioTrackSheet()
    }

    MenuItem {
        text: "选集"
        onTriggered: root.requestEpisodeSheet()
    }

    MenuItem {
        text: "版本"
        onTriggered: root.requestVersionSheet()
    }

}
