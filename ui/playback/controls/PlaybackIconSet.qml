import QtQuick 6.5

Canvas {
    id: root

    property string iconName: "play"
    property color strokeColor: Qt.rgba(1, 1, 1, 0.72)
    property real strokeWidth: 1.5

    implicitWidth: 18
    implicitHeight: 18

    onIconNameChanged: requestPaint()
    onStrokeColorChanged: requestPaint()
    onStrokeWidthChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)
        ctx.strokeStyle = strokeColor
        ctx.fillStyle = strokeColor
        ctx.lineWidth = strokeWidth
        ctx.lineCap = "round"
        ctx.lineJoin = "round"

        var w = width
        var h = height
        var cx = w / 2
        var cy = h / 2

        function line(x1, y1, x2, y2) {
            ctx.beginPath()
            ctx.moveTo(x1, y1)
            ctx.lineTo(x2, y2)
            ctx.stroke()
        }

        function rect(x, y, rw, rh) {
            ctx.beginPath()
            ctx.rect(x, y, rw, rh)
            ctx.stroke()
        }

        function dot(x, y, radius) {
            ctx.beginPath()
            ctx.arc(x, y, radius, 0, Math.PI * 2)
            ctx.fill()
        }

        function chevron(left, x, y, size) {
            ctx.beginPath()
            if (left) {
                ctx.moveTo(x + size, y)
                ctx.lineTo(x, y + size / 2)
                ctx.lineTo(x + size, y + size)
            } else {
                ctx.moveTo(x, y)
                ctx.lineTo(x + size, y + size / 2)
                ctx.lineTo(x, y + size)
            }
            ctx.stroke()
        }

        switch (iconName) {
        case "play":
            ctx.beginPath()
            ctx.moveTo(w * 0.36, h * 0.24)
            ctx.lineTo(w * 0.36, h * 0.76)
            ctx.lineTo(w * 0.76, h * 0.5)
            ctx.closePath()
            ctx.fill()
            break
        case "pause":
            ctx.beginPath()
            ctx.roundedRect(w * 0.35, h * 0.26, w * 0.10, h * 0.48, w * 0.03, w * 0.03)
            ctx.fill()
            ctx.beginPath()
            ctx.roundedRect(w * 0.55, h * 0.26, w * 0.10, h * 0.48, w * 0.03, w * 0.03)
            ctx.fill()
            break
        case "previous":
            line(w * 0.28, h * 0.28, w * 0.28, h * 0.72)
            chevron(true, w * 0.34, h * 0.31, h * 0.38)
            break
        case "next":
            line(w * 0.72, h * 0.28, w * 0.72, h * 0.72)
            chevron(false, w * 0.28, h * 0.31, h * 0.38)
            break
        case "rewind10":
            chevron(true, w * 0.22, h * 0.32, h * 0.36)
            line(w * 0.58, h * 0.34, w * 0.58, h * 0.66)
            ctx.font = "8px sans-serif"
            ctx.fillText("10", w * 0.50, h * 0.64)
            break
        case "forward10":
            chevron(false, w * 0.16, h * 0.32, h * 0.36)
            ctx.font = "8px sans-serif"
            ctx.fillText("10", w * 0.46, h * 0.64)
            break
        case "volume":
            ctx.beginPath()
            ctx.moveTo(w * 0.20, h * 0.42)
            ctx.lineTo(w * 0.34, h * 0.42)
            ctx.lineTo(w * 0.52, h * 0.28)
            ctx.lineTo(w * 0.52, h * 0.72)
            ctx.lineTo(w * 0.34, h * 0.58)
            ctx.lineTo(w * 0.20, h * 0.58)
            ctx.closePath()
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(w * 0.58, h * 0.50, w * 0.16, -0.65, 0.65)
            ctx.stroke()
            break
        case "muted":
            ctx.beginPath()
            ctx.moveTo(w * 0.20, h * 0.42)
            ctx.lineTo(w * 0.34, h * 0.42)
            ctx.lineTo(w * 0.52, h * 0.28)
            ctx.lineTo(w * 0.52, h * 0.72)
            ctx.lineTo(w * 0.34, h * 0.58)
            ctx.lineTo(w * 0.20, h * 0.58)
            ctx.closePath()
            ctx.stroke()
            line(w * 0.66, h * 0.36, w * 0.82, h * 0.64)
            line(w * 0.82, h * 0.36, w * 0.66, h * 0.64)
            break
        case "audio":
            line(w * 0.28, h * 0.70, w * 0.28, h * 0.30)
            line(w * 0.28, h * 0.30, w * 0.70, h * 0.24)
            line(w * 0.70, h * 0.24, w * 0.70, h * 0.62)
            dot(w * 0.24, h * 0.72, w * 0.08)
            dot(w * 0.66, h * 0.64, w * 0.08)
            break
        case "episodes":
            rect(w * 0.26, h * 0.28, w * 0.48, h * 0.34)
            line(w * 0.34, h * 0.72, w * 0.66, h * 0.72)
            line(w * 0.38, h * 0.80, w * 0.62, h * 0.80)
            break
        case "more":
            dot(w * 0.30, cy, w * 0.045)
            dot(cx, cy, w * 0.045)
            dot(w * 0.70, cy, w * 0.045)
            break
        case "close":
            line(w * 0.32, h * 0.32, w * 0.68, h * 0.68)
            line(w * 0.68, h * 0.32, w * 0.32, h * 0.68)
            break
        case "fullscreen":
            line(w * 0.25, h * 0.40, w * 0.25, h * 0.25)
            line(w * 0.25, h * 0.25, w * 0.40, h * 0.25)
            line(w * 0.75, h * 0.40, w * 0.75, h * 0.25)
            line(w * 0.75, h * 0.25, w * 0.60, h * 0.25)
            line(w * 0.25, h * 0.60, w * 0.25, h * 0.75)
            line(w * 0.25, h * 0.75, w * 0.40, h * 0.75)
            line(w * 0.75, h * 0.60, w * 0.75, h * 0.75)
            line(w * 0.75, h * 0.75, w * 0.60, h * 0.75)
            break
        case "cast":
            rect(w * 0.22, h * 0.28, w * 0.56, h * 0.38)
            ctx.beginPath()
            ctx.arc(w * 0.22, h * 0.78, w * 0.08, 0, Math.PI * 1.5)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(w * 0.22, h * 0.78, w * 0.20, 0, Math.PI * 1.5)
            ctx.stroke()
            break
        case "pip":
            rect(w * 0.22, h * 0.26, w * 0.56, h * 0.48)
            rect(w * 0.50, h * 0.48, w * 0.20, h * 0.16)
            break
        case "screenshot":
            rect(w * 0.24, h * 0.34, w * 0.52, h * 0.36)
            rect(w * 0.38, h * 0.26, w * 0.24, h * 0.10)
            ctx.beginPath()
            ctx.arc(cx, h * 0.52, w * 0.11, 0, Math.PI * 2)
            ctx.stroke()
            break
        case "loop":
            ctx.beginPath()
            ctx.arc(cx, cy, w * 0.28, 0.2, Math.PI * 1.55)
            ctx.stroke()
            chevron(false, w * 0.56, h * 0.20, w * 0.16)
            break
        case "ratio":
            rect(w * 0.22, h * 0.32, w * 0.56, h * 0.36)
            line(w * 0.34, h * 0.32, w * 0.34, h * 0.68)
            line(w * 0.66, h * 0.32, w * 0.66, h * 0.68)
            break
        case "speed":
            ctx.beginPath()
            ctx.arc(cx, cy, w * 0.30, Math.PI, 0)
            ctx.stroke()
            line(cx, cy, w * 0.68, h * 0.38)
            break
        case "info":
            ctx.beginPath()
            ctx.arc(cx, cy, w * 0.30, 0, Math.PI * 2)
            ctx.stroke()
            line(cx, h * 0.46, cx, h * 0.66)
            dot(cx, h * 0.34, w * 0.035)
            break
        case "report":
            ctx.beginPath()
            ctx.arc(cx, cy, w * 0.30, 0, Math.PI * 2)
            ctx.stroke()
            line(cx, h * 0.32, cx, h * 0.56)
            dot(cx, h * 0.68, w * 0.035)
            break
        default:
            line(w * 0.28, cy, w * 0.72, cy)
            break
        }
    }
}
