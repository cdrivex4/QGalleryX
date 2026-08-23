import QtQuick
import QtQuick.Controls
import ScrollBenchBackend
import ScrollBenchBackend 1.0

Item {
    id: semanticRoot
    signal imageClicked(int index)
    
    property var model: imageModel
    property int lastSelectedIndex: -1  // Track for Shift+Click
    
    function toggleItemSelection(idx) {
        semanticRoot.model.toggleSelection(idx)
        semanticRoot.lastSelectedIndex = idx
    }
    
    // Enable keyboard focus
    focus: true
    
    // Grouping Mode: 1=Day, 2=Week, 3=Month, 4=Year, 5=Type
    property int groupingMode: 1
    onGroupingModeChanged: {
        semanticRoot.model.setSortMode(groupingMode)
    }
    property bool groupingAuto: true
    
    readonly property real gridResolution: settings.gridResolution
    // DEBOUNCED loading resolution — propagates to delegates only 400ms
    // after the slider stops, preventing a flood of 500+ simultaneous
    // re-requests that crash the thread pool.
    property int loadingResolution: settings.thumbnailSize
    Timer {
        id: resolutionDebounce
        interval: 400
        repeat: false
        onTriggered: semanticRoot.loadingResolution = settings.thumbnailSize
    }
    Connections {
        target: settings
        function onThumbnailSizeChanged() { resolutionDebounce.restart() }
    }
    readonly property alias proxyModel: proxyModel

    function findChildListView() { return list }
    
    // Auto-link grouping mode to grid size using root thresholds
    onGridResolutionChanged: {
        if (groupingAuto) {
            if (gridResolution < root.thresholdYear) groupingMode = 4 // Year
            else if (gridResolution < root.thresholdMonth) groupingMode = 3 // Month
            else if (gridResolution < root.thresholdWeek) groupingMode = 2 // Week
            else groupingMode = 1 // Day
        }
    }

    // Grouped Proxy Model
    GroupedProxyModel {
        id: proxyModel
        sourceModel: semanticRoot.model
        columns: Math.max(1, Math.floor((semanticRoot.width - 40) / (settings.gridResolution + 10)))
        
        groupRole: {
            if (groupingMode === 1) return 260 // SectionDayRole (UserRole + 4)
            if (groupingMode === 2) return 263 // SectionWeekRole (UserRole + 7)
            if (groupingMode === 3) return 261 // SectionMonthRole (UserRole + 5)
            if (groupingMode === 4) return 262 // SectionYearRole (UserRole + 6)
            if (groupingMode === 5) return 264 // SectionTypeRole (UserRole + 8)
            return 260
        }
    }

    // Centralized Action Handler
    function performAction(action, payload) {
        if (action === "ToggleSelect") {
            semanticRoot.model.toggleSelection(payload.index)
            semanticRoot.lastSelectedIndex = payload.index
            
        } else if (action === "RangeSelect") {
            if (semanticRoot.lastSelectedIndex >= 0) {
                semanticRoot.model.selectRange(semanticRoot.lastSelectedIndex, payload.index)
            } else {
                semanticRoot.model.toggleSelection(payload.index)
            }
            semanticRoot.lastSelectedIndex = payload.index
            
        } else if (action === "Open") {
            semanticRoot.imageClicked(payload.index)
            semanticRoot.lastSelectedIndex = payload.index
            
        } else if (action === "Zoom") {
            settings.gridResolution = Math.max(30, Math.min(settings.gridResolution + payload.delta, 400))
        }
    }

    // Keyboard Shortcuts
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_A && event.modifiers & Qt.ControlModifier) {
             semanticRoot.model.selectAll()
             event.accepted = true
        } else if (event.key === Qt.Key_Escape) {
             semanticRoot.model.clearSelection()
             event.accepted = true
        } else if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
             performAction("Zoom", { delta: 20 })
             event.accepted = true
        } else if (event.key === Qt.Key_Minus) {
             performAction("Zoom", { delta: -20 })
             event.accepted = true
        }
    }
    
    function updateVisibleRange() {
        if (!model || list.count === 0 || !list.contentItem) return;
        
        var sourceStart = -1;
        var sourceEnd = -1;
        
        var viewTop = list.contentY;
        var viewBottom = list.contentY + list.height;
        var children = list.contentItem.children;
        
        for (var i = 0; i < children.length; i++) {
            var item = children[i];
            if (!item || !item.hasOwnProperty("capturedType")) continue;
            
            // Fast vertical overlap check
            if (item.y + item.height < viewTop || item.y > viewBottom) continue;
            
            if (item.capturedType === 1) { // Image row
                if (sourceStart === -1 || item.capturedStart < sourceStart) {
                    sourceStart = item.capturedStart;
                }
                var rowEnd = item.capturedStart + item.capturedCount - 1;
                if (sourceEnd === -1 || rowEnd > sourceEnd) {
                    sourceEnd = rowEnd;
                }
            }
        }
        
        // Fallback if only headers or nothing found
        if (sourceStart === -1) sourceStart = 0;
        if (sourceEnd === -1) sourceEnd = Math.min(model.totalItems - 1, sourceStart + 800);
        
        // Failsafe clamp to prevent layout bugs from requesting thousands of thumbnails
        if (sourceEnd - sourceStart > 800) {
            sourceEnd = sourceStart + 800;
        }
        
        if (sourceStart !== -1 && sourceEnd !== -1) {
            model.visibleStartIndex = Math.max(0, sourceStart);
            model.visibleEndIndex = Math.min(model.totalItems - 1, sourceEnd);
        }
    }

    ListView {
        id: list
        anchors.fill: parent
        model: proxyModel
        clip: true
        spacing: 12
        cacheBuffer: 500
        reuseItems: true
        z: 1
        interactive: !dragSelect.active
        
        onContentYChanged: rangeTimer.restart()
        onHeightChanged: rangeTimer.restart()
        
        Timer {
            id: rangeTimer
            interval: 100; repeat: false
            onTriggered: semanticRoot.updateVisibleRange()
        }

        delegate: Item {
            id: rowDelegate
            width: list.width
            height: type === 0 ? 40 : settings.gridResolution + 12
            
            // These properties are scrutinized by updateVisibleRange via peering
            readonly property int capturedType: type
            readonly property int capturedStart: rowStartIndex
            readonly property int capturedCount: rowCount
            readonly property string capturedTitle: headerTitle

            // Header Delegate
            Rectangle {
                anchors.fill: parent
                color: "#1e1e1e"
                visible: rowDelegate.capturedType === 0
                
                Text {
                    anchors.left: parent.left; anchors.leftMargin: 20
                    anchors.verticalCenter: parent.verticalCenter
                    text: rowDelegate.capturedTitle || "No Date"
                    color: "#2196F3"; font.bold: true; font.pixelSize: 18
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#333" }
            }
            
            // Row Delegate
            Row {
                anchors.fill: parent
                anchors.leftMargin: 20; anchors.rightMargin: 20
                spacing: 10
                visible: rowDelegate.capturedType === 1
                
                Repeater {
                    model: rowDelegate.capturedType === 1 ? rowDelegate.capturedCount : 0
                    delegate: Item {
                        width: settings.gridResolution
                        height: settings.gridResolution
                        
                        property int sourceIdx: rowDelegate.capturedStart + index
                        
                        FastImage {
                            id: thumbFast
                            anchors.fill: parent
                            source: {
                                var path = semanticRoot.model.data(semanticRoot.model.index(sourceIdx, 0), 257)
                                return path ? "image://async/" + path : ""
                            }
                            sourceSize: Qt.size(semanticRoot.loadingResolution, semanticRoot.loadingResolution)
                            fillMode: 1 // PreserveAspectCrop
                        }
                        
                        property bool isLoading: thumbFast.isLoading
                        property string activeSource: thumbFast.source    
                        Rectangle {
                            anchors.fill: parent; color: "#222"
                            visible: activeSource === "" || isLoading
                        }

                        // Video Icon Overlay
                        Item {
                            anchors.fill: parent
                            visible: semanticRoot.model.data(semanticRoot.model.index(sourceIdx, 0), 267) // isVideo (Qt::UserRole + 11)
                            Rectangle { anchors.fill: parent; color: "black"; opacity: 0.15 }
                            Text { 
                                anchors.centerIn: parent
                                text: "▶️"
                                font.pixelSize: parent.width * 0.3
                                color: "white"
                                style: Text.Outline; styleColor: "black"
                            }
                        }

                        // RAW Indicator Overlay
                        Rectangle {
                            anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 4
                            property bool isRaw: semanticRoot.model.data(semanticRoot.model.index(sourceIdx, 0), 266) // isRaw (Qt::UserRole + 10)
                            visible: isRaw
                            width: txtRawS.width + 6; height: txtRawS.height + 2
                            color: "#AA000000"; radius: 2
                            Text { id: txtRawS; anchors.centerIn: parent; text: "RAW"; color: "#FF9800"; font.pixelSize: 10; font.bold: true }
                        }

                        // Selection States
                        Rectangle {
                            anchors.fill: parent; color: "#2196F3"
                            // IsSelectedRole is 409
                            property bool isSelected: semanticRoot.model.selectedCount !== -1 && semanticRoot.model.data(semanticRoot.model.index(sourceIdx, 0), 409)
                            opacity: isSelected ? 0.4 : 0
                            border.color: "#2196F3"; border.width: isSelected ? 2 : 0
                            
                            // Scaled Checkmark
                            Rectangle {
                                width: Math.max(10, parent.width * 0.25); height: width; radius: width/2
                                color: parent.isSelected ? "#2196F3" : "#44000000"
                                anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 2
                                visible: parent.isSelected || semanticRoot.model.selectedCount > 0
                                border.color: parent.isSelected ? "white" : "#88ffffff"
                                border.width: 2
                                Text { anchors.centerIn: parent; text: "✓"; color: "white"; font.bold: true; font.pixelSize: parent.width * 0.7; visible: parent.parent.isSelected }
                            }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            onClicked: (mouse) => {
                                if (mouse.modifiers & Qt.ControlModifier) {
                                    semanticRoot.performAction("ToggleSelect", { index: sourceIdx })
                                } else if (mouse.modifiers & Qt.ShiftModifier) {
                                    semanticRoot.performAction("RangeSelect", { index: sourceIdx })
                                } else {
                                    semanticRoot.performAction("Open", { index: sourceIdx })
                                }
                            }
                            onPressAndHold: (mouse) => {
                                semanticRoot.performAction("ToggleSelect", { index: sourceIdx })
                            }
                        }
                    }
                }
            }
        }
        
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }

        WheelHandler {
            acceptedModifiers: Qt.ControlModifier
            onWheel: (wheel) => {
                var delta = wheel.angleDelta.y > 0 ? 20 : -20
                settings.gridResolution = Math.max(30, Math.min(settings.gridResolution + delta, 400))
            }
        }

        PinchHandler {
            property real baseSize
            onActiveChanged: if (active) baseSize = settings.gridResolution
            onScaleChanged: if (active) settings.gridResolution = Math.max(30, Math.min(baseSize * scale, 400))
        }

    // Semantic Drag Selection Logic
    DragHandler {
        id: dragSelect
        target: null // Do not move any items
        enabled: !list.moving && !list.flicking
        acceptedModifiers: Qt.NoModifier | Qt.ShiftModifier | Qt.ControlModifier
            
            property point startPos
            property bool isDragging: false
            
            onActiveChanged: {
                if (active) {
                    startPos = centroid.position
                    isDragging = true
                } else if (isDragging) {
                    isDragging = false
                    // Perform Selection
                    let x1 = Math.min(startPos.x, centroid.position.x)
                    let y1 = Math.min(startPos.y, centroid.position.y)
                    let x2 = Math.max(startPos.x, centroid.position.x)
                    let y2 = Math.max(startPos.y, centroid.position.y)
                    
                    semanticRoot.selectByRect(x1, y1, x2, y2)
                }
            }
        }

        Rectangle {
            id: selectionRect
            visible: dragSelect.active
            color: "#442196F3"; border.color: "#2196F3"; border.width: 2; z: 100
            
            x: Math.min(dragSelect.startPos.x, dragSelect.centroid.position.x)
            y: Math.min(dragSelect.startPos.y, dragSelect.centroid.position.y)
            width: Math.abs(dragSelect.centroid.position.x - dragSelect.startPos.x)
            height: Math.abs(dragSelect.centroid.position.y - dragSelect.startPos.y)
        }
    }
    
    function selectByRect(x1, y1, x2, y2) {
        // Adjust for contentY
        let contentY1 = y1 + list.contentY
        let contentY2 = y2 + list.contentY
        
        let children = list.contentItem.children
        let indicesToSelect = []
        
        for (let i = 0; i < children.length; i++) {
            let item = children[i]
            if (!item || !item.hasOwnProperty("capturedType")) continue
            if (item.y + item.height < contentY1 || item.y > contentY2) continue
            
            if (item.capturedType === 1) { // Row
                let cellTotal = settings.gridResolution + 10 // size + spacing
                let startCol = Math.floor((x1 - 20) / cellTotal)
                let endCol = Math.floor((x2 - 20) / cellTotal)
                
                if (startCol < 0) startCol = 0
                
                for (let c = startCol; c <= endCol; c++) {
                    if (c >= item.capturedCount) break;
                    let cellX = 20 + c * cellTotal
                    let cellRight = cellX + settings.gridResolution
                    
                    if (cellRight > x1 && cellX < x2) {
                        let sourceIdx = item.capturedStart + c
                        if (!semanticRoot.model.data(semanticRoot.model.index(sourceIdx,0), 409)) {
                             indicesToSelect.push(sourceIdx)
                        }
                    }
                }
            }
        }
        
        if (indicesToSelect.length > 0) {
            semanticRoot.model.selectItems(indicesToSelect)
        }
    }

    Component.onCompleted: updateVisibleRange()
}
