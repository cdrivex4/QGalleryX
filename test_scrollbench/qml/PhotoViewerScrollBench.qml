import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import ScrollBenchBackend // Import ScrollBenchBackend

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
        if (visible) {
            forceActiveFocus()
            if (currentIndex >= 0) {
                listView.currentIndex = currentIndex
                listView.positionViewAtIndex(currentIndex, ListView.SnapPosition)
            }
            // Pause background tasks and frame budget when PhotoViewer becomes visible
            desktopHelper.pauseBackgroundTasks();
            if (typeof frameBudget !== "undefined") frameBudget.paused = true;
        } else {
            // Resume both when PhotoViewer becomes invisible
            desktopHelper.resumeBackgroundTasks();
            if (typeof frameBudget !== "undefined") frameBudget.paused = false;
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
                    listView.currentIndex = root.currentIndex
                    listView.positionViewAtIndex(root.currentIndex, ListView.SnapPosition)
                }
            }
        }
        
        delegate: Item {
            width: listView.width
            height: listView.height
            
            property string filePath: model.filePath
            property string fileName: model.fileName
            // Use backend model as single source of truth for file types
            property bool isVideo: model.isVideo !== undefined ? model.isVideo : false
            property bool isRaw: model.isRaw !== undefined ? model.isRaw : false
            property bool isCurrent: index === ListView.view.currentIndex
            onIsCurrentChanged: {
                if (!isCurrent && isVideo) {
                    console.log("Stopping background video:", filePath)
                    player.stop()
                } else if (isCurrent && isVideo) {
                    console.log("Starting video (isCurrent):", filePath)
                }
            }
            
            property int rotation: 0
            property double zoom: 1.0
            
            // Rotation animation
            Behavior on rotation {
                RotationAnimation { duration: 250; easing.type: Easing.OutCubic; direction: RotationAnimation.Shortest }
            }
            
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
            
            function togglePlay() {
                if (isVideo) {
                    if (player.playbackState === MediaPlayer.PlayingState)
                        player.pause()
                    else
                        player.play()
                }
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
                        source: {
                            if (isVideo || !filePath) return ""
                            
                            // RAW files: Use AsyncImageProvider for LibRaw embedded thumbnail extraction
                            // (Fast ~200ms vs 120+ sec full decode)
                            if (isRaw) {
                                return "image://async/" + filePath
                            }
                            
                            // Regular images: Direct loading for best performance
                            var url = filePath
                            if (url.startsWith("\\\\")) {
                                return "file:" + url.replace(/\\/g, "/")
                            }
                            if (url.indexOf("://") === -1) {
                                return "file:///" + url.replace(/\\/g, "/")
                            }
                            return url
                        }
                        
                        // Bind size to zoom
                        width: flickable.width * flickable.zoom
                        height: flickable.height * flickable.zoom
                        rotation: root.rotation
                        
                        anchors.centerIn: parent
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        cache: true
                        mipmap: true 
                        autoTransform: true
                        
                        property real startTime: 0
                        onSourceChanged: {
                            startTime = new Date().getTime()
                            console.log("[PhotoViewer] Image source changed to:", source)
                            console.log("[PhotoViewer] Delegate filePath:", filePath)
                            console.log("[PhotoViewer] Is video:", isVideo, "| Is current:", isCurrent)
                        }
                        onStatusChanged: {
                            console.log("[PhotoViewer] Image status:", status, "for index:", index)
                            if (status === Image.Ready) {
                                var endTime = new Date().getTime()
                                root.imageLoaded(endTime - startTime)
                                console.log("[PhotoViewer] ✓ Image loaded in", (endTime - startTime), "ms")
                                console.log("[PhotoViewer]   - Image bounds: width=", img.width, "height=", img.height)
                                console.log("[PhotoViewer]   - Source size:", img.sourceSize.width, "x", img.sourceSize.height)
                                console.log("[PhotoViewer]   - Implicit size:", img.implicitWidth, "x", img.implicitHeight)
                                console.log("[PhotoViewer]   - Painted width:", img.paintedWidth, "painted height:", img.paintedHeight)
                                console.log("[PhotoViewer]   - Visible:", img.visible, "| Opacity:", img.opacity)
                                console.log("[PhotoViewer]   - Flickable zoom:", flickable.zoom)
                            } else if (status === Image.Error) {
                                console.error("[PhotoViewer] ✗ Image load FAILED for source:", source)
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
            
            // Video Player
            Item {
                anchors.fill: parent
                visible: isVideo
                
                MediaPlayer {
                    id: player
                    source: {
                        if (!isCurrent || !visible || !filePath) return ""
                        
                        var url = filePath
                        // If it's a Windows network path (starts with \\), MediaPlayer needs file: protocol
                        if (url.startsWith("\\\\")) {
                            return "file:" + url.replace(/\\/g, "/")
                        }
                        // If it doesn't have a protocol and isn't a UNC path, prepend file:///
                        if (url.indexOf("://") === -1) {
                            return "file:///" + url.replace(/\\/g, "/")
                        }
                        return url
                    }
                    audioOutput: AudioOutput {}
                    videoOutput: videoOutput
                    autoPlay: false
                    
                    onPlaybackStateChanged: {
                        console.log("[MediaPlayer] Playback state:", playbackState, "for:", filePath)
                    }
                    
                    onMetaDataChanged: {
                        console.log("[MediaPlayer] Video codec:", metaData.value(7)) // VideoCodec
                        console.log("[MediaPlayer] Resolution:", metaData.value(12), "x", metaData.value(13)) // Resolution  
                        console.log("[MediaPlayer] Video bitrate:", metaData.value(14), "bps") // VideoBitRate
                    }
                    
                    onErrorOccurred: (error, errorString) => {
                        console.log("MediaPlayer Error: " + errorString + " (" + error + ")")
                    }
                }
                
                // Play Button Overlay
                Rectangle {
                    anchors.centerIn: parent
                    width: 80
                    height: 80
                    radius: 40
                    color: "#80000000"
                    visible: isVideo && player.playbackState !== MediaPlayer.PlayingState
                    
                    Canvas {
                        anchors.centerIn: parent
                        width: 40
                        height: 40
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.fillStyle = "white"
                            ctx.beginPath()
                            ctx.moveTo(10, 5)
                            ctx.lineTo(35, 20)
                            ctx.lineTo(10, 35)
                            ctx.closePath()
                            ctx.fill()
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: player.play()
                    }
                }
                
                VideoOutput {
                    id: videoOutput
                    anchors.fill: parent
                    fillMode: VideoOutput.PreserveAspectFit
                }
                
                // Simple Play/Pause on Tap
                TapHandler {
                    onTapped: {
                        root.controlsVisible = !root.controlsVisible
                    }
                }
                
                // Play Icon Overlay (Center)
                Rectangle {
                    anchors.centerIn: parent
                    width: 80
                    height: 80
                    radius: 40
                    color: "#80000000"
                    visible: player.playbackState !== MediaPlayer.PlayingState && root.controlsVisible
                    
                    Text {
                        anchors.centerIn: parent
                        text: "▶"
                        color: "white"
                        font.pixelSize: 40
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: player.play()
                    }
                }
                
                // Bottom Controls Bar
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 80
                    color: "#AA000000"
                    visible: root.controlsVisible && isVideo
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 15
                        
                        // Play/Pause Button
                        Text {
                            text: player.playbackState === MediaPlayer.PlayingState ? "⏸" : "▶"
                            color: "white"
                            font.pixelSize: 30
                            MouseArea {
                                anchors.fill: parent
                                onClicked: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
                            }
                        }
                        
                        // Current Time
                        Text {
                            text: {
                                var m = Math.floor(player.position / 60000)
                                var s = Math.floor((player.position % 60000) / 1000)
                                return m + ":" + (s < 10 ? "0" + s : s)
                            }
                            color: "white"
                            font.pixelSize: 14
                        }
                        
                        // Slider
                        Slider {
                            Layout.fillWidth: true
                            from: 0
                            to: player.duration
                            value: player.position
                            onMoved: player.setPosition(value)
                        }
                        
                        // Total Time
                        Text {
                            text: {
                                var m = Math.floor(player.duration / 60000)
                                var s = Math.floor((player.duration % 60000) / 1000)
                                return m + ":" + (s < 10 ? "0" + s : s)
                            }
                            color: "white"
                            font.pixelSize: 14
                        }
                    }
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
            source: (root.model && root.currentIndex >= 0 && root.model.data(root.model.index(root.currentIndex, 0), 257).toLowerCase().indexOf(".mp4") === -1) ? "file:///" + root.model.data(root.model.index(root.currentIndex, 0), 257) : "" // FilePathRole (257)
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
        if (event.key === Qt.Key_Space) {
             if (listView.currentItem && listView.currentItem.togglePlay) {
                 listView.currentItem.togglePlay()
             }
             event.accepted = true
        } else if (event.key === Qt.Key_Left) {
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

    // Info Overlay
    Rectangle {
        id: infoOverlay
        anchors.fill: parent
        color: "#CC000000"
        visible: false
        z: 25
        
        property var meta: null
        
        MouseArea { anchors.fill: parent; onClicked: infoOverlay.visible = false } // Click to dismiss
        
        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.8, 400)
            height: Math.min(parent.height * 0.8, 500)
            color: "#202020"
            radius: 10
            border.color: "#404040"
            border.width: 1
            
            // Prevent clicks inside the dialog from closing it
            MouseArea { anchors.fill: parent; onClicked: (mouse) => mouse.accepted = true }
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 10
                
                Text {
                    text: "Details"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Rectangle { height: 1; Layout.fillWidth: true; color: "#404040" }
                
                Repeater {
                    model: infoOverlay.meta ? Object.keys(infoOverlay.meta) : []
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: modelData + ":"
                            color: "#AAAAAA"
                            font.pixelSize: 14
                            Layout.preferredWidth: 100
                        }
                        Text {
                            text: infoOverlay.meta[modelData]
                            color: "white"
                            font.pixelSize: 14
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }
                        
                        // Folder Icon Button for "Path"
                        Button {
                            visible: modelData === "Path"
                            text: "📂" // Folder icon
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 30
                            onClicked: {
                                console.log("Opening in explorer: " + infoOverlay.meta[modelData])
                                desktopHelper.openInExplorer(infoOverlay.meta[modelData])
                            }
                        }
                    }
                }
                
                Item { Layout.fillHeight: true } // Spacer
                
                Button {
                    text: "Close"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: infoOverlay.visible = false
                }
            }
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
        
        RowLayout {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 10
            spacing: 10
            
                    Button {
                text: "Info"
                onClicked: {
                    if (listView.currentItem && listView.currentItem.isVideo) {
                        var m = {}
                        m["Filename"] = listView.currentItem.fileName
                        m["Path"] = listView.currentItem.filePath
                        m["Type"] = "Video"
                        infoOverlay.meta = m
                    } else {
                        infoOverlay.meta = root.model.getMetadata(root.currentIndex)
                    }
                    infoOverlay.visible = true
                }
            }

            Button {
                text: "Crop"
                visible: listView.currentItem && !listView.currentItem.isVideo
                onClicked: {
                    // Simple center crop for now to verify backend
                    // In next step we will add a visual selector
                    var rect = Qt.rect(0.1, 0.1, 0.8, 0.8)
                    if (root.model.cropImage(root.currentIndex, rect)) {
                        console.log("Image cropped successfully")
                    }
                }
            }

            Button {
                text: "Edit"
                visible: listView.currentItem && !listView.currentItem.isVideo
                onClicked: root.isEditing = true
            }
        }
    }
}
