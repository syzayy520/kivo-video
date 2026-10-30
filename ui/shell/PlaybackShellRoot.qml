import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Dialogs 6.5

ApplicationWindow {
    id: shellWindow

    title: qsTr("Kivo Video Playback")
    width: 1280
    height: 720
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "#000000"

    function manualOpenController() {
        if (typeof shellManualMediaOpenController === "undefined")
            return null
        return shellManualMediaOpenController
    }

    function requestManualOpen() {
        openVideoDialog.open()
    }

    function requestManualFolderOpen() {
        openFolderDialog.open()
    }

    function openSelectedMedia(fileUrl) {
        var controller = manualOpenController()
        if (!controller || !playbackPageLoader.item)
            return false
        return controller.openLocalMediaUrl(playbackPageLoader.item, fileUrl)
    }

    function openDroppedMedia(drop) {
        if (!drop || !drop.hasUrls || drop.urls.length === 0)
            return false
        drop.acceptProposedAction()
        return openSelectedMedia(drop.urls[0])
    }

    Loader {
        id: playbackPageLoader
        anchors.fill: parent
        source: playbackPageUrl
        onLoaded: {
            if (!item)
                return
            item.runtimeAdapter = shellRuntimeAdapter
            item.backRequested.connect(function() {
                shellRuntimeAdapter.close()
                shellWindow.close()
            })
            item.requestOpenFile.connect(function() {
                shellWindow.requestManualOpen()
            })
            item.requestOpenFolder.connect(function() {
                shellWindow.requestManualFolderOpen()
            })
        }
    }

    DropArea {
        anchors.fill: parent

        onDropped: function(drop) {
            shellWindow.openDroppedMedia(drop)
        }
    }

    FileDialog {
        id: openVideoDialog

        title: qsTr("Open Video")
        fileMode: FileDialog.OpenFile
        nameFilters: [
            qsTr("Video files (*.mkv *.mp4 *.m4v *.mov *.avi *.ts *.m2ts *.webm)"),
            qsTr("All files (*)")
        ]

        onAccepted: shellWindow.openSelectedMedia(selectedFile)
    }

    FolderDialog {
        id: openFolderDialog

        title: qsTr("Open Video Folder")

        onAccepted: shellWindow.openSelectedMedia(selectedFolder)
    }
}
