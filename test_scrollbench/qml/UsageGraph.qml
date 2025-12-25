import QtQuick 2.15
import QtQuick.Shapes 1.15

Item {
    id: root
    width: 200
    height: 60
    
    property var dataPoints: []
    property string label: "Usage"
    property string color: "#00FF00"
    property string suffix: "%"
    property real maxValue: 100
    
    // Background
    Rectangle {
        anchors.fill: parent
        color: "#22000000"
        border.color: "#444"
        border.width: 1
    }
    
    // Graph
    Shape {
        anchors.fill: parent
        anchors.margins: 1
        
        ShapePath {
            strokeWidth: 1
            strokeColor: root.color
            fillColor: Qt.rgba(root.color.r, root.color.g, root.color.b, 0.3)
            startX: 0
            startY: root.height
            
            PathLine { x: 0; y: root.height } // Start bottom-left
            
            // Dynamic points
            // We can't use Repeater inside ShapePath easily in Qt5/6 without Instantiator or manual path construction.
            // So we'll use a Canvas instead for better performance and flexibility.
        }
    }
    
    Canvas {
        id: canvas
        anchors.fill: parent
        anchors.margins: 1
        
        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            
            if (!root.dataPoints || root.dataPoints.length < 2) return
            
            ctx.beginPath()
            ctx.lineWidth = 1
            ctx.strokeStyle = root.color
            ctx.fillStyle = Qt.rgba(
                parseInt(root.color.substring(1, 3), 16) / 255,
                parseInt(root.color.substring(3, 5), 16) / 255,
                parseInt(root.color.substring(5, 7), 16) / 255,
                0.3
            )
            
            var stepX = width / (root.dataPoints.length - 1)
            
            ctx.moveTo(0, height) // Bottom-left
            
            for (var i = 0; i < root.dataPoints.length; i++) {
                var val = root.dataPoints[i]
                var normalized = val / root.maxValue
                var y = height - (normalized * height)
                var x = i * stepX
                ctx.lineTo(x, y)
            }
            
            ctx.lineTo(width, height) // Bottom-right
            ctx.lineTo(0, height) // Close loop
            ctx.closePath()
            ctx.fill()
            ctx.stroke()
        }
    }
    
    // Update trigger
    onDataPointsChanged: canvas.requestPaint()
    
    // Label
    Text {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 4
        text: root.label
        color: "#aaa"
        font.pixelSize: 10
    }
    
    // Current Value
    Text {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 4
        text: (root.dataPoints && root.dataPoints.length > 0 ? root.dataPoints[root.dataPoints.length - 1].toFixed(1) : "0") + root.suffix
        color: "white"
        font.bold: true
        font.pixelSize: 10
    }
    
    // Mouse Hover
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        
        onPositionChanged: (mouse) => {
            if (!root.dataPoints || root.dataPoints.length < 2) return
            var index = Math.round((mouse.x / width) * (root.dataPoints.length - 1))
            if (index >= 0 && index < root.dataPoints.length) {
                var val = root.dataPoints[index]
                tooltipText.text = val.toFixed(1) + root.suffix
                tooltip.x = mouse.x + 10
                tooltip.y = mouse.y - 20
                tooltip.visible = true
            }
        }
        onExited: tooltip.visible = false
    }
    
    Rectangle {
        id: tooltip
        width: tooltipText.width + 8
        height: tooltipText.height + 4
        color: "#333"
        border.color: "#888"
        visible: false
        z: 10
        
        Text {
            id: tooltipText
            anchors.centerIn: parent
            color: "white"
            font.pixelSize: 10
        }
    }
}
