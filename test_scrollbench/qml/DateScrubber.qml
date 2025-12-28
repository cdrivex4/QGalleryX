import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 150
    
    property var listView
    property var proxyModel
    property var rawModel 
    
    property var yearData: []
    
    function updateYearData() {
        if (proxyModel) {
            yearData = proxyModel.getYearDistribution()
        } else if (rawModel && rawModel.rowCount() > 0) {
            var list = []
            var count = rawModel.rowCount()
            // Sample start, mid, end
            var firstYear = rawModel.data(rawModel.index(0, 0), 262) // SectionYearRole
            if (firstYear) list.push({ year: firstYear, proxyIndex: 0 })
            
            if (count > 1000) {
                var midIdx = Math.floor(count / 2)
                var midYear = rawModel.data(rawModel.index(midIdx, 0), 262)
                if (midYear && midYear !== firstYear) list.push({ year: midYear, proxyIndex: midIdx })
            }
            
            var lastIdx = count - 1
            var lastYear = rawModel.data(rawModel.index(lastIdx, 0), 262)
            if (lastYear && lastYear !== firstYear) list.push({ year: lastYear, proxyIndex: lastIdx })
            yearData = list
        }
    }
    
    function updateLabelFromScroll() {
        if (!listView) return
        var idx = -1
        if (listView.indexAt) {
            // Find item at center of viewport
            idx = listView.indexAt(listView.width / 2, listView.contentY + (listView.height / 2))
            if (idx === -1) idx = listView.indexAt(10, listView.contentY + 50)
        }
        
        if (idx !== -1) {
            if (proxyModel) {
                root.currentDateString = proxyModel.getLabelForProxyIndex(idx)
            } else if (rawModel) {
                root.currentDateString = rawModel.data(rawModel.index(idx, 0), 260) // SectionDayRole
            }
        }
    }

    Connections {
        target: proxyModel ? proxyModel : (rawModel ? rawModel : null)
        function onModelReset() { updateYearData() }
        function onRowsInserted() { updateYearData() }
    }
    
    Component.onCompleted: updateYearData()
    
    // Year Markers
    Repeater {
        model: yearData
        delegate: Rectangle {
            width: 45; height: 20; radius: 10
            color: "#333"; opacity: 0.7
            anchors.right: parent.right
            anchors.rightMargin: 5
            y: {
                var total = proxyModel ? proxyModel.rowCount() : (rawModel ? rawModel.rowCount() : 1)
                return (modelData.proxyIndex / Math.max(1, total)) * root.height - (height / 2)
            }
            Text {
                anchors.centerIn: parent; text: modelData.year
                color: "white"; font.pixelSize: 10; font.bold: true
            }
        }
    }
    
    // Bubble
    Rectangle {
        id: bubble
        width: 110; height: 40; radius: 20
        color: "#2196F3"
        anchors.right: parent.right
        anchors.rightMargin: 55
        z: 10
        
        property bool isDragging: false
        
        y: {
            if (isDragging) return y;
            if (!listView || listView.contentHeight <= 0) return 0;
            // Map contentY to track height
            var trackHeight = root.height - bubble.height
            var progress = listView.contentY / (listView.contentHeight - listView.height)
            return Math.max(0, Math.min(progress * trackHeight, trackHeight))
        }
        
        Text {
            anchors.centerIn: parent
            text: root.currentDateString
            color: "white"; font.bold: true; font.pixelSize: 13
        }
        
        MouseArea {
            anchors.fill: parent
            drag.target: bubble; drag.axis: Drag.YAxis
            drag.minimumY: 0; drag.maximumY: root.height - bubble.height
            
            onPressed: {
                bubble.isDragging = true
                if (listView) listView.interactive = false
            }
            onReleased: {
                bubble.isDragging = false
                if (listView) {
                    listView.interactive = true
                    listView.returnToBounds()
                }
            }
            onPositionChanged: {
                if (drag.active && listView) {
                    var trackHeight = root.height - bubble.height
                    var progress = bubble.y / trackHeight
                    listView.contentY = progress * (listView.contentHeight - listView.height)
                    root.updateLabelFromScroll() // Instant feedback
                }
            }
        }
    }
    
    property string currentDateString: "---"
    
    Timer {
        interval: 200; running: !bubble.isDragging; repeat: true
        onTriggered: root.updateLabelFromScroll()
    }
}
