import QtQuick
import QtQuick.Controls

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

    GridView {
        id: grid
        anchors.fill: parent
        cellWidth: settings.gridSize
        cellHeight: settings.gridSize
        model: root.model
        clip: true
        
        // Expose state for debugging
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

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.imageClicked(index)
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