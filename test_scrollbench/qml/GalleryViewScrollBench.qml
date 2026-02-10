import QtQuick
import QtQuick.Controls

Item {
    id: root
    signal imageClicked(int index)
    signal imageLoaded(int timeMs)
    
    // Selection state tracking
    property int lastSelectedIndex: -1
    
    // Enable keyboard focus for shortcuts
    focus: true

    function scanFolder(path) {
        imageModel.scanDirectory(path)
    }

    function findChildGridView() {
        return grid
    }

    property var model: imageModel
    
    // UI Grid Size (Zoom level)
    readonly property real uiThumbnailSize: settings.gridSize
    
    // Loading Resolution (Quality/Performance setting)
    readonly property int loadingResolution: settings.thumbnailSize
    
    // Timer to debounce viewport updates
    Timer {
        id: updateTimer
        interval: 150 
        repeat: false
        onTriggered: grid.updateVisibleRange()
    }

    property real lastRequestedY: 0

    // Centralized Action Handler
    // Unifies Mouse, Keyboard, and Touch interactions into a single logic flow
    function performAction(action, payload) {
        // action: "ToggleSelect", "RangeSelect", "Open", "Zoom", "Navigate"
        
        if (action === "ToggleSelect") {
            root.model.toggleSelection(payload.index)
            root.lastSelectedIndex = payload.index
            
        } else if (action === "RangeSelect") {
            if (root.lastSelectedIndex >= 0) {
                root.model.selectRange(root.lastSelectedIndex, payload.index)
            } else {
                root.model.toggleSelection(payload.index)
            }
            root.lastSelectedIndex = payload.index
            
        } else if (action === "Open") {
            // Removed clearSelection to solve "forgotten selection" issue
            root.imageClicked(payload.index)
            root.lastSelectedIndex = payload.index // Track last focused item
            
        } else if (action === "Navigate") {
            // payload: { direction: int } (-1 prev, +1 next)
            // Logic handled by view focus, but could be extended here
            
        } else if (action === "Zoom") {
            // payload: { delta: real }
            settings.gridSize = Math.max(40, Math.min(settings.gridSize + payload.delta, 400))
            updateTimer.restart()
        }
    }

    // Keyboard Shortcuts routed through centralized handler
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_A && event.modifiers & Qt.ControlModifier) {
             imageModel.selectAll()
             event.accepted = true
        } else if (event.key === Qt.Key_Escape) {
             imageModel.clearSelection()
             event.accepted = true
        } else if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
             performAction("Zoom", { delta: 20 })
             event.accepted = true
        } else if (event.key === Qt.Key_Minus) {
             performAction("Zoom", { delta: -20 })
             event.accepted = true
        }
    }

    // Drag-to-Select Handler
    DragHandler {
        id: dragSelect
        target: null
        enabled: !grid.moving && !grid.flicking
        
        property point startPos
        property bool isDragging: false
        
        onActiveChanged: {
            if (active) {
                startPos = centroid.position
                isDragging = true
            } else if (isDragging) {
                isDragging = false
                // Calculate grid selection rectangle
                let x1 = Math.min(startPos.x, centroid.position.x)
                let y1 = Math.min(startPos.y, centroid.position.y)
                let x2 = Math.max(startPos.x, centroid.position.x)
                let y2 = Math.max(startPos.y, centroid.position.y)
                
                let cols = Math.max(1, Math.floor(grid.width / grid.cellWidth))
                let colMin = Math.floor(x1 / grid.cellWidth)
                let colMax = Math.floor(x2 / grid.cellWidth)
                let rowMin = Math.floor((y1 + grid.contentY) / grid.cellHeight)
                let rowMax = Math.floor((y2 + grid.contentY) / grid.cellHeight)
                
                imageModel.selectVisualRect(colMin, colMax, rowMin, rowMax, cols)
            }
        }
        
    }

    // Visual drag feedback
    Rectangle {
        id: selectionRect
        visible: dragSelect.active
        color: "#442196F3"
        border.color: "#2196F3"
        border.width: 2
        z: 100
        
        // Use DragHandler's internal points for stability
        x: Math.min(dragSelect.startPos.x, dragSelect.centroid.position.x)
        y: Math.min(dragSelect.startPos.y, dragSelect.centroid.position.y)
        width: Math.abs(dragSelect.centroid.position.x - dragSelect.startPos.x)
        height: Math.abs(dragSelect.centroid.position.y - dragSelect.startPos.y)
    }

    GridView {
        id: grid
        anchors.fill: parent
        cellWidth: settings.gridSize
        cellHeight: settings.gridSize
        model: imageModel
        clip: true
        interactive: !dragSelect.active
        
        onMovingChanged: if (!moving) updateTimer.restart()
        onFlickingChanged: if (!flicking) updateTimer.restart()

        // Robust math-based visible range calculation
        function updateVisibleRange() {
            if (!model || grid.count === 0) return;
            if (grid.width <= 0 || grid.cellWidth <= 0) return;

            let cols = Math.floor(grid.width / grid.cellWidth);
            if (cols < 1) cols = 1;

            let firstRow = Math.floor(grid.contentY / grid.cellHeight);
            let lastRow = Math.ceil((grid.contentY + grid.height) / grid.cellHeight);
            
            let firstVisible = firstRow * cols;
            let lastVisible = (lastRow * cols) + cols;

            // Clamp and update C++ model
            model.visibleStartIndex = Math.max(0, firstVisible);
            model.visibleEndIndex = Math.min(grid.count - 1, lastVisible);
            root.lastRequestedY = grid.contentY
            
            console.log("Viewport settled at:", model.visibleStartIndex, "-", model.visibleEndIndex);
        }

        // Trigger debounce timer on movement or size change
        onContentYChanged: {
            if (Math.abs(contentY - root.lastRequestedY) > cellHeight * 3) {
                updateVisibleRange()
            }
            updateTimer.restart()
        }
        onHeightChanged: updateTimer.restart()
        onWidthChanged: updateTimer.restart()
        
        WheelHandler {
            acceptedModifiers: Qt.ControlModifier
            onWheel: (wheel) => {
                var oldSize = settings.gridSize
                var newSize = oldSize
                if (wheel.angleDelta.y > 0) {
                    newSize = Math.min(oldSize + 20, 400)
                } else {
                    newSize = Math.max(oldSize - 20, 40)
                }
                
                if (newSize !== oldSize) {
                    settings.gridSize = newSize
                    updateTimer.restart()
                }
            }
        }

        Connections {
            target: settings
            function onGridSizeChanged() {
                grid.forceLayout()
                updateTimer.restart()
            }
        }

        Connections {
            target: root.model
            function onForceUpdateGridView() {
                updateTimer.restart()
            }
        }
        
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
            active: true
        }

        delegate: Item {
            width: grid.cellWidth
            height: grid.cellHeight
            
            property bool isVideo: model.isVideo !== undefined ? model.isVideo : false
            property bool isRaw: model.isRaw !== undefined ? model.isRaw : false
            property bool isSelected: model.isSelected !== undefined ? model.isSelected : false

            Image {
                id: img
                anchors.fill: parent
                anchors.margins: 1
                // CRITICAL: Reset source on delegate reuse to force fresh load
                source: {
                    if (!model.filePath) return "";
                    // Force reevaluation by including index
                    return "image://async/" + model.filePath + "?idx=" + model.imageIndex;
                }
                sourceSize.width: root.loadingResolution
                sourceSize.height: root.loadingResolution
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                visible: status === Image.Ready // Hide recycled content
                cache: false  // FORCE FRESH LOADS - NOT USING QML CACHE
                mipmap: true 
                
                // Video Play Icon
                Item {
                    anchors.fill: parent
                    visible: isVideo
                    Rectangle { anchors.fill: parent; color: "black"; opacity: 0.15 }
                    Text { 
                        anchors.centerIn: parent
                        text: "▶️"
                        font.pixelSize: parent.width * 0.3
                        color: "white"
                        style: Text.Outline; styleColor: "black"
                    }
                }

                // RAW Indicator
                Rectangle {
                    anchors.top: parent.top; anchors.left: parent.left
                    anchors.margins: 4
                    width: txtRaw.width + 6; height: txtRaw.height + 2
                    color: "#AA000000"; radius: 2; visible: isRaw
                    Text {
                        id: txtRaw
                        anchors.centerIn: parent
                        text: "RAW"
                        color: "#FF9800"
                        font.pixelSize: 10; font.bold: true
                    }
                }
                
                property bool hasError: status === Image.Error
                property real loadStartTime: 0
                Component.onCompleted: loadStartTime = new Date().getTime()
                property bool hasReported: false
                
                onStatusChanged: {
                    if (status === Image.Ready && !hasReported) {
                        hasReported = true
                        var timeTaken = new Date().getTime() - (loadStartTime || new Date().getTime())
                        telemetry.reportLoadTime(timeTaken)
                    }
                }
                
                Rectangle {
                    anchors.fill: parent; color: "#1a1a1a"
                    visible: img.status !== Image.Ready
                    BusyIndicator { 
                        anchors.centerIn: parent
                        width: parent.width * 0.4; height: width
                        visible: img.status === Image.Loading
                        opacity: 0.5
                    }
                    Text { 
                        anchors.centerIn: parent
                        text: "⚠️"
                        visible: img.status === Image.Error
                        color: "#ff4444"
                    }
                }
                
                // Selection Visual Feedback
                Rectangle {
                    anchors.fill: parent
                    color: "#2196F3"
                    opacity: isSelected ? 0.4 : 0
                    border.color: "#2196F3"
                    border.width: isSelected ? 3 : 0
                    
                    // Checkmark
                    Rectangle {
                        width: Math.max(16, parent.width * 0.25)
                        height: width
                        radius: width/2
                        color: "#2196F3"
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: 4
                        visible: isSelected
                        border.color: "white"
                        border.width: 2
                        
                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: "white"
                            font.bold: true
                            font.pixelSize: parent.width * 0.6
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    // Propagate composition events if needed, but usually not for selection
                    onClicked: (mouse) => {
                        if (mouse.modifiers & Qt.ControlModifier) {
                            root.performAction("ToggleSelect", { index: index })
                        } else if (mouse.modifiers & Qt.ShiftModifier) {
                            root.performAction("RangeSelect", { index: index })
                        } else {
                            root.performAction("Open", { index: index })
                        }
                    }
                    onPressAndHold: (mouse) => {
                        root.performAction("ToggleSelect", { index: index })
                    }
                }
            }
        }
    }

    PinchHandler {
        property real baseSize
        onActiveChanged: if (active) baseSize = settings.gridSize
        onScaleChanged: if (active) {
            settings.gridSize = Math.max(40, Math.min(baseSize * scale, 400))
            updateTimer.restart()
        }
    }
    
    Text {
        anchors.centerIn: parent
        text: "No images found.\nCheck folder permissions or select a different folder."
        color: "#888"
        horizontalAlignment: Text.AlignHCenter
        visible: grid.count === 0
        font.pixelSize: 18
    }
}