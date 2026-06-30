import QtQuick 6.5

Item {
    id: root

    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: "#050608"

        gradient: Gradient {
            GradientStop { position: 0.00; color: "#07080b" }
            GradientStop { position: 0.48; color: "#10131a" }
            GradientStop { position: 1.00; color: "#020203" }
        }
    }

    Rectangle {
        anchors.fill: parent
        opacity: 0.38

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.00; color: Qt.rgba(0.02, 0.03, 0.04, 0.70) }
            GradientStop { position: 0.34; color: Qt.rgba(0.14, 0.14, 0.13, 0.30) }
            GradientStop { position: 0.72; color: Qt.rgba(0.03, 0.08, 0.10, 0.36) }
            GradientStop { position: 1.00; color: Qt.rgba(0.00, 0.00, 0.00, 0.82) }
        }
    }
}
