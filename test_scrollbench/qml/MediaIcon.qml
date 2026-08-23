import QtQuick

Item {
    id: root
    implicitWidth: 24
    implicitHeight: 24

    // iconType: "play", "pause", "speaker", "mute", "rotate"
    property string iconType: "play"
    property color color: "#ffffff"

    Canvas {
        id: canvas
        anchors.fill: parent
        renderTarget: Canvas.Image

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.fillStyle = root.color
            ctx.strokeStyle = root.color
            ctx.lineWidth = 2
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            var type = root.iconType

            if (type === "play") {
                // Solid white play triangle (matching Image 2 style)
                ctx.beginPath()
                ctx.moveTo(7, 5)
                ctx.lineTo(19, 12)
                ctx.lineTo(7, 19)
                ctx.closePath()
                ctx.fill()
            } else if (type === "pause") {
                // Solid white pause bars (matching Image 2 style)
                ctx.beginPath()
                ctx.fillRect(6, 5, 4, 14)
                ctx.fillRect(14, 5, 4, 14)
            } else if (type === "speaker") {
                // Solid white speaker body + sound wave arcs (matching Image 2 style)
                ctx.beginPath()
                ctx.moveTo(3, 9)
                ctx.lineTo(7, 9)
                ctx.lineTo(12, 5)
                ctx.lineTo(12, 19)
                ctx.lineTo(7, 15)
                ctx.lineTo(3, 15)
                ctx.closePath()
                ctx.fill()

                // Sound wave arc
                ctx.beginPath()
                ctx.arc(12, 12, 5, -Math.PI/3, Math.PI/3)
                ctx.stroke()
            } else if (type === "mute") {
                // Solid white speaker body + 'x' mark (matching Image 3 style)
                ctx.beginPath()
                ctx.moveTo(3, 9)
                ctx.lineTo(7, 9)
                ctx.lineTo(12, 5)
                ctx.lineTo(12, 19)
                ctx.lineTo(7, 15)
                ctx.lineTo(3, 15)
                ctx.closePath()
                ctx.fill()

                // X mark
                ctx.lineWidth = 2.5
                ctx.beginPath()
                ctx.moveTo(15, 9)
                ctx.lineTo(21, 15)
                ctx.moveTo(21, 9)
                ctx.lineTo(15, 15)
                ctx.stroke()
            } else if (type === "rotate") {
                // Solid white rotate arrow icon
                ctx.lineWidth = 2
                ctx.beginPath()
                ctx.arc(12, 12, 6, -Math.PI*0.8, Math.PI*0.6)
                ctx.stroke()

                // Arrow head
                ctx.beginPath()
                ctx.moveTo(16, 5)
                ctx.lineTo(19, 9)
                ctx.lineTo(14, 10)
                ctx.closePath()
                ctx.fill()
            }
        }

        Connections {
            target: root
            function onIconTypeChanged() { canvas.requestPaint() }
            function onColorChanged() { canvas.requestPaint() }
        }
    }
}
