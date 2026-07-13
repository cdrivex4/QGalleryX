import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 200 // Width to contain bubble and markers
    
    property var listView
    property var proxyModel
    
    // Year Distribution Data
    property var yearData: []
    
    function updateYearData() {
        if (proxyModel) {
            yearData = proxyModel.getYearDistribution()
        }
    }
    
    Connections {
        target: proxyModel
        function onModelReset() { updateYearData() }
        function onLayoutChanged() { updateYearData() }
    }
    
    Component.onCompleted: updateYearData()
    
    // Year Markers (Timeline)
    Repeater {
        model: yearData
        delegate: Rectangle {
            width: 50
            height: 24
            radius: 12
            color: "#333"
            opacity: 0.8
            anchors.right: parent.right
            anchors.rightMargin: 10
            
            // Calculate Y position
            // Approximation: (proxyIndex / rowCount) * height
            y: (modelData.proxyIndex / Math.max(1, proxyModel.rowCount())) * root.height - (height / 2)
            
            Text {
                anchors.centerIn: parent
                text: modelData.year
                color: "#ccc"
                font.pixelSize: 12
                font.bold: true
            }
        }
    }
    
    // Current Date Bubble (Scrubber Thumb)
    Rectangle {
        id: scrubber
        width: 120
        height: 40
        radius: 20
        color: "#2196F3" // Blue
        anchors.right: parent.right
        anchors.rightMargin: 70 // To the left of year markers
        
        // Position based on scroll
        // We use visibleArea.yPosition which is 0.0 to 1.0
        // We need to sync this.
        // If not dragging, update from listView
        property bool isDragging: false
        
        y: isDragging ? y : (listView.visibleArea.yPosition * root.height)
        
        // Clamp y
        onYChanged: {
            if (y < 0) y = 0
            if (y > root.height - height) y = root.height - height
        }
        
        visible: listView.visibleArea.heightRatio < 1.0 // Hide if all content fits
        
        Text {
            anchors.centerIn: parent
            text: root.currentDateString
            color: "white"
            font.bold: true
            font.pixelSize: 14
        }
        
        // Drag Logic
        MouseArea {
            anchors.fill: parent
            drag.target: scrubber
            drag.axis: Drag.YAxis
            drag.minimumY: 0
            drag.maximumY: root.height - scrubber.height
            
            onPressed: {
                scrubber.isDragging = true
                listView.interactive = false // Disable list scroll while dragging
            }
            
            onReleased: {
                scrubber.isDragging = false
                listView.interactive = true
            }
            
            onPositionChanged: {
                if (drag.active) {
                    var pct = scrubber.y / (root.height - scrubber.height)
                    
                    // If content height is accurate we can use contentY
                    // but ListView/GridView calculates contentHeight lazily
                    // which causes out of bounds if setting contentY directly.
                    // It's safer to map to index and positionViewAtIndex
                    var maxIndex = listView.count - 1
                    if (maxIndex >= 0) {
                        var targetIndex = Math.floor(pct * maxIndex)
                        listView.positionViewAtIndex(targetIndex, ListView.Beginning)
                    }
                }
            }
        }
    }
    
    // Current Date Logic
    property string currentDateString: ""
    
    Timer {
        interval: 100
        running: true
        repeat: true
        onTriggered: {
            if (!listView || !proxyModel) return
            
            // Find visible item index
            var idx = listView.indexAt(10, listView.contentY + 50)
            if (idx !== -1) {
                root.currentDateString = proxyModel.getLabelForProxyIndex(idx)
            }
        }
    }
}
