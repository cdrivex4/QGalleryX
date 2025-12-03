import QtQuick
import QtQuick.Controls
import SamsungGallery 1.0

Item {
    id: root
    
    // Signals
    signal imageClicked(int index)
    signal imageLoaded(int timeMs)
    
    // Properties
    property real uiThumbnailSize: appSettings ? appSettings.gridSize : 100
    property int loadingResolution: appSettings ? appSettings.thumbnailSize : 200
    property string folderPath: ""
    
    onFolderPathChanged: {
        if (folderPath !== "") scanFolder(folderPath)
    }
    
    // Helper to scan folder
    function scanFolder(path) {
        imageModel.scanDirectory(path)
    }

    // Models
    ImageModel {
        id: imageModel
    }
    property alias model: imageModel

    // Grouping Mode: 0=Auto, 1=Day, 2=Week, 3=Month, 4=Year
    property int groupingMode: 0

    GroupedProxyModel {
        id: proxyModel
        sourceModel: imageModel
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
    property real startPinchGridSize: 100
    property int zoomTargetSourceIndex: -1
    property real zoomTargetRelativeY: 0.5 // 0 to 1 (relative to viewport height)

    // Helper to find source index at screen coordinates
    function getSourceIndexAt(screenX, screenY) {
        // Map screen coordinates to content coordinates
        var contentX = screenX + listView.contentX
        var contentY = screenY + listView.contentY
        
        var item = listView.itemAt(contentX, contentY)
        if (item) {
            // Check if it's a row delegate (has rowStartIndex property)
            // Note: item is the Loader's loaded item (Rectangle or Item)
            // But itemAt returns the Delegate (Loader).
            // So we check item.item (the loaded component)
            
            var loadedItem = item.item
            if (loadedItem && loadedItem.hasOwnProperty("rowStartIndex")) {
                // It's a row
                var rowLocalX = contentX - item.x
                var col = Math.floor(rowLocalX / loadedItem.itemSize)
                // Clamp col
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
            
            // Visual Scaling
            scale: root.currentScale
            transformOrigin: Item.Center // Will be updated by PinchHandler
            
            // Performance
            cacheBuffer: 1000
            
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
                root.startPinchGridSize = appSettings.gridSize
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
                var newSize = root.startPinchGridSize * root.currentScale
                newSize = Math.max(20, Math.min(newSize, 400)) // Min 20px
                
                // Reset Scale
                root.currentScale = 1.0
                
                // Apply new size (this will trigger re-layout)
                appSettings.gridSize = newSize
                
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
                
                var oldSize = appSettings.gridSize
                var newSize = oldSize
                
                if (wheel.angleDelta.y > 0) {
                    newSize = Math.min(oldSize * 1.2, 400)
                } else {
                    newSize = Math.max(oldSize / 1.2, 20) // Min 20px
                }
                
                if (newSize !== oldSize) {
                    appSettings.gridSize = newSize
                    
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
                        
                        // Fetch data
                        property string filePath: root.model.data(root.model.index(sourceIndex, 0), ImageModel.FilePathRole)
                        property var fileExt: filePath.split('.').pop().toLowerCase()
                        property bool isVideo: fileExt === "mp4" || fileExt === "mkv" || fileExt === "avi"
                        
                        Image {
                            id: img
                            anchors.fill: parent
                            anchors.margins: 1
                            source: (filePath && !isVideo) ? "image://async/" + filePath : ""
                            sourceSize.width: root.loadingResolution
                            sourceSize.height: root.loadingResolution
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
                                color: "#222"
                                visible: isVideo
                                Text {
                                    anchors.centerIn: parent
                                    text: "▶️"
                                    color: "white"
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.imageClicked(sourceIndex)
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
        visible: imageModel.count === 0
        font.pixelSize: 18
    }
}
