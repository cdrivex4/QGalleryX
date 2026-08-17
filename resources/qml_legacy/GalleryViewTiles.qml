import QtQuick
import QtQuick.Controls
import QGalleryX 1.0

Item {
    id: root
    
    // Signals
    signal imageClicked(int index)
    signal imageLoaded(int timeMs)
    
    // Properties
    property real uiThumbnailSize: appSettings ? appSettings.gridResolution : 100
    property int loadingResolution: appSettings ? appSettings.thumbnailSize : 200
    property string folderPath: ""
    
    onFolderPathChanged: {
        console.log("[QML_DEBUG] GalleryViewTiles folderPath changed to:", folderPath)
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


    // Zoom State
    property real currentScale: 1.0
    property real startPinchGridResolution: 100
    property int zoomTargetIndex: -1
    
    // Timeline State
    property int groupingMode: 1 // 1=Day, 3=Month, 4=Year
    property string currentSectionLabel: ""

    // Main Viewport
    GridView {
        id: gridView
        anchors.fill: parent
        clip: true
        focus: true
        keyNavigationEnabled: false

        Component.onCompleted: gridView.forceActiveFocus()

        Keys.onPressed: (event) => {
            var cols = Math.max(1, Math.floor(gridView.width / gridView.cellWidth))
            var rowsPerPage = Math.max(1, Math.floor(gridView.height / gridView.cellHeight))
            var pageStep = rowsPerPage * cols
            var count = gridView.count

            if (count === 0) return
            if (gridView.currentIndex < 0) gridView.currentIndex = 0

            if (event.key === Qt.Key_Left) {
                gridView.currentIndex = Math.max(0, gridView.currentIndex - 1)
                gridView.positionViewAtIndex(gridView.currentIndex, GridView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_Right) {
                gridView.currentIndex = Math.min(count - 1, gridView.currentIndex + 1)
                gridView.positionViewAtIndex(gridView.currentIndex, GridView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_Up) {
                if (gridView.currentIndex < cols) {
                    searchField.forceActiveFocus()
                } else {
                    gridView.currentIndex = Math.max(0, gridView.currentIndex - cols)
                    gridView.positionViewAtIndex(gridView.currentIndex, GridView.Contain)
                }
                event.accepted = true
            } else if (event.key === Qt.Key_Down) {
                if (gridView.currentIndex >= count - cols) {
                    bottomBar.focusTab(0)
                } else {
                    gridView.currentIndex = Math.min(count - 1, gridView.currentIndex + cols)
                    gridView.positionViewAtIndex(gridView.currentIndex, GridView.Contain)
                }
                event.accepted = true
            } else if (event.key === Qt.Key_PageUp) {
                gridView.currentIndex = Math.max(0, gridView.currentIndex - pageStep)
                gridView.positionViewAtIndex(gridView.currentIndex, GridView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_PageDown) {
                gridView.currentIndex = Math.min(count - 1, gridView.currentIndex + pageStep)
                gridView.positionViewAtIndex(gridView.currentIndex, GridView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_Home) {
                gridView.currentIndex = 0
                gridView.positionViewAtIndex(gridView.currentIndex, GridView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_End) {
                gridView.currentIndex = count - 1
                gridView.positionViewAtIndex(gridView.currentIndex, GridView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                if (gridView.currentIndex >= 0 && gridView.currentIndex < count) {
                    root.imageClicked(gridView.currentIndex)
                }
                event.accepted = true
            } else if (event.key === Qt.Key_Space) {
                if (root.model && typeof root.model.toggleSelection === "function") {
                    root.model.toggleSelection(gridView.currentIndex)
                }
                event.accepted = true
            } else if (event.key === Qt.Key_Tab) {
                bottomBar.focusTab(0)
                event.accepted = true
            } else if (event.key === Qt.Key_Backtab) {
                viewModeBtn.forceActiveFocus()
                event.accepted = true
            }
        }
        
        model: root.model
        
        cellWidth: root.uiThumbnailSize
        cellHeight: root.uiThumbnailSize
        
        // Performance
        cacheBuffer: 1000
        
        // ScrollBar
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
            active: true
        }
        
        function updateViewportNow() {
            var sIdx = indexAt(width / 2, contentY)
            var eIdx = indexAt(width / 2, contentY + height)

            if (typeof viewportGovernor !== "undefined" && sIdx !== -1 && eIdx !== -1 && root.model) {
                viewportGovernor.updateViewport(sIdx, eIdx, root.model.count, contentY - root.lastContentY)
                root.model.visibleStartIndex = sIdx
                root.model.visibleEndIndex = eIdx
            }
        }

        onContentYChanged: {
            updateViewportNow()
            
            // Calculate center item index to determine current date section
            var index = indexAt(width / 2, contentY + cellHeight / 2)
            if (index === -1) index = indexAt(width / 2, contentY)
            
            if (index !== -1 && root.model) {
                var role = 260; // Day
                if (root.groupingMode === 3) role = 261; // Month
                if (root.groupingMode === 4) role = 262; // Year
                
                var section = root.model.data(root.model.index(index, 0), role)
                if (section !== undefined) root.currentSectionLabel = section
            }
            
            root.lastContentY = contentY
        }
        
        onCountChanged: Qt.callLater(updateViewportNow)
        onHeightChanged: Qt.callLater(updateViewportNow)
        onWidthChanged: Qt.callLater(updateViewportNow)
        
        // Animations
        add: Transition { NumberAnimation { properties: "x,y"; duration: 200 } }
        move: Transition { NumberAnimation { properties: "x,y"; duration: 200 } }
        displaced: Transition { NumberAnimation { properties: "x,y"; duration: 200 } }

        delegate: Item {
            width: gridView.cellWidth
            height: gridView.cellHeight
            
            // Fetch data
            property string filePath: model.filePath
            property int fileType: desktopHelper ? desktopHelper.getFileType(filePath) : 0
            property bool isVideo: fileType === 2 // DesktopHelper.Video
            property bool isSelected: model.isSelected
            
            Component.onCompleted: {
                if (index < 5) { // Only log first 5 to avoid spam
                     console.log("[QML_DEBUG] Index:", index, "FilePath:", filePath, 
                                 "Helper:", desktopHelper, "Type:", fileType, 
                                 "IsVideo:", isVideo)
                }
            }
            
            Image {
                anchors.fill: parent
                anchors.margins: 1
                sourceSize: Qt.size(root.loadingResolution, root.loadingResolution)
                source: filePath ? "image://async/" + filePath + "?idx=" + index : ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                
                property real startTime: 0
                onSourceChanged: startTime = new Date().getTime()
                
                onStatusChanged: {
                    if (status === Image.Ready) {
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

                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 4
                    width: 28
                    height: 16
                    color: "#cc000000" // Darker background
                    radius: 3
                    
                    // Direct model access is safer sometimes in complex delegates
                    visible: model.isRaw === true
                    
                    Text {
                        anchors.centerIn: parent
                        text: "RAW"
                        color: "#FF4444" // Bright Red text for visibility check
                        font.pixelSize: 10
                        font.bold: true
                    }
                }
                
                // Selection Visuals
                Rectangle {
                    anchors.fill: parent
                    color: isSelected ? "#440078D7" : "transparent"
                    border.color: "#0078D7"
                    border.width: Math.max(1, Math.min(4, parent.width * 0.05))
                    visible: isSelected
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
                            model.isSelected = !isSelected
                        }
                    }
                }
                
                // Windows Explorer Single File Focus / Caret Highlight Box
                Rectangle {
                    anchors.fill: parent
                    color: index === gridView.currentIndex ? "#443B82F6" : (isSelected ? "#440078D7" : "transparent")
                    border.color: index === gridView.currentIndex ? "#38BDF8" : (isSelected ? "#0078D7" : "transparent")
                    border.width: index === gridView.currentIndex ? 3 : (isSelected ? 2 : 0)
                    radius: 2
                    z: 15
                    visible: index === gridView.currentIndex || isSelected

                    // High-contrast inner white ring
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 1
                        color: "transparent"
                        border.color: "#FFFFFF"
                        border.width: 1
                        radius: 1
                        opacity: 0.6
                        visible: index === gridView.currentIndex
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        gridView.currentIndex = index
                        if (root.model.selectedCount > 0) {
                            model.isSelected = !isSelected
                        } else {
                            root.imageClicked(index)
                        }
                    }
                    onPressAndHold: {
                        gridView.currentIndex = index
                        if (!isSelected) {
                            model.isSelected = true
                        }
                    }
                }
            }
        }
    }
    
    // Pinch Handler for Smooth Zoom
    PinchHandler {
        target: null
        
        onActiveChanged: {
            if (active) {
                root.startPinchGridResolution = appSettings.gridResolution
                gridView.interactive = false
                
                // Capture target
                var item = gridView.itemAt(centroid.position.x + gridView.contentX, centroid.position.y + gridView.contentY)
                if (item) {
                    // Find index? GridView doesn't expose index of item easily?
                    // We can calculate it if we know the layout.
                    // But itemAt returns the delegate.
                    // We can add property index to delegate.
                    // But standard delegate has 'index' property.
                    // root.zoomTargetIndex = item.index // This might work if item is the delegate
                }
                
            } else {
                var newSize = root.startPinchGridResolution * root.currentScale
                newSize = Math.max(20, Math.min(newSize, 400))
                
                root.currentScale = 1.0
                appSettings.gridResolution = newSize
                
                gridView.interactive = true
            }
        }
        
        onScaleChanged: (delta) => {
            var nextScale = root.currentScale * delta
            if (nextScale > 0.5 && nextScale < 3.0) {
                root.currentScale = nextScale
                // Update cell size immediately for smooth flow?
                // If we update cellWidth immediately, GridView will reflow continuously.
                // This is "Tile View" behavior!
                appSettings.gridResolution = root.startPinchGridResolution * root.currentScale
            }
        }
    }
    
    // Mouse Wheel Zoom
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        propagateComposedEvents: true
        
        onWheel: (wheel) => {
            if (wheel.modifiers & Qt.ControlModifier) {
                var oldSize = appSettings.gridResolution
                var newSize = oldSize
                
                if (wheel.angleDelta.y > 0) {
                    newSize = Math.min(oldSize * 1.2, 400)
                } else {
                    newSize = Math.max(oldSize / 1.2, 20)
                }
                
                if (newSize !== oldSize) {
                    appSettings.gridResolution = newSize
                }
                wheel.accepted = true
            } else {
                wheel.accepted = false
            }
        }
    }
    
    Text {
        anchors.centerIn: parent
        text: "No images found."
        color: "#888"
        visible: root.model ? (root.model.count === 0 && !root.model.isLoading) : false
        font.pixelSize: 18
        
        onVisibleChanged: console.log("[QML_DEBUG] 'No images found' visible:", visible, "Count:", root.model ? root.model.count : 0)
    }

    // Floating Date Header
    Rectangle {
        id: sectionHeader
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 10
        width: headerText.contentWidth + 30
        height: 30
        color: "#AA000000"
        radius: 15
        visible: root.currentSectionLabel !== "" && (root.model ? root.model.count > 0 : false)
        z: 10
        
        Text {
            id: headerText
            anchors.centerIn: parent
            text: root.currentSectionLabel
            color: "white"
            font.bold: true
            font.pixelSize: 14
        }
    }
}
