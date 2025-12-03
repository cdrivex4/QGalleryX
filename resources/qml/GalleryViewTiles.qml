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

    // Zoom State
    property real currentScale: 1.0
    property real startPinchGridSize: 100
    property int zoomTargetIndex: -1
    
    // Main Viewport
    GridView {
        id: gridView
        anchors.fill: parent
        clip: true
        
        model: imageModel
        
        cellWidth: root.uiThumbnailSize
        cellHeight: root.uiThumbnailSize
        
        // Performance
        cacheBuffer: 1000
        
        // ScrollBar
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
            active: true
        }
        
        // Animations
        add: Transition { NumberAnimation { properties: "x,y"; duration: 200 } }
        move: Transition { NumberAnimation { properties: "x,y"; duration: 200 } }
        displaced: Transition { NumberAnimation { properties: "x,y"; duration: 200 } }

        delegate: Item {
            width: gridView.cellWidth
            height: gridView.cellHeight
            
            // Fetch data
            property string filePath: model.filePath
            property var fileExt: filePath.split('.').pop().toLowerCase()
            property bool isVideo: fileExt === "mp4" || fileExt === "mkv" || fileExt === "avi"
            
            Image {
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
                    onClicked: root.imageClicked(index)
                }
            }
        }
    }
    
    // Pinch Handler for Smooth Zoom
    PinchHandler {
        target: null
        
        onActiveChanged: {
            if (active) {
                root.startPinchGridSize = appSettings.gridSize
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
                var newSize = root.startPinchGridSize * root.currentScale
                newSize = Math.max(20, Math.min(newSize, 400))
                
                root.currentScale = 1.0
                appSettings.gridSize = newSize
                
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
                appSettings.gridSize = root.startPinchGridSize * root.currentScale
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
                var oldSize = appSettings.gridSize
                var newSize = oldSize
                
                if (wheel.angleDelta.y > 0) {
                    newSize = Math.min(oldSize * 1.2, 400)
                } else {
                    newSize = Math.max(oldSize / 1.2, 20)
                }
                
                if (newSize !== oldSize) {
                    appSettings.gridSize = newSize
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
        visible: imageModel.count === 0
        font.pixelSize: 18
    }
}
