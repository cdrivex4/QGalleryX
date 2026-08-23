import QtQuick
import QtQuick.Controls
import QGalleryX 1.0

Item {
    id: root
    
    // Signals
    signal imageClicked(int index)
    signal imageLoaded(int timeMs)
    signal sectionChanged(string section)
    
    // Properties
    property real uiThumbnailSize: appSettings ? appSettings.gridResolution : 100
    property int loadingResolution: appSettings ? appSettings.thumbnailSize : 200
    
    // Dynamically downsample the loaded texture to match the UI grid size, saving massive VRAM
    property int effectiveLoadingResolution: {
        if (uiThumbnailSize <= 48) return 64
        if (uiThumbnailSize <= 128) return 128
        return 256
    }
    property string folderPath: ""
    
    onFolderPathChanged: {
        if (folderPath !== "") scanFolder(folderPath)
    }
    
    // Helper to scan folder
    function scanFolder(path) {
        console.log("[QML_DEBUG] Scanning folder in ImageModel:", path)
        if (root.model) root.model.scanDirectory(path)
    }

    ImageModel {
        id: localImageModel
    }

    // Model property (passed from parent/Main.qml)
    property var model: localImageModel
    property alias activeModel: root.model
    property alias grid: listView
    property alias gridView: listView
    property int caretIndex: 0

    function ensureCaretVisible() {
        if (proxyModel && root.model && root.model.count > 0) {
            var proxyRow = proxyModel.getProxyRowForSourceIndex(root.caretIndex)
            if (proxyRow >= 0) {
                listView.positionViewAtIndex(proxyRow, ListView.Contain)
            }
        }
    }

    onCaretIndexChanged: ensureCaretVisible()

    // Grouping Mode: 0=Auto, 1=Day, 2=Week, 3=Month, 4=Year
    property int groupingMode: 0

    GroupedProxyModel {
        id: proxyModel
        sourceModel: root.model
        // Calculate columns based on current grid size
        columns: Math.max(1, Math.floor(root.width / root.uiThumbnailSize))
        
        // Dynamic Grouping Role
        groupRole: {
            if (root.groupingMode === 1) return ImageModel.SectionDayRole
            if (root.groupingMode === 2) return ImageModel.SectionWeekRole
            if (root.groupingMode === 3) return ImageModel.SectionMonthRole
            if (root.groupingMode === 4) return ImageModel.SectionYearRole
            
            // Auto
            if (root.uiThumbnailSize < 80) return ImageModel.SectionYearRole
            if (root.uiThumbnailSize < 150) return ImageModel.SectionMonthRole
            return ImageModel.SectionDayRole
        }
    }

    // Zoom State
    property real currentScale: 1.0
    property real startPinchGridResolution: 100
    property int zoomTargetSourceIndex: -1
    property real zoomTargetRelativeY: 0.5 // 0 to 1 (relative to viewport height)

    // Helper to find source index at screen coordinates
    function getSourceIndexAt(screenX, screenY) {
        // Map screen coordinates to content coordinates
        var contentX = screenX + listView.contentX
        var contentY = screenY + listView.contentY
        
        var item = listView.itemAt(contentX, contentY)
        if (item) {
            var loadedItem = item.item
            if (loadedItem && loadedItem.hasOwnProperty("rowStartIndex")) {
                var rowLocalX = contentX - item.x
                var col = Math.floor(rowLocalX / loadedItem.itemSize)
                col = Math.max(0, Math.min(col, loadedItem.rowCount - 1))
                return loadedItem.rowStartIndex + col
            }
        }
        return -1
    }

    // Main Viewport
    Item {
        id: viewport
        anchors.fill: parent
        clip: true
        
        // The List View
        ListView {
            id: listView
            anchors.fill: parent
            model: proxyModel
            focus: true
            keyNavigationEnabled: false

            Component.onCompleted: listView.forceActiveFocus()

            Keys.onPressed: (event) => {
                var cols = Math.max(1, proxyModel.columns)
                var rowsPerPage = Math.max(1, Math.floor(listView.height / (listView.width / cols)))
                var pageStep = rowsPerPage * cols
                var count = root.model ? root.model.count : 0

                if (count === 0) return
                if (root.caretIndex < 0) root.caretIndex = 0

                if (event.key === Qt.Key_Left) {
                    root.caretIndex = Math.max(0, root.caretIndex - 1)
                    event.accepted = true
                } else if (event.key === Qt.Key_Right) {
                    root.caretIndex = Math.min(count - 1, root.caretIndex + 1)
                    event.accepted = true
                } else if (event.key === Qt.Key_Up) {
                    if (root.caretIndex < cols) {
                        searchField.forceActiveFocus()
                    } else {
                        root.caretIndex = Math.max(0, root.caretIndex - cols)
                    }
                    event.accepted = true
                } else if (event.key === Qt.Key_Down) {
                    if (root.caretIndex >= count - cols) {
                        bottomBar.focusTab(0)
                    } else {
                        root.caretIndex = Math.min(count - 1, root.caretIndex + cols)
                    }
                    event.accepted = true
                } else if (event.key === Qt.Key_PageUp) {
                    root.caretIndex = Math.max(0, root.caretIndex - pageStep)
                    event.accepted = true
                } else if (event.key === Qt.Key_PageDown) {
                    root.caretIndex = Math.min(count - 1, root.caretIndex + pageStep)
                    event.accepted = true
                } else if (event.key === Qt.Key_Home) {
                    root.caretIndex = 0
                    event.accepted = true
                } else if (event.key === Qt.Key_End) {
                    root.caretIndex = count - 1
                    event.accepted = true
                } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    if (root.caretIndex >= 0 && root.caretIndex < count) {
                        root.imageClicked(root.caretIndex)
                    }
                    event.accepted = true
                } else if (event.key === Qt.Key_Space) {
                    if (root.model && typeof root.model.toggleSelection === "function") {
                        root.model.toggleSelection(root.caretIndex)
                    }
                    event.accepted = true
                } else if (event.key === Qt.Key_Tab) {
                    bottomBar.focusTab(0)
                    event.accepted = true
                } else if (event.key === Qt.Key_Backtab) {
                    if (groupCombo.visible) groupCombo.forceActiveFocus()
                    else viewModeBtn.forceActiveFocus()
                    event.accepted = true
                }
            }
            
            // Visual Scaling
            scale: root.currentScale
            transformOrigin: Item.Center
            
            // Dynamic cacheBuffer optimized for 100k+ items
            cacheBuffer: Math.min(viewport.height * 2, 500)

            property real lastContentY: 0
            property real cellHeight: root.width / proxyModel.columns

            function updateViewportNow() {
                var sIdx = indexAt(width / 2, contentY)
                var eIdx = indexAt(width / 2, contentY + height)
                
                if (sIdx === -1) {
                    // Approximate fallback for semantic view
                    sIdx = Math.max(0, Math.floor(contentY / cellHeight) * 3)
                }
                if (eIdx === -1) {
                    eIdx = Math.min(root.model ? root.model.count - 1 : 0, sIdx + Math.ceil(height / cellHeight) * 3 + 6)
                }

                if (typeof viewportGovernor !== "undefined" && root.model) {
                    viewportGovernor.updateViewport(sIdx, eIdx, root.model.count, contentY - lastContentY)
                    root.model.visibleStartIndex = sIdx
                    root.model.visibleEndIndex = eIdx
                }
            }

            onContentYChanged: {
                updateViewportNow()
                
                var centerIdx = indexAt(width / 2, contentY + height / 2)
                if (centerIdx !== -1 && root.model) {
                    // Section roles: 0x0106 = Day, 0x0107 = Month, 0x0108 = Year
                    var sectionStr = ""
                    if (root.uiThumbnailSize < 80) sectionStr = root.model.data(root.model.index(centerIdx, 0), 0x0108) 
                    else if (root.uiThumbnailSize < 150) sectionStr = root.model.data(root.model.index(centerIdx, 0), 0x0107) 
                    else sectionStr = root.model.data(root.model.index(centerIdx, 0), 0x0106)
                    
                    if (sectionStr !== "") {
                        root.sectionChanged(sectionStr)
                    }
                }
                
                lastContentY = contentY
            }
            
            onCountChanged: {
                Qt.callLater(updateViewportNow)
            }
            onHeightChanged: Qt.callLater(updateViewportNow)
            onWidthChanged: Qt.callLater(updateViewportNow)
            
            // ScrollBar
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
                active: true
            }

            // Delegate Loader
            delegate: Loader {
                id: delegateLoader
                width: listView.width
                height: {
                    if (model.type === 0) return 40 // Header Height
                    // Row Height: Use calculated item size
                    return listView.width / proxyModel.columns
                }
                
                sourceComponent: model.type === 0 ? headerDelegate : rowDelegate
                
                // Explicitly pass model roles to the loaded item using Binding
                Binding { target: delegateLoader.item; property: "headerTitle"; value: model.headerTitle }
                Binding { target: delegateLoader.item; property: "rowCount"; value: model.rowCount }
                Binding { target: delegateLoader.item; property: "rowStartIndex"; value: model.rowStartIndex }
            }
        }
    }
    
    // Pinch Handler for Smooth Zoom
    PinchHandler {
        id: pinchHandler
        target: null // We handle manually
        
        onActiveChanged: {
            if (active) {
                // Pinch Started
                root.startPinchGridResolution = appSettings.gridResolution
                listView.interactive = false // Disable scrolling during pinch
                
                // Capture target
                root.zoomTargetSourceIndex = root.getSourceIndexAt(centroid.position.x, centroid.position.y)
                root.zoomTargetRelativeY = centroid.position.y / viewport.height
                
                // Set transform origin to pinch center for visual zoom
                // We need to adjust listView position to keep visual stability if we change origin?
                // For now, let's just keep Center or try to map centroid.
                // Actually, simply scaling around center is easier, but less "Google Maps".
                // To do proper pinch zoom, we should scale around centroid.
                listView.transformOriginPoint = Qt.point(centroid.position.x, centroid.position.y)
                
            } else {
                // Pinch Ended - Commit Zoom
                var newSize = root.startPinchGridResolution * root.currentScale
                newSize = Math.max(32, Math.min(newSize, 256)) // Min 32px, Max 256px
                
                // Reset Scale
                root.currentScale = 1.0
                
                // Apply new size (this will trigger re-layout)
                appSettings.gridResolution = newSize
                
                // Restore Position
                if (root.zoomTargetSourceIndex !== -1) {
                    // Find new row for this source index
                    var newProxyIndex = proxyModel.getProxyIndexForSourceIndex(root.zoomTargetSourceIndex)
                    if (newProxyIndex.valid) {
                        // Position the view such that the row is at the same relative Y
                        listView.positionViewAtIndex(newProxyIndex.row, ListView.Beginning)
                        listView.contentY -= (root.zoomTargetRelativeY * viewport.height)
                    }
                }
                
                listView.interactive = true
            }
        }
        
        onScaleChanged: (delta) => {
            // Update visual scale
            var nextScale = root.currentScale * delta
            
            // Limit visual scale to reasonable bounds relative to start
            // e.g. 0.5x to 3x
            if (nextScale > 0.5 && nextScale < 3.0) {
                root.currentScale = nextScale
            }
        }
    }
    
    // Mouse Wheel Zoom (Ctrl + Wheel)
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        propagateComposedEvents: true
        
        onWheel: (wheel) => {
            if (wheel.modifiers & Qt.ControlModifier) {
                // Capture target
                var targetSourceIndex = root.getSourceIndexAt(wheel.x, wheel.y)
                var targetRelativeY = wheel.y / viewport.height
                
                var oldSize = appSettings.gridResolution
                var newSize = oldSize
                
                if (wheel.angleDelta.y > 0) {
                    newSize = Math.min(oldSize * 1.2, 256)
                } else {
                    newSize = Math.max(oldSize / 1.2, 32) // Min 32px
                }
                
                if (newSize !== oldSize) {
                    appSettings.gridResolution = newSize
                    
                    // Restore Position
                    if (targetSourceIndex !== -1) {
                        var newProxyIndex = proxyModel.getProxyIndexForSourceIndex(targetSourceIndex)
                        if (newProxyIndex.valid) {
                            listView.positionViewAtIndex(newProxyIndex.row, ListView.Beginning)
                            listView.contentY -= (targetRelativeY * viewport.height)
                        }
                    }
                }
                wheel.accepted = true
            } else {
                wheel.accepted = false
            }
        }
    }
    
    // Date Scrubber
    DateScrubber {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        
        listView: listView
        proxyModel: proxyModel
        
        visible: !pinchHandler.active
    }

    // Delegates
    Component {
        id: headerDelegate
        Rectangle {
            property string headerTitle
            property int rowCount
            property int rowStartIndex
            
            width: listView.width
            height: 40
            color: "#000000"
            opacity: 0.9
            
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.verticalCenter: parent.verticalCenter
                text: headerTitle
                color: "white"
                font.bold: true
                font.pixelSize: 18
                
                // Keep text size constant during zoom
                scale: 1.0 / Math.max(0.1, root.currentScale)
                transformOrigin: Item.Left
            }
            
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#333"
            }
        }
    }

    Component {
        id: rowDelegate
        Item {
            property string headerTitle
            property int rowCount
            property int rowStartIndex
            
            property real itemSize: listView.width / proxyModel.columns
            
            width: listView.width
            height: itemSize
            
            Row {
                anchors.fill: parent
                spacing: 0
                
                Repeater {
                    model: rowCount
                    
                    delegate: Item {
                        width: itemSize
                        height: itemSize
                        
                        property int sourceIndex: rowStartIndex + index
                        property bool isCaretActive: root.caretIndex === sourceIndex
                        
                        // Fetch data
                        property string filePath: root.model.data(root.model.index(sourceIndex, 0), ImageModel.FilePathRole)
                        property int fileType: desktopHelper ? desktopHelper.getFileType(filePath) : 0
                        property bool isVideo: fileType === 2 // DesktopHelper.Video
                        property bool isSelected: root.model.data(root.model.index(sourceIndex, 0), ImageModel.IsSelectedRole)
                        
                        Connections {
                            target: root.model
                            function onSelectedCountChanged() {
                                isSelected = root.model.data(root.model.index(sourceIndex, 0), ImageModel.IsSelectedRole)
                            }
                        }
                        
                        Image {
                            id: img
                            anchors.fill: parent
                            anchors.margins: 1
                            sourceSize: Qt.size(root.effectiveLoadingResolution, root.effectiveLoadingResolution)
                            source: filePath ? "image://async/" + filePath + "?idx=" + sourceIndex : ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: true
                            
                            property int retries: 0
                            property real startTime: 0
                            onSourceChanged: startTime = new Date().getTime()
                            
                            Timer {
                                id: retryTimer
                                interval: 1000 + (Math.random() * 2000)
                                repeat: false
                                onTriggered: {
                                    var original = filePath ? "image://async/" + filePath + "?idx=" + sourceIndex : ""
                                    parent.source = ""
                                    parent.source = original + "&retry=" + parent.retries
                                }
                            }
                            
                            onStatusChanged: {
                                if (status === Image.Error && retries < 3) {
                                    retries++
                                    retryTimer.start()
                                } else if (status === Image.Ready) {
                                    retries = 0
                                    var endTime = new Date().getTime()
                                    var duration = endTime - startTime
                                    if (startTime > 0) {
                                        root.imageLoaded(duration)
                                    }
                                }
                            }
                            
                            Rectangle {
                                anchors.fill: parent
                                color: "transparent"
                                visible: isVideo
                                
                                // Optional: dimming for video distinctness
                                Rectangle { 
                                    anchors.fill: parent
                                    color: "black"
                                    opacity: 0.2
                                }
                                Text {
                                    anchors.centerIn: parent
                                    text: "▶"
                                    color: "white"
                                    font.pixelSize: Math.max(10, Math.min(24, parent.height * 0.5))
                                }
                            }
                            
                            // Windows Explorer Single File Focus / Caret Highlight Box
                            Rectangle {
                                anchors.fill: parent
                                color: isCaretActive ? "#443B82F6" : (isSelected ? "#440078D7" : "transparent")
                                border.color: isCaretActive ? "#38BDF8" : (isSelected ? "#0078D7" : "transparent")
                                border.width: isCaretActive ? 3 : (isSelected ? 2 : 0)
                                radius: 2
                                z: 15
                                visible: isCaretActive || isSelected

                                // High-contrast inner white ring so caret is unmistakably visible on dark/light images
                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: 1
                                    color: "transparent"
                                    border.color: "#FFFFFF"
                                    border.width: 1
                                    radius: 1
                                    opacity: 0.6
                                    visible: isCaretActive
                                }
                            }
                            
                            Rectangle {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.margins: Math.max(2, parent.width * 0.05)
                                width: Math.max(12, Math.min(24, parent.width * 0.3))
                                height: width
                                radius: width / 2
                                color: isSelected ? "#0078D7" : "#44000000"
                                border.color: "white"
                                border.width: 1.5
                                visible: root.model.selectedCount > 0 || isSelected
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "✓"
                                    color: "white"
                                    font.pixelSize: Math.max(8, parent.width * 0.6)
                                    font.bold: true
                                    visible: isSelected
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        root.caretIndex = sourceIndex
                                        root.model.setData(root.model.index(sourceIndex, 0), !isSelected, ImageModel.IsSelectedRole)
                                    }
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root.caretIndex = sourceIndex
                                    if (root.model.selectedCount > 0) {
                                        root.model.setData(root.model.index(sourceIndex, 0), !isSelected, ImageModel.IsSelectedRole)
                                    } else {
                                        root.imageClicked(sourceIndex)
                                    }
                                }
                                onPressAndHold: {
                                    root.caretIndex = sourceIndex
                                    if (!isSelected) {
                                        root.model.setData(root.model.index(sourceIndex, 0), true, ImageModel.IsSelectedRole)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    Text {
        anchors.centerIn: parent
        text: "No images found."
        color: "#888"
        visible: root.model ? (root.model.count === 0 && !root.model.isLoading) : false
        font.pixelSize: 18
        font.bold: true
    }
}
