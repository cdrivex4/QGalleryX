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
        console.log("[QML_DEBUG] GalleryViewTiles folderPath changed to:", folderPath)
        if (folderPath !== "") scanFolder(folderPath)
    }
    
    // Helper to scan folder
    function scanFolder(path) {
        console.log("[QML_DEBUG] Scanning folder in ImageModel:", path)
        imageModel.scanDirectory(path)
    }

    // Models
    ImageModel {
        id: imageModel
    }
    property alias model: imageModel

    DesktopHelper {
        id: desktopHelper
    }

    // Zoom State
    property real currentScale: 1.0
    property real startPinchGridSize: 100
    property int zoomTargetIndex: -1
    
    // Timeline State
    property int groupingMode: 1 // 1=Day, 3=Month, 4=Year
    property string currentSectionLabel: ""

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
        
        onContentYChanged: {
            // Calculate center item index to determine current date section
            var index = indexAt(width / 2, contentY + cellHeight / 2)
            if (index === -1) index = indexAt(width / 2, contentY)
            
            if (index !== -1) {
                // Role IDs from ImageModel.h:
                // SectionDayRole = 260
                // SectionMonthRole = 261
                // SectionYearRole = 262
                
                var role = 260; // Day
                if (root.groupingMode === 3) role = 261; // Month
                if (root.groupingMode === 4) role = 262; // Year
                
                var section = imageModel.data(imageModel.index(index, 0), role)
                if (section !== undefined) root.currentSectionLabel = section
            }
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
            property int fileType: desktopHelper ? desktopHelper.getFileType(filePath) : 0
            property bool isVideo: fileType === DesktopHelper.Video
            
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
                source: filePath ? "image://async/" + filePath : ""
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
                    color: "transparent"
                    visible: isVideo
                    
                    Rectangle { 
                         anchors.fill: parent
                         color: "black"
                         opacity: 0.2
                    }
                    Text {
                        anchors.centerIn: parent
                        text: "??????"
                        color: "white"
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
        
        onVisibleChanged: console.log("[QML_DEBUG] 'No images found' visible:", visible, "Count:", imageModel.count)
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
        visible: root.currentSectionLabel !== "" && imageModel.count > 0
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
