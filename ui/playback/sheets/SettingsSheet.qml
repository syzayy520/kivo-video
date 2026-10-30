import QtQuick 6.5

PlaybackSheet {
    id: root

    property real playbackSpeed: 1.0
    property real subtitleScale: 1.0
    property int subtitleDelayMs: 0
    property int audioDelayMs: 0
    property string aspectMode: "fit"
    property string toneMappingMode: "auto"
    property bool autoPlayNext: false
    property bool subtitleControlsAvailable: false
    property bool audioControlsAvailable: false
    property bool playbackSpeedControlsAvailable: false
    property bool autoPlayNextControlsAvailable: false

    signal playbackSpeedRequested(real speed)
    signal subtitleScaleRequested(real scale)
    signal subtitleDelayRequested(int delayMs)
    signal audioDelayRequested(int delayMs)
    signal aspectModeRequested(string mode)
    signal toneMappingModeRequested(string mode)
    signal autoPlayNextRequested(bool enabled)

    title: qsTr("Settings")

    SettingStepperRow {
        objectName: "speedRow"
        label: qsTr("Speed")
        visible: root.playbackSpeedControlsAvailable
        height: visible ? 42 : 0
        value: root.playbackSpeed
        minimumValue: 0.5
        maximumValue: 2.0
        stepSize: 0.25
        valueText: Number(root.playbackSpeed).toFixed(2) + "x"
        onValueRequested: function(value) { root.playbackSpeedRequested(value) }
    }

    SettingStepperRow {
        objectName: "subtitleTextSizeRow"
        label: qsTr("Text size")
        visible: root.subtitleControlsAvailable
        height: visible ? 42 : 0
        value: root.subtitleScale
        minimumValue: 0.75
        maximumValue: 1.60
        stepSize: 0.05
        valueText: Math.round(root.subtitleScale * 100) + "%"
        onValueRequested: function(value) { root.subtitleScaleRequested(value) }
    }

    SettingStepperRow {
        objectName: "subtitleSyncRow"
        label: qsTr("Subtitle sync")
        visible: root.subtitleControlsAvailable
        height: visible ? 42 : 0
        value: root.subtitleDelayMs
        minimumValue: -5000
        maximumValue: 5000
        stepSize: 250
        valueText: root.subtitleDelayMs + " ms"
        onValueRequested: function(value) { root.subtitleDelayRequested(Math.round(value)) }
    }

    SettingStepperRow {
        objectName: "audioSyncRow"
        label: qsTr("Audio sync")
        visible: root.audioControlsAvailable
        height: visible ? 42 : 0
        value: root.audioDelayMs
        minimumValue: -5000
        maximumValue: 5000
        stepSize: 250
        valueText: root.audioDelayMs + " ms"
        onValueRequested: function(value) { root.audioDelayRequested(Math.round(value)) }
    }

    SettingToggleRow {
        objectName: "continuePlayingRow"
        label: qsTr("Continue playing")
        visible: root.autoPlayNextControlsAvailable
        height: visible ? 42 : 0
        checked: root.autoPlayNext
        onCheckedRequested: function(checked) { root.autoPlayNextRequested(checked) }
    }

}
