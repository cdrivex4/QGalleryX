import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: "ScrollBench - Viewport Culling & Frame Budget Test"
    color: "#1e1e1e"

    // Shared properties
    property int gridSize: 20        // UI cell size (zoom level) - Ctrl+Wheel to change
    property int thumbResolution: 192  // Image decode quality (default 192px as per user spec)
    property bool overlayVisible: false  // Performance overlay visibility
    property bool selectionMode: false  // Selection mode state
    property int dragSelectionStart: -1 // Anchor for drag selection

    // FPS counter
    Timer {
        interval: 16
        running: true
        repeat: true
        onTriggered: telemetry.recordFrame()
    }

    // Memory usage update
    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: telemetry.updateMemoryUsage()
    }

    // Share Dialog
    ShareDialog {
        id: shareDialog
    }

    // Main content area (full screen)
    Rectangle {
        anchors.fill: parent
        color: "#2d2d2d"

        GridView {
            id: gridView
            anchors.fill: parent
            cellWidth: root.gridSize
            cellHeight: root.gridSize
            model: imageModel
            clip: true
            interactive: root.dragSelectionStart === -1 // Disable scrolling while dragging selection box

            delegate: Rectangle {
                width: gridView.cellWidth - 4
                height: gridView.cellHeight - 4
                color: "#3d3d3d"
                border.color: "#505050"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 3

                    // Thumbnail
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: model.testColor || "#666666"

                        // Real image
                        Image {
                            anchors.fill: parent
                            source: model.path && !model.path.startsWith("synthetic://") && model.isLoaded ? 
                                   "image://async/" + model.path : ""
                            sourceSize.width: root.thumbResolution
                            sourceSize.height: root.thumbResolution
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: false
                            visible: source !== ""
                        }

                        // Loading indicator
                        BusyIndicator {
                            anchors.centerIn: parent
                            width: 30
                            height: 30
                            running: !model.isLoaded && model.path && !model.path.startsWith("synthetic://")
                            visible: running && gridView.cellWidth > 60
                        }
                    }

                    // Info text (filename at 90px+, index below)
                    Text {
                        Layout.fillWidth: true
                        text: gridView.cellWidth >= 90 ? model.fileName : "#" + model.imageIndex
                        color: "#ffffff"
                        font.pixelSize: Math.max(8, Math.min(12, gridView.cellWidth / 15))
                        elide: Text.ElideMiddle
                        horizontalAlignment: Text.AlignHCenter
                        visible: gridView.cellWidth > 40
                    }
                }

                // Selection overlay (only in selection mode)
                Rectangle {
                    anchors.fill: parent
                    color: model.isSelected ? "#802196F3" : "transparent"
                    border.color: model.isSelected ? "#2196F3" : "transparent"
                    border.width: 3
                    visible: root.selectionMode
                    
                    Text {
                        anchors.centerIn: parent
                        text: "✓"
                        color: "#FFFFFF"
                        font.pixelSize: Math.min(gridView.cellWidth / 3, 48)
                        font.bold: true
                        visible: model.isSelected
                    }
                }

                // Visual Selection Box
                Rectangle {
                    id: selectionBox
                    parent: gridView
                    z: 100 // On top of items
                    visible: root.dragSelectionStart >= 0
                    color: "#332196F3"
                    border.color: "#2196F3"
                    border.width: 1
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: false
                    // Track start position in GridView VIEWPORT coordinates
                    property point dragStartPoint
                    
                    onPressAndHold: {
                        // Map local click (Item coords) to GridView (Viewport coords)
                        let p = mapToItem(gridView, mouse.x, mouse.y)
                        dragStartPoint = p
                        
                        if (!root.selectionMode) {
                            root.selectionMode = true
                            imageModel.toggleSelection(model.imageIndex)
                            root.dragSelectionStart = model.imageIndex
                        } else {
                             root.dragSelectionStart = model.imageIndex
                             imageModel.toggleSelection(model.imageIndex) 
                        }
                    }
                    
                    onPositionChanged: (mouse) => {
                         if (root.dragSelectionStart >= 0) {
                             // Map current mouse (Item coords) to GridView (Viewport coords)
                             let p = mapToItem(gridView, mouse.x, mouse.y)
                             
                             // 1. Update selectionBox geometry (Viewport Coords)
                             let bx = Math.min(dragStartPoint.x, p.x)
                             let by = Math.min(dragStartPoint.y, p.y)
                             let bw = Math.abs(dragStartPoint.x - p.x)
                             let bh = Math.abs(dragStartPoint.y - p.y)
                             
                             selectionBox.x = bx
                             selectionBox.y = by
                             selectionBox.width = bw
                             selectionBox.height = bh
                             
                             // 2. Calculate selection in Content Coords
                             // contentX/Y needs to be added to convert Viewport -> Content
                             let contentRectX = bx + gridView.contentX
                             let contentRectY = by + gridView.contentY
                             
                             let colMin = Math.floor(contentRectX / gridView.cellWidth)
                             let colMax = Math.ceil((contentRectX + bw) / gridView.cellWidth) - 1
                             let rowMin = Math.floor(contentRectY / gridView.cellHeight)
                             let rowMax = Math.ceil((contentRectY + bh) / gridView.cellHeight) - 1
                             
                             let columns = Math.floor(gridView.width / gridView.cellWidth)
                             
                             // 3. Invoke Model
                             if (columns > 0) {
                                imageModel.selectVisualRect(colMin, colMax, rowMin, rowMax, columns)
                             }
                         }
                    }

                    onReleased: {
                         root.dragSelectionStart = -1
                    }
                    
                    onClicked: {
                        if (root.selectionMode) {
                            imageModel.toggleSelection(model.imageIndex)
                        } else {
                            // TODO: Open PhotoViewer
                            console.log("Clicked image:", model.imageIndex)
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
            }

            // Track visible range
            function updateVisibleRange() {
                let firstVisible = indexAt(contentX + 10, contentY + 10) // Top-left with margin
                if (firstVisible < 0) firstVisible = indexAt(contentX + width/2, contentY + 10) // Top-center fallback
                
                // Robust lastVisible calculation
                let lastVisible = indexAt(contentX + width - 10, contentY + height - 10) // Bottom-right
                if (lastVisible < 0) {
                     lastVisible = indexAt(contentX + width/2, contentY + height - 10) // Bottom-center
                }
                if (lastVisible < 0) {
                     lastVisible = indexAt(contentX + 10, contentY + height - 10) // Bottom-left
                }
                
                // Mathematical fallback if probing fails completely (e.g. fast scroll or spacing)
                if (firstVisible >= 0 && lastVisible < 0) {
                     let approxRows = Math.ceil(height / cellHeight)
                     let approxCols = Math.floor(width / cellWidth)
                     // Add buffer rows to be safe
                     let estimatedEnd = firstVisible + (approxRows + 1) * approxCols
                     lastVisible = Math.min(count - 1, estimatedEnd)
                }

                // Apply valid updates
                if (firstVisible >= 0) {
                    imageModel.visibleStartIndex = firstVisible
                }
                
                // Ensure endIndex is at least started
                if (lastVisible >= 0) {
                     // Sanity check: end must be >= start
                     if (imageModel.visibleStartIndex > lastVisible) {
                         lastVisible = imageModel.visibleStartIndex + 20 // minimal fallback
                     }
                     imageModel.visibleEndIndex = lastVisible
                } else if (imageModel.visibleStartIndex >= 0) {
                     // Last ditch: if we have start but no end, assume a page
                     imageModel.visibleEndIndex = Math.min(count - 1, imageModel.visibleStartIndex + 50)
                }
            }

            onContentYChanged: updateVisibleRange()
            onContentXChanged: updateVisibleRange()
            onHeightChanged: updateVisibleRange()
            onWidthChanged: updateVisibleRange()
            onCellWidthChanged: updateVisibleRange() // Update on Zoom
        }

        // Ctrl+Wheel Zoom Handler
        MouseArea {
            anchors.fill: gridView
            acceptedButtons: Qt.NoButton
            propagateComposedEvents: true
            
            onWheel: (wheel) => {
                if (wheel.modifiers & Qt.ControlModifier) {
                    let delta = wheel.angleDelta.y > 0 ? 10 : -10
                    let newSize = Math.max(20, Math.min(400, root.gridSize + delta))
                    root.gridSize = newSize
                    console.log("Grid size changed to:", root.gridSize)
                    wheel.accepted = true
                } else {
                    wheel.accepted = false
                }
            }
        }

        // Pinch Zoom Handler (for touch)
        PinchHandler {
            target: null
            onActiveScaleChanged: {
                let scaleFactor = activeScale
                let delta = (scaleFactor - 1.0) * 50
                let newSize = Math.max(20, Math.min(400, root.gridSize + delta))
                root.gridSize = Math.round(newSize)
            }
        }

        // Info label
        Text {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 20
            color: "#ffffff"
            font.pixelSize: 14
            font.bold: true
            text: "ScrollBench Test Application\n" +
                  imageModel.totalItems + " items | " +
                  "Visible: " + imageModel.visibleStartIndex + "-" + imageModel.visibleEndIndex + "\n" +
                  "Grid: " + root.gridSize + "px | Thumb: " + root.thumbResolution + "px\n" +
                  "Ctrl+Wheel to zoom"
            lineHeight: 1.2
        }

        // Folder Selection Button
        Button {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 20
            text: "📁 Select Folder"
            onClicked: folderDialog.open()
        }

        FolderDialog {
            id: folderDialog
            title: "Select Image Folder"
            onAccepted: {
                var folderPath = selectedFolder.toString()
                
                if (folderPath.startsWith("file:///")) {
                    folderPath = folderPath.substring(8)
                } else if (folderPath.startsWith("file://")) {
                    folderPath = "\\\\" + folderPath.substring(7).replace(/\//g, "\\")
                }
                
                console.log("Loading images from:", folderPath)
                imageModel.clearData()
                imageModel.scanDirectory(folderPath)
            }
        }
    }

    // Sliding Performance Overlay (from left)
    Rectangle {
        id: perfOverlay
        width: 380
        height: parent.height
        x: overlayVisible ? 0 : -width
        y: 0
        z: 100
        color: "#252525"
        border.color: "#404040"
        border.width: 1
        clip: true  // Prevent content spillover

        Behavior on x {
            NumberAnimation {
                duration: 200
                easing.type: Easing.InOutQuad
            }
        }

        PerformanceOverlay {
            anchors.fill: parent
        }
    }

    // Transparent Toggle Button (left edge, middle)
    Rectangle {
        id: toggleButton
        width: 30
        height: 60
        x: overlayVisible ? perfOverlay.width : 0
        y: (parent.height - height) / 2
        z: 101
        color: "#ffffff"
        opacity: toggleButtonArea.containsMouse ? 0.3 : 0.1
        radius: 5

        Behavior on x {
            NumberAnimation {
                duration: 200
                easing.type: Easing.InOutQuad
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }

        Text {
            anchors.centerIn: parent
            text: overlayVisible ? "◀" : "▶"
            color: "#000000"
            font.pixelSize: 16
            font.bold: true
        }

        MouseArea {
            id: toggleButtonArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.overlayVisible = !root.overlayVisible
            cursorShape: Qt.PointingHandCursor
        }
    }

    // Selection Mode Overlay (top bar)
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        color: "#2196F3"
        visible: root.selectionMode
        z: 200
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10
            
            Button {
                text: "✕"
                font.pixelSize: 20
                onClicked: {
                    root.selectionMode = false
                    imageModel.clearSelection()
                }
                Layout.preferredWidth: 40
            }
            
            Text {
                text: imageModel.selectedCount + " selected"
                color: "#FFFFFF"
                font.pixelSize: 16
                font.bold: true
            }
            
            Item { Layout.fillWidth: true }
            
            Button {
                text: "Select All"
                onClicked: imageModel.selectAll()
                Layout.preferredWidth: 90
            }
            
            Button {
                text: "Select None"
                onClicked: imageModel.clearSelection()
                Layout.preferredWidth: 100
            }
            
            Button {
                text: "Invert"
                onClicked: imageModel.invertSelection()
                Layout.preferredWidth: 70
            }
            
            Button {
                text: "⋮"  // Android share icon (vertical ellipsis)
                font.pixelSize: 24
                onClicked: shareDialog.open()
                Layout.preferredWidth: 50
            }
        }
    }
}
