import QtQuick 6.5

Item {
    id: root

    property var overlayState: null
    property bool autoHideEnabled: true

    visible: false

    function restartIfAllowed() {
        if (!autoHideEnabled || !overlayState) {
            autoHideTimer.stop()
            return
        }

        if (overlayState.shouldAutoHide)
            autoHideTimer.restart()
        else
            autoHideTimer.stop()
    }

    Timer {
        id: autoHideTimer

        interval: 3000
        repeat: false

        onTriggered: {
            if (root.overlayState)
                root.overlayState.hideIfAllowed()
        }
    }

    Connections {
        target: root.overlayState
        ignoreUnknownSignals: true

        function onAutoHidePolicyChanged() {
            root.restartIfAllowed()
        }
    }

    onAutoHideEnabledChanged: restartIfAllowed()

    Component.onCompleted: restartIfAllowed()
}
