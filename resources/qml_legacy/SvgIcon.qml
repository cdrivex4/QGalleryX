import QtQuick

// SvgIcon - renders a named icon as a vector shape using Canvas
// Usage: SvgIcon { iconName: "folder"; size: 18; color: "white" }
Item {
    id: root
    property string iconName: "folder"
    property real size: 18
    property color color: "#ffffff"

    width: size
    height: size

    onIconNameChanged: canvas.requestPaint()
    onColorChanged:    canvas.requestPaint()
    onWidthChanged:    canvas.requestPaint()
    onHeightChanged:   canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var s = root.size
            var sc = s / 24.0  // icons designed at 24x24 grid

            ctx.save()
            ctx.scale(sc, sc)
            ctx.fillStyle   = root.color
            ctx.strokeStyle = root.color

            if (root.iconName === "folder") {
                // Solid folder shape
                ctx.beginPath()
                ctx.moveTo(10, 4)
                ctx.lineTo(4,  4)
                ctx.quadraticCurveTo(2, 4, 2, 6)
                ctx.lineTo(2,  18)
                ctx.quadraticCurveTo(2, 20, 4, 20)
                ctx.lineTo(20, 20)
                ctx.quadraticCurveTo(22, 20, 22, 18)
                ctx.lineTo(22, 8)
                ctx.quadraticCurveTo(22, 6, 20, 6)
                ctx.lineTo(12, 6)
                ctx.lineTo(10, 4)
                ctx.closePath()
                ctx.fill()

            } else if (root.iconName === "folder_network") {
                // Folder + small wifi arc on face to indicate network
                ctx.beginPath()
                ctx.moveTo(10, 4)
                ctx.lineTo(4,  4)
                ctx.quadraticCurveTo(2, 4, 2, 6)
                ctx.lineTo(2,  18)
                ctx.quadraticCurveTo(2, 20, 4, 20)
                ctx.lineTo(20, 20)
                ctx.quadraticCurveTo(22, 20, 22, 18)
                ctx.lineTo(22, 8)
                ctx.quadraticCurveTo(22, 6, 20, 6)
                ctx.lineTo(12, 6)
                ctx.lineTo(10, 4)
                ctx.closePath()
                ctx.fill()
                // Overlay wifi arc
                ctx.strokeStyle = "rgba(0,0,0,0.55)"
                ctx.lineWidth = 1.8
                ctx.lineCap = "round"
                ctx.beginPath()
                ctx.arc(12, 14, 4, Math.PI, 0)
                ctx.stroke()
                ctx.beginPath()
                ctx.arc(12, 14, 2, Math.PI, 0)
                ctx.stroke()
                ctx.fillStyle = "rgba(0,0,0,0.55)"
                ctx.beginPath()
                ctx.arc(12, 14, 0.9, 0, Math.PI * 2)
                ctx.fill()

            } else if (root.iconName === "menu") {
                // 3-line hamburger
                ctx.fillRect(3,  5, 18, 2.5)
                ctx.fillRect(3, 11, 18, 2.5)
                ctx.fillRect(3, 17, 18, 2.5)

            } else if (root.iconName === "snail") {
                // simple snail profile
                ctx.beginPath();
                ctx.arc(11, 13, 5, 0, Math.PI * 2);
                ctx.fill();
                ctx.beginPath();
                ctx.moveTo(6, 18);
                ctx.lineTo(19, 18);
                ctx.lineTo(21, 14);
                ctx.lineWidth = 3;
                ctx.lineCap = "round";
                ctx.stroke();

                // antennae
                ctx.beginPath();
                ctx.moveTo(19, 14);
                ctx.lineTo(17, 9);
                ctx.moveTo(21, 14);
                ctx.lineTo(23, 9);
                ctx.lineWidth = 1.5;
                ctx.stroke();

            } else if (root.iconName === "battery") {
                // Battery icon
                ctx.lineWidth = 2;
                ctx.strokeRect(5, 7, 12, 10);
                ctx.fillRect(17, 10, 2, 4);
                ctx.fillRect(7, 9, 3, 6);
                ctx.fillRect(11, 9, 3, 6);
                
            } else if (root.iconName === "rocket") {
                // Rocket icon
                ctx.beginPath();
                ctx.moveTo(12, 4);
                ctx.lineTo(16, 10);
                ctx.lineTo(16, 16);
                ctx.lineTo(20, 20);
                ctx.lineTo(4, 20);
                ctx.lineTo(8, 16);
                ctx.lineTo(8, 10);
                ctx.closePath();
                ctx.fill();
                
                // Flames
                ctx.beginPath();
                ctx.moveTo(10, 20);
                ctx.lineTo(12, 23);
                ctx.lineTo(14, 20);
                ctx.fill();

            } else if (root.iconName === "close") {
                // X
                ctx.lineWidth = 2.5
                ctx.lineCap = "round"
                ctx.beginPath()
                ctx.moveTo(5,  5)
                ctx.lineTo(19, 19)
                ctx.stroke()
                ctx.beginPath()
                ctx.moveTo(19, 5)
                ctx.lineTo(5,  19)
                ctx.stroke()

            } else if (root.iconName === "delete") {
                // Trash can
                ctx.fillRect(9, 2, 6, 2)       // handle
                ctx.fillRect(3, 4, 18, 2)       // lid
                ctx.beginPath()
                ctx.moveTo(5, 7)
                ctx.lineTo(6, 21)
                ctx.quadraticCurveTo(6, 22, 7, 22)
                ctx.lineTo(17, 22)
                ctx.quadraticCurveTo(18, 22, 18, 21)
                ctx.lineTo(19, 7)
                ctx.closePath()
                ctx.fill()

            } else if (root.iconName === "scan") {
                // Magnifier
                ctx.lineWidth = 2.5
                ctx.lineCap = "round"
                ctx.beginPath()
                ctx.arc(10, 10, 6, 0, Math.PI * 2)
                ctx.stroke()
                ctx.beginPath()
                ctx.moveTo(14.5, 14.5)
                ctx.lineTo(21,   21)
                ctx.stroke()

            } else if (root.iconName === "settings") {
                // Gear
                ctx.beginPath()
                ctx.arc(12, 12, 3, 0, Math.PI * 2)
                ctx.fill()
                ctx.lineWidth = 3
                ctx.lineCap = "butt"
                for (var i = 0; i < 8; i++) {
                    var a = (i / 8) * Math.PI * 2
                    ctx.beginPath()
                    ctx.moveTo(12 + Math.cos(a) * 5, 12 + Math.sin(a) * 5)
                    ctx.lineTo(12 + Math.cos(a) * 9, 12 + Math.sin(a) * 9)
                    ctx.stroke()
                }

            } else if (root.iconName === "pictures") {
                // Image frame + mountain + sun
                // Manual rounded rect (Qt Canvas does not support roundRect)
                var rx = 2, ry = 3, rw = 20, rh = 18, rr = 2
                ctx.beginPath()
                ctx.moveTo(rx + rr, ry)
                ctx.lineTo(rx + rw - rr, ry)
                ctx.quadraticCurveTo(rx + rw, ry, rx + rw, ry + rr)
                ctx.lineTo(rx + rw, ry + rh - rr)
                ctx.quadraticCurveTo(rx + rw, ry + rh, rx + rw - rr, ry + rh)
                ctx.lineTo(rx + rr, ry + rh)
                ctx.quadraticCurveTo(rx, ry + rh, rx, ry + rh - rr)
                ctx.lineTo(rx, ry + rr)
                ctx.quadraticCurveTo(rx, ry, rx + rr, ry)
                ctx.closePath()
                ctx.fill()
                // Mountain
                ctx.fillStyle = Qt.darker(root.color, 1.6)
                ctx.beginPath()
                ctx.moveTo(2,  19)
                ctx.lineTo(8,  11)
                ctx.lineTo(13, 16)
                ctx.lineTo(16, 13)
                ctx.lineTo(22, 19)
                ctx.closePath()
                ctx.fill()
                // Sun dot
                ctx.beginPath()
                ctx.arc(17, 8, 2.5, 0, Math.PI * 2)
                ctx.fill()

            } else if (root.iconName === "albums") {
                // Stacked cards
                ctx.globalAlpha = 0.5
                ctx.fillRect(4, 6, 16, 14)
                ctx.globalAlpha = 0.75
                ctx.fillRect(6, 4, 16, 14)
                ctx.globalAlpha = 1.0
                ctx.fillRect(8, 2, 16, 14)

            } else if (root.iconName === "rebuild") {
                // Circular arrow
                ctx.lineWidth = 2.5
                ctx.lineCap = "round"
                ctx.beginPath()
                ctx.arc(12, 12, 8, -Math.PI * 0.75, Math.PI * 0.75)
                ctx.stroke()
                // Arrow head
                ctx.beginPath()
                ctx.moveTo(19, 6.5)
                ctx.lineTo(21, 9.5)
                ctx.lineTo(17.5, 10.5)
                ctx.stroke()
            }

            ctx.restore()
        }
    }
}
