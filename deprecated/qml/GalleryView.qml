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

    function findChildGridView() {
        return grid
    }

    ImageModel {
        id: imageModel
    }
    property alias model: imageModel

    // UI Grid Size (Zoom level)
    property real uiThumbnailSize: appSettings.gridSize
    
    // Loading Resolution (Quality/Performance setting)
    property int loadingResolution: appSettings.thumbnailSize
    
    // Timer to debounce viewport updates (Settle Strategy)
    Timer {
        id: updateTimer
        interval: 150 
        repeat: false
        onTriggered: grid.updateVisibleRange()
    }

    property real lastRequestedY: 0

    GridView {
        id: grid
        anchors.fill: parent
        cellWidth: uiThumbnailSize
        cellHeight: uiThumbnailSize
        model: imageModel
        clip: true
        
        cacheBuffer: cellHeight * 4
        
        onMovingChanged: if (!moving) updateTimer.restart()
        onFlickingChanged: if (!flicking) updateTimer.restart()

        function updateVisibleRange() {
            if (!imageModel || grid.count === 0) return;
            if (grid.width <= 0 || grid.cellWidth <= 0) return;

            let cols = Math.floor(grid.width / grid.cellWidth);
            if (cols < 1) cols = 1;

            let firstRow = Math.floor(grid.contentY / grid.cellHeight);
            let lastRow = Math.ceil((grid.contentY + grid.height) / grid.cellHeight);
            
            let firstVisible = firstRow * cols;
            let lastVisible = (lastRow * cols) + cols;

            // Update C++ model range
            imageModel.visibleStartIndex = Math.max(0, firstVisible);
            imageModel.visibleEndIndex = Math.min(grid.count - 1, lastVisible);
            root.lastRequestedY = grid.contentY
        }

        onContentYChanged: {
            if (Math.abs(contentY - root.lastRequestedY) > cellHeight * 3) {
                updateVisibleRange()
            }
            updateTimer.restart()
        }
        onHeightChanged: updateTimer.restart()
        onWidthChanged: updateTimer.restart()

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
            active: true
        }

        delegate: Item {
            width: grid.cellWidth
            height: grid.cellHeight
            
            property var fileExt: model.filePath ? model.filePath.split('.').pop().toLowerCase() : ""
            property bool isVideo: fileExt === "mp4" || fileExt === "mkv" || fileExt === "avi" || fileExt === "mov"

            Image {
                id: img
                anchors.fill: parent
                anchors.margins: 1
                source: model.filePath ? "image://async/" + model.filePath : ""
                sourceSize.width: root.loadingResolution
                sourceSize.height: root.loadingResolution
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                mipmap: true 
                
                Rectangle {
                    anchors.fill: parent; color: "#222"; visible: isVideo
                    Text { anchors.centerIn: parent; text: "▶️"; font.pixelSize: parent.width * 0.4; color: "white" }
                }
                
                property bool hasError: status === Image.Error
                property real loadStartTime: 0
                Component.onCompleted: loadStartTime = new Date().getTime()
                property bool hasReported: false
                
                onStatusChanged: {
                    if (status === Image.Ready && !hasReported) {
                        hasReported = true
                        var timeTaken = new Date().getTime() - (loadStartTime || new Date().getTime())
                        root.imageLoaded(timeTaken)
                    }
                }
                
                Rectangle {
                    anchors.fill: parent; color: "#333"
                    visible: !isVideo && (img.hasError || img.status === Image.Loading)
                    Text { anchors.centerIn: parent; text: img.hasError ? "⚠️" : "..."; color: "#888" }
                }

                MouseArea {
                    anchors.fill: parent; onClicked: root.imageClicked(index)
                }
            }
        }
    }

    // Zoom Overlay
    MouseArea {
        anchors.fill: parent; acceptedButtons: Qt.NoButton; propagateComposedEvents: true
        onWheel: (wheel) => {
            if (wheel.modifiers & Qt.ControlModifier) {
                var oldSize = appSettings.gridSize
                var newSize = oldSize
                if (wheel.angleDelta.y > 0) newSize = Math.min(oldSize + 20, 400)
                else newSize = Math.max(oldSize - 20, 40)
                
                if (newSize !== oldSize) {
                    appSettings.gridSize = newSize
                    updateTimer.restart()
                }
                wheel.accepted = true
            }
        }
    }
    
    PinchHandler {
        target: grid
        onScaleChanged: (delta) => {
            var newSize = appSettings.gridSize * (1 + (delta - 1) * 0.5)
            appSettings.gridSize = Math.max(40, Math.min(newSize, 400))
            updateTimer.restart()
        }
    }
}