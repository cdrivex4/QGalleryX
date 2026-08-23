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
            
            property real lastScrubContentY: 0
            
            onPressed: {
                scrubber.isDragging = true
                if (listView) {
                    lastScrubContentY = listView.contentY
                    listView.interactive = false
                }
            }
            
            onReleased: {
                scrubber.isDragging = false
                if (listView) {
                    listView.interactive = true
                    listView.returnToBounds()
                    if (typeof viewportGovernor !== "undefined" && listView.indexAt) {
                        var sIdx = listView.indexAt(listView.width / 2, listView.contentY)
                        var eIdx = listView.indexAt(listView.width / 2, listView.contentY + listView.height)
                        var modelCount = (proxyModel ? proxyModel.rowCount() : (listView.model ? (listView.model.count !== undefined ? listView.model.count : listView.model.rowCount()) : 0))
                        if (sIdx !== -1 && eIdx !== -1 && modelCount > 0) {
                            viewportGovernor.updateViewport(sIdx, eIdx, modelCount, 0)
                        }
                    }
                }
            }
            
            onPositionChanged: {
                if (drag.active && listView) {
                    var trackHeight = Math.max(1, root.height - scrubber.height)
                    var progress = Math.max(0.0, Math.min(1.0, scrubber.y / trackHeight))
                    var maxScroll = Math.max(0, listView.contentHeight - listView.height)
                    var newContentY = progress * maxScroll
                    var deltaY = newContentY - lastScrubContentY
                    lastScrubContentY = newContentY
                    listView.contentY = newContentY
                    
                    // Unified Viewport Governor update
                    if (typeof viewportGovernor !== "undefined" && listView.indexAt) {
                        var sIdx = listView.indexAt(listView.width / 2, newContentY)
                        var eIdx = listView.indexAt(listView.width / 2, newContentY + listView.height)
                        var modelCount = (proxyModel ? proxyModel.rowCount() : (listView.model ? (listView.model.count !== undefined ? listView.model.count : listView.model.rowCount()) : 0))
                        if (sIdx !== -1 && eIdx !== -1 && modelCount > 0) {
                            viewportGovernor.updateViewport(sIdx, eIdx, modelCount, deltaY)
                            if (listView.model && listView.model.visibleStartIndex !== undefined) {
                                listView.model.visibleStartIndex = sIdx
                                listView.model.visibleEndIndex = eIdx
                            }
                        }
                    }
                    
                    root.updateLabelFromScroll()
                }
            }
        }
    }
    
    // Current Date Logic
    property string currentDateString: ""
    
    function updateLabelFromScroll() {
        if (!listView) return
        var idx = -1
        if (listView.indexAt) {
            idx = listView.indexAt(listView.width / 2, listView.contentY + (listView.height / 2))
            if (idx === -1) idx = listView.indexAt(10, listView.contentY + 50)
        }
        if (idx !== -1) {
            if (proxyModel && typeof proxyModel.getLabelForProxyIndex === "function") {
                root.currentDateString = proxyModel.getLabelForProxyIndex(idx)
            } else if (listView.model) {
                var nameVal = ""
                if (typeof listView.model.data === "function") {
                    nameVal = listView.model.data(listView.model.index(idx, 0), Qt.UserRole + 1) // NameRole
                }
                if (nameVal) root.currentDateString = nameVal.toString()
            }
        }
    }
    
    Timer {
        interval: 100
        running: !scrubber.isDragging
        repeat: true
        onTriggered: root.updateLabelFromScroll()
    }
}
