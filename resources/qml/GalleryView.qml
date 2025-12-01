import QtQuick
import QtQuick.Controls
import SamsungGallery 1.0

Item {
    id: root
    signal imageClicked(int index)
    signal imageLoaded(int timeMs)

    function scanFolder(path) {
        imageModel.scanDirectory(path)
    }

    ImageModel {
        id: imageModel
    }
    property alias model: imageModel

    // UI Grid Size (Zoom level)
    property real uiThumbnailSize: appSettings.gridSize
    
    // Loading Resolution (Quality/Performance setting)
    property int loadingResolution: appSettings.thumbnailSize
    
    // Dynamic Section Role based on Zoom Level
    property string currentSectionRole: {
        if (uiThumbnailSize < 80) return "sectionYear"
        if (uiThumbnailSize < 150) return "sectionMonth"
        return "sectionDay"
    }

    GridView {
        id: grid
        anchors.fill: parent
        cellWidth: uiThumbnailSize
        cellHeight: uiThumbnailSize
        model: imageModel
        clip: true
        
        // Increased cacheBuffer to utilize available memory and improve scrolling performance
        cacheBuffer: cellHeight * 10
        
        // Dynamic Section Headers (Semantic Zoom) - TEMPORARILY DISABLED FOR STABILITY
        // section.property: root.currentSectionRole
        // section.criteria: ViewSection.FullString
        // section.delegate: Component {
        //     Rectangle {
        //         width: grid.width
        //         height: 40
        //         color: "#000000" // Solid background for readability
        //         opacity: 0.9
        //         z: 2 
        //         
        //         Text {
        //             anchors.left: parent.left
        //             anchors.leftMargin: 15
        //             anchors.verticalCenter: parent.verticalCenter
        //             text: section
        //             color: "white"
        //             font.bold: true
        //             font.pixelSize: 18
        //         }
        //         
        //         Rectangle {
        //             anchors.bottom: parent.bottom
        //             width: parent.width
        //             height: 1
        //             color: "#333"
        //         }
        //     }
        // }
        
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
            
            // Case-insensitive check
            property var fileExt: model.filePath.split('.').pop().toLowerCase()
            property bool isVideo: fileExt === "mp4" || fileExt === "mkv" || fileExt === "avi" || fileExt === "mov"

            Image {
                id: img
                anchors.fill: parent
                anchors.margins: 1
                // Use custom async provider for optimized thumbnail loading
                source: isVideo ? "" : "image://async/" + model.filePath 
                sourceSize.width: root.loadingResolution
                sourceSize.height: root.loadingResolution
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                mipmap: true // Enable GPU mipmapping
                
                // Video Placeholder
                Rectangle {
                    anchors.fill: parent
                    color: "#222"
                    visible: isVideo
                    
                    Text {
                        anchors.centerIn: parent
                        text: "▶️"
                        font.pixelSize: parent.width * 0.4
                        color: "white"
                    }
                    
                    Text {
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottomMargin: 5
                        text: "Video"
                        color: "#aaa"
                        font.pixelSize: 12
                    }
                }
                
                property bool hasError: status === Image.Error
                property real loadStartTime: 0
                
                Component.onCompleted: loadStartTime = new Date().getTime()
                
                property bool hasReported: false
                
                onStatusChanged: {
                    if (status === Image.Ready && !hasReported) {
                        hasReported = true
                        if (loadStartTime === 0) {
                            // Loaded instantly (cached) or before Component.onCompleted
                            root.imageLoaded(0)
                        } else {
                            var timeTaken = new Date().getTime() - loadStartTime
                            root.imageLoaded(timeTaken)
                        }
                    }
                }
                
                Rectangle {
                    anchors.fill: parent
                    color: "#333"
                    visible: !isVideo && (img.hasError || img.status === Image.Loading)
                    
                    Text {
                        anchors.centerIn: parent
                        text: img.hasError ? "⚠️" : "..."
                        color: "#888"
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.imageClicked(index) // Pass index
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
                var oldSize = appSettings.gridSize
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
                    appSettings.gridSize = newSize
                    
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
            var newSize = appSettings.gridSize * (1 + (delta - 1) * 0.5) // Dampen sensitivity
            appSettings.gridSize = Math.max(40, Math.min(newSize, 400))
        }
    }
    
    Text {
        anchors.centerIn: parent
        text: "No images found.\nCheck folder permissions or select a different folder."
        color: "#888"
        horizontalAlignment: Text.AlignHCenter
        visible: imageModel.count === 0
        font.pixelSize: 18
    }
}
