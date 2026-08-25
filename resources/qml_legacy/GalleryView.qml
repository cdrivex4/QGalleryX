import QtQuick
import QtQuick.Controls
import QGalleryX 1.0

Item {
    id: root
    signal imageClicked(int index)
    signal imageLoaded(int timeMs)

    function scanFolder(path) {
        imageModel.scanDirectory(path)
    }

    ImageModel {
        id: imageModel
        loadingResolution: root.loadingResolution
    }
    property alias model: imageModel

    // UI Grid Size (Zoom level)
    property real uiThumbnailSize: appSettings.gridResolution
    
    // Loading Resolution (Quality/Performance setting)
    property int loadingResolution: appSettings.thumbnailSize
    
    // Dynamically downsample the loaded texture to match the UI grid size, saving massive VRAM
    property int effectiveLoadingResolution: {
        if (uiThumbnailSize <= 48) return 64
        if (uiThumbnailSize <= 128) return 128
        return 256
    }
    
    // Dynamic Section Role based on Zoom Level
    property string currentSectionRole: {
        if (uiThumbnailSize < 80) return "sectionYear"
        if (uiThumbnailSize < 150) return "sectionMonth"
        return "sectionDay"
    }

    GridView {
        id: grid
        anchors.fill: parent
        clip: true
        focus: true
        keyNavigationEnabled: false
        
        Component.onCompleted: grid.forceActiveFocus()

        Keys.onPressed: (event) => {
            var cols = Math.max(1, Math.floor(grid.width / grid.cellWidth))
            var rowsPerPage = Math.max(1, Math.floor(grid.height / grid.cellHeight))
            var pageStep = rowsPerPage * cols
            var count = grid.count

            if (count === 0) return
            if (grid.currentIndex < 0) grid.currentIndex = 0

            if (event.key === Qt.Key_Left) {
                grid.currentIndex = Math.max(0, grid.currentIndex - 1)
                grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_Right) {
                grid.currentIndex = Math.min(count - 1, grid.currentIndex + 1)
                grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_Up) {
                if (grid.currentIndex < cols) {
                    searchField.forceActiveFocus()
                } else {
                    grid.currentIndex = Math.max(0, grid.currentIndex - cols)
                    grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                }
                event.accepted = true
            } else if (event.key === Qt.Key_Down) {
                if (grid.currentIndex >= count - cols) {
                    bottomBar.focusTab(0)
                } else {
                    grid.currentIndex = Math.min(count - 1, grid.currentIndex + cols)
                    grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                }
                event.accepted = true
            } else if (event.key === Qt.Key_PageUp) {
                grid.currentIndex = Math.max(0, grid.currentIndex - pageStep)
                grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_PageDown) {
                grid.currentIndex = Math.min(count - 1, grid.currentIndex + pageStep)
                grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_Home) {
                grid.currentIndex = 0
                grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_End) {
                grid.currentIndex = count - 1
                grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                event.accepted = true
            } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                if (grid.currentIndex >= 0 && grid.currentIndex < count) {
                    root.imageClicked(grid.currentIndex)
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

        property real lastContentY: 0
        property real lastViewportY: 0
        function updateViewportNow() {
            var sIdx = indexAt(width / 2, contentY)
            var eIdx = indexAt(width / 2, contentY + height)

            if (typeof viewportGovernor !== "undefined" && sIdx !== -1 && eIdx !== -1 && root.model) {
                viewportGovernor.updateViewport(sIdx, eIdx, root.model.count, contentY - lastContentY)
                root.model.visibleStartIndex = sIdx
                root.model.visibleEndIndex = eIdx
            }
        }

        onContentYChanged: {
            if (Math.abs(contentY - lastViewportY) >= cellHeight * 0.5) {
                lastViewportY = contentY
                updateViewportNow()
            }
            lastContentY = contentY
        }
        
        onMovementEnded: updateViewportNow()
        onFlickEnded: updateViewportNow()
        
        onCountChanged: {
            Qt.callLater(updateViewportNow)
        }
        
        onHeightChanged: Qt.callLater(updateViewportNow)
        onWidthChanged: Qt.callLater(updateViewportNow)
        cellWidth: uiThumbnailSize
        cellHeight: uiThumbnailSize
        model: imageModel
        
        // Qt 6 Delegate Pooling: Recycles QML items instead of allocating/destroying on scroll
        reuseItems: true
        
        // Lightweight bounded cacheBuffer for smooth scrolling
        cacheBuffer: Math.min(height, 400)
        
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
            active: true
            contentItem: Rectangle {
                implicitWidth: 6
                implicitHeight: 100
                radius: 3
                color: "#888"
            }
        }

        delegate: Item {
            width: grid.cellWidth
            height: grid.cellHeight
            
            // Direct C++ Role Access (Zero JS string splitting overhead)
            readonly property bool isVideo: model.isVideo !== undefined ? model.isVideo : false

            Image {
                id: img
                anchors.fill: parent
                anchors.margins: 1
                source: "image://async/" + model.filePath + "?idx=" + index
                sourceSize.width: root.effectiveLoadingResolution
                sourceSize.height: root.effectiveLoadingResolution
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                mipmap: false // Avoid GPU mipmap generation overhead on dynamic tiles
                
                // Video Play Icon (Overlay on top of actual thumbnail)
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    visible: isVideo
                    
                    Rectangle {
                        anchors.centerIn: parent
                        width: 30
                        height: 30
                        radius: 15
                        color: "#88000000"
                        
                        Text {
                            anchors.centerIn: parent
                            text: "▶️"
                            font.pixelSize: 15
                            color: "white"
                        }
                    }
                }
                
                property bool hasError: status === Image.Error
                
                Rectangle {
                    anchors.fill: parent
                    color: "#333"
                    visible: !isVideo && (img.hasError || img.status === Image.Loading)
                    
                    Text {
                        anchors.centerIn: parent
                        text: img.hasError ? "⚠️" : "..."
                        color: "white"
                    }
                }

                // Windows Explorer Single File Focus / Caret Highlight Box
                Rectangle {
                    anchors.fill: parent
                    color: index === grid.currentIndex ? "#443B82F6" : (isSelected ? "#440078D7" : "transparent")
                    border.color: index === grid.currentIndex ? "#38BDF8" : (isSelected ? "#0078D7" : "transparent")
                    border.width: index === grid.currentIndex ? 3 : (isSelected ? 2 : 0)
                    radius: 2
                    z: 15
                    visible: index === grid.currentIndex || isSelected

                    // High-contrast inner white ring
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 1
                        color: "transparent"
                        border.color: "#FFFFFF"
                        border.width: 1
                        radius: 1
                        opacity: 0.6
                        visible: index === grid.currentIndex
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        grid.currentIndex = index
                        root.imageClicked(index)
                    }
                }
            }
        }
    }

    // Zoom Grid with Ctrl + Wheel
    // We place this inside a MouseArea that fills the parent to ensure we catch events
    MouseArea {
        anchors.fill: parent
        propagateComposedEvents: true
        hoverEnabled: true
        acceptedButtons: Qt.NoButton // Don't block clicks
        
        onWheel: (wheel) => {
            if (wheel.modifiers & Qt.ControlModifier) {
                // 1. Identify Item Under Mouse
                var oldSize = appSettings.gridResolution
                var oldCols = Math.floor(grid.width / oldSize)
                if (oldCols < 1) oldCols = 1
                
                var mouseContentY = grid.contentY + wheel.y
                var row = Math.floor(mouseContentY / oldSize)
                var col = Math.floor(wheel.x / oldSize)
                if (col >= oldCols) col = oldCols - 1
                
                var index = row * oldCols + col
                if (index < 0) index = 0
                if (index >= imageModel.count) index = imageModel.count - 1
                
                // Calculate where the mouse is relative to the top of this item
                var itemTopY = row * oldSize
                var relativeMouseY = mouseContentY - itemTopY
                
                // 2. Apply Zoom
                var newSize = oldSize
                if (wheel.angleDelta.y > 0) {
                    newSize = Math.min(oldSize + 20, 400)
                } else {
                    newSize = Math.max(oldSize - 20, 40)
                }
                
                if (newSize !== oldSize) {
                    appSettings.gridResolution = newSize
                    
                    // 3. Calculate New Position
                    // We use a Timer to ensure the GridView has finished its layout update
                    // before we force the contentY. Qt.callLater might be too early.
                    zoomRestoreTimer.targetContentY = (() => {
                        var newCols = Math.floor(grid.width / newSize)
                        if (newCols < 1) newCols = 1
                        
                        var newRow = Math.floor(index / newCols)
                        var newItemTopY = newRow * newSize
                        
                        return newItemTopY + relativeMouseY - wheel.y
                    })()
                    zoomRestoreTimer.restart()
                }
                
                wheel.accepted = true
            } else {
                wheel.accepted = false
            }
        }
    }
    
    Timer {
        id: zoomRestoreTimer
        interval: 20 // Slightly increased delay for safety
        repeat: false
        property real targetContentY: 0
        onTriggered: {
            grid.contentY = Math.max(0, targetContentY)
        }
    }
    
    // Pinch to Zoom (Touchpad/Touchscreen)
    PinchHandler {
        target: grid
        onActiveChanged: {
            if (active) {
                // Start pinch
            }
        }
        onScaleChanged: (delta) => {
            var newSize = appSettings.gridResolution * (1 + (delta - 1) * 0.5) // Dampen sensitivity
            appSettings.gridResolution = Math.max(32, Math.min(newSize, 256))
        }
    }
    
    Text {
        anchors.centerIn: parent
        text: "No images found.\nCheck folder permissions or select a different folder."
        color: "#888"
        horizontalAlignment: Text.AlignHCenter
        visible: imageModel ? (imageModel.count === 0 && !imageModel.isLoading) : false
        font.pixelSize: 18
    }
}
