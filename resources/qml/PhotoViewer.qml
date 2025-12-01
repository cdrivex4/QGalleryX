import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Item {
    id: root
    property var model: null
    property int currentIndex: -1
    property bool isEditing: false
    property bool controlsVisible: true
    
    signal backClicked()
    signal imageLoaded(int timeMs)
    
    visible: false
    
    property var startTime: 0
    
    onVisibleChanged: {
        if (visible && currentIndex >= 0) {
            listView.positionViewAtIndex(currentIndex, ListView.SnapPosition)
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        
        // Block all mouse events from propagating to underlying layers
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onWheel: (wheel) => wheel.accepted = true
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        // Disable swipe when editing OR when zoomed in
        interactive: !root.isEditing && (currentItem && currentItem.children[0].zoom === 1.0)
        
        orientation: ListView.Horizontal
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange
        highlightMoveDuration: 0
        highlightResizeDuration: 0
        cacheBuffer: width // Preload next/prev image
        
        model: root.model
        currentIndex: root.currentIndex
        
        onCurrentIndexChanged: {
            if (moving) { // Only update root if moved by user interaction
                root.currentIndex = currentIndex
            }
        }
        
        // Sync with root index changes (external navigation)
        Connections {
            target: root
            function onCurrentIndexChanged() {
                if (listView.currentIndex !== root.currentIndex) {
                    listView.positionViewAtIndex(root.currentIndex, ListView.SnapPosition)
                }
            }
        }
        
        delegate: Item {
            width: listView.width
            height: listView.height
            
            property string filePath: model.filePath
            property string fileName: model.fileName
            property var fileExt: filePath.split('.').pop().toLowerCase()
            property bool isVideo: fileExt === "mp4" || fileExt === "mkv" || fileExt === "avi" || fileExt === "mov"
            
            // Unified Zoom Function
            function zoomAt(center, step) {
                var newZoom = Math.max(1.0, Math.min(flickable.zoom + step, 10.0))
                if (newZoom === flickable.zoom) return;

                var contentPos = flickable.mapToItem(contentItem, center.x, center.y)
                var relX = contentPos.x / contentItem.width
                var relY = contentPos.y / contentItem.height

                flickable.zoom = newZoom

                var newW = Math.max(flickable.width, flickable.width * newZoom)
                var newH = Math.max(flickable.height, flickable.height * newZoom)

                flickable.contentX = (relX * newW) - center.x
                flickable.contentY = (relY * newH) - center.y
                flickable.returnToBounds()
            }

            Flickable {
                id: flickable
                anchors.fill: parent
                contentWidth: contentItem.width
                contentHeight: contentItem.height
                clip: true
                visible: !isVideo
                
                property real zoom: 1.0
                
                // Reset zoom when index changes
                Connections {
                    target: listView
                    function onCurrentIndexChanged() { flickable.zoom = 1.0 }
                }

                Item {
                    id: contentItem
                    width: Math.max(flickable.width, img.width)
                    height: Math.max(flickable.height, img.height)
                    transformOrigin: Item.Center

                    Image {
                        id: img
                        source: (!isVideo && filePath) ? "file:///" + filePath : ""
                        
                        // Bind size to zoom
                        width: flickable.width * flickable.zoom
                        height: flickable.height * flickable.zoom
                        
                        anchors.centerIn: parent
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        cache: true
                        mipmap: true 
                        autoTransform: true
                        
                        onSourceChanged: root.startTime = new Date().getTime()
                        onStatusChanged: {
                            if (status === Image.Ready) {
                                var endTime = new Date().getTime()
                                root.imageLoaded(endTime - root.startTime)
                            }
                        }
                    }
                }
            }

            // MouseArea for Wheel Zoom (Overlay)
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton // Pass clicks to Flickable/TapHandler
                onWheel: (wheel) => {
                    var step = wheel.angleDelta.y > 0 ? 0.5 : -0.5
                    zoomAt(Qt.point(wheel.x, wheel.y), step)
                    wheel.accepted = true
                }
            }

            // Input Handlers (Overlay)
            TapHandler {
                onTapped: (eventPoint) => {
                    if (eventPoint.tapCount === 2) {
                        // Double Tap: Zoom
                        if (flickable.zoom > 1.0) {
                            flickable.zoom = 1.0
                            flickable.returnToBounds()
                        } else {
                            zoomAt(eventPoint.position, 2.0) // Zoom in by +2.0 (to 3.0)
                        }
                    } else {
                        // Single Tap: Toggle Controls
                        root.controlsVisible = !root.controlsVisible
                    }
                }
            }
            
            PinchHandler {
                target: null 
                onActiveChanged: {
                    if (!active) flickable.returnToBounds()
                }
                onScaleChanged: (delta) => {
                    var newZoom = Math.max(1.0, Math.min(flickable.zoom * delta, 10.0))
                    var center = point.position // Viewport coords
                    
                    var contentPos = flickable.mapToItem(contentItem, center.x, center.y)
                    var relX = contentPos.x / contentItem.width
                    var relY = contentPos.y / contentItem.height
                    
                    flickable.zoom = newZoom
                    
                    var newW = Math.max(flickable.width, flickable.width * newZoom)
                    var newH = Math.max(flickable.height, flickable.height * newZoom)
                    
                    flickable.contentX = (relX * newW) - center.x
                    flickable.contentY = (relY * newH) - center.y
                }
            }
            
            // Video Player Placeholder
            Item {
                anchors.fill: parent
                visible: isVideo
                Text {
                    anchors.centerIn: parent
                    text: "Video: " + fileName
                    color: "white"
                    font.pixelSize: 24
                }
                TapHandler {
                    onTapped: root.controlsVisible = !root.controlsVisible
                }
            }
        }
    }
    
    // Edit Overlay
    Rectangle {
        id: editOverlay
        anchors.fill: parent
        color: "#AA000000"
        visible: root.isEditing
        z: 20
        
        Image {
            id: editImg
            anchors.centerIn: parent
            width: parent.width * 0.8
            height: parent.height * 0.8
            fillMode: Image.PreserveAspectFit
            source: (root.model && root.currentIndex >= 0) ? "file:///" + root.model.data(root.model.index(root.currentIndex, 0), 257) : "" // FilePathRole
        }
        
        // Crop Handles (Visual)
        Rectangle {
            anchors.fill: editImg
            border.color: "white"
            border.width: 2
            color: "transparent"
            
            Rectangle { width: 20; height: 20; color: "white"; anchors.left: parent.left; anchors.top: parent.top }
            Rectangle { width: 20; height: 20; color: "white"; anchors.right: parent.right; anchors.top: parent.top }
            Rectangle { width: 20; height: 20; color: "white"; anchors.left: parent.left; anchors.bottom: parent.bottom }
            Rectangle { width: 20; height: 20; color: "white"; anchors.right: parent.right; anchors.bottom: parent.bottom }
        }
        
        RowLayout {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.margins: 20
            spacing: 20
            
            Button { 
                text: "Save"
                onClicked: {
                    var rect = Qt.rect(0.25, 0.25, 0.5, 0.5) 
                    if (root.model.cropImage(root.currentIndex, rect)) {
                        root.isEditing = false
                        console.log("Image cropped and saved")
                    } else {
                        console.log("Failed to crop image")
                    }
                }
            }
            Button { text: "Cancel"; onClicked: root.isEditing = false }
        }
    }

    // Navigation Buttons
    Rectangle {
        width: 50
        height: 50
        radius: 25
        color: "#80000000"
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 20
        visible: root.controlsVisible && !root.isEditing && root.currentIndex > 0
        
        Text {
            anchors.centerIn: parent
            text: "‹"
            color: "white"
            font.pixelSize: 40
        }
        MouseArea { anchors.fill: parent; onClicked: root.currentIndex-- }
    }

    Rectangle {
        width: 50
        height: 50
        radius: 25
        color: "#80000000"
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 20
        visible: root.controlsVisible && !root.isEditing && root.model && root.currentIndex < root.model.rowCount() - 1
        
        Text {
            anchors.centerIn: parent
            text: "›"
            color: "white"
            font.pixelSize: 40
        }
        MouseArea { anchors.fill: parent; onClicked: root.currentIndex++ }
    }

    // Keyboard Navigation
    focus: true
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Left) {
            if (currentIndex > 0) currentIndex--
            event.accepted = true
        } else if (event.key === Qt.Key_Right) {
            if (currentIndex < model.rowCount() - 1) currentIndex++
            event.accepted = true
        } else if (event.key === Qt.Key_Escape) {
            if (root.isEditing) {
                root.isEditing = false
            } else {
                root.backClicked()
            }
            event.accepted = true
        } else if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
             // Zoom In (Center)
             if (listView.currentItem) {
                 listView.currentItem.zoomAt(Qt.point(listView.width/2, listView.height/2), 0.5)
             }
             event.accepted = true
        } else if (event.key === Qt.Key_Minus) {
             // Zoom Out (Center)
             if (listView.currentItem) {
                 listView.currentItem.zoomAt(Qt.point(listView.width/2, listView.height/2), -0.5)
             }
             event.accepted = true
        }
    }

    // Top Bar
    Rectangle {
        id: topBar
        width: parent.width
        height: 60
        color: "#80000000"
        anchors.top: parent.top
        visible: controlsVisible && !root.isEditing
        z: 1

        Button {
            text: "Back"
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 10
            onClicked: root.backClicked()
        }
        
        Button {
            text: "Edit"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 10
            onClicked: root.isEditing = true
        }
    }
}
