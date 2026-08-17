import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QGalleryX 1.0

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
    
    function openSingleFile(path) {
        if (!path || path.length === 0) return;
        var fName = path.substring(Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\")) + 1);
        root.model = [{
            filePath: path,
            fileName: fName,
            fileSizeFormatted: "",
            dateFormatted: "",
            dimensionsFormatted: ""
        }];
        root.currentIndex = 0;
        root.visible = true;
    }

    function openWithNeighbors(targetPath, neighborsList) {
        if (!targetPath || targetPath.length === 0) return;
        if (!neighborsList || neighborsList.length === 0) {
            openSingleFile(targetPath);
            return;
        }

        var items = [];
        var targetIndex = 0;
        var normTarget = targetPath.replace(/\\/g, "/").toLowerCase();

        for (var i = 0; i < neighborsList.length; i++) {
            var p = neighborsList[i].replace(/\\/g, "/");
            var fName = p.substring(Math.max(p.lastIndexOf("/"), p.lastIndexOf("\\")) + 1);
            items.push({
                filePath: p,
                fileName: fName,
                fileSizeFormatted: "",
                dateFormatted: "",
                dimensionsFormatted: ""
            });
            if (p.toLowerCase() === normTarget) {
                targetIndex = i;
            }
        }

        root.model = items;
        root.currentIndex = targetIndex;
        root.visible = true;
    }

    function getModelCount() {
        if (!root.model) return 0;
        if (typeof root.model.rowCount === "function") {
            return root.model.rowCount();
        }
        if (typeof root.model.count === "number") {
            return root.model.count;
        }
        if (Array.isArray(root.model)) {
            return root.model.length;
        }
        return 0;
    }

    function getCurrentFilePath() {
        if (!root.model || root.currentIndex < 0) return "";
        try {
            if (typeof root.model.data === "function") {
                // Role 257 is FilePathRole (Qt::UserRole + 1)
                var p = root.model.data(root.model.index(root.currentIndex, 0), 257);
                if (p) return p.toString();
            }
            if (Array.isArray(root.model) && root.currentIndex < root.model.length) {
                return root.model[root.currentIndex].filePath || "";
            }
            if (listView.currentItem && listView.currentItem.filePath) {
                return listView.currentItem.filePath;
            }
        } catch (e) {
            console.log("[PhotoViewer] getCurrentFilePath error:", e);
        }
        return "";
    }

    function isVideoFile(path) {
        if (!path) return false;
        var lower = path.toLowerCase();
        return lower.endsWith(".mp4") || lower.endsWith(".avi") || lower.endsWith(".mkv") || lower.endsWith(".mov");
    }

    function getMetadataForIndex(idx) {
        if (!root.model) return {};
        if (typeof root.model.getMetadata === "function") {
            return root.model.getMetadata(idx);
        }
        var curPath = "";
        var curName = "";
        if (Array.isArray(root.model) && idx >= 0 && idx < root.model.length) {
            curPath = root.model[idx].filePath || "";
            curName = root.model[idx].fileName || "";
        } else if (listView.currentItem) {
            curPath = listView.currentItem.filePath || "";
            curName = listView.currentItem.fileName || "";
        }
        var m = {};
        m["Filename"] = curName !== "" ? curName : curPath.substring(Math.max(curPath.lastIndexOf("/"), curPath.lastIndexOf("\\")) + 1);
        m["Path"] = curPath;
        m["Type"] = isVideoFile(curPath) ? "Video" : "Image";
        if (typeof desktopHelper !== "undefined" && typeof desktopHelper.getFileSizeFormatted === "function") {
            m["Size"] = desktopHelper.getFileSizeFormatted(curPath);
        }
        return m;
    }
    
    onVisibleChanged: {
        if (visible) {
            forceActiveFocus()
            taskScheduler.pauseBackground(true)
            if (currentIndex >= 0) {
                listView.positionViewAtIndex(currentIndex, ListView.SnapPosition)
            }
        } else {
            taskScheduler.pauseBackground(false)
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
            
            property string filePath: (typeof model !== "undefined" && model.filePath !== undefined) ? model.filePath : (typeof modelData !== "undefined" && modelData.filePath !== undefined ? modelData.filePath : "")
            property string fileName: (typeof model !== "undefined" && model.fileName !== undefined) ? model.fileName : (typeof modelData !== "undefined" && modelData.fileName !== undefined ? modelData.fileName : "")
            property bool isVideo: (desktopHelper ? (desktopHelper.getFileType(filePath) === 2) : false) || 
                                    (fileExt === "mp4" || fileExt === "mkv" || fileExt === "avi" || fileExt === "mov" || 
                                     fileExt === "webm" || fileExt === "flv" || fileExt === "ts" || fileExt === "m2ts" || 
                                     fileExt === "mts" || fileExt === "3gp" || fileExt === "wmv" || fileExt === "vob" || 
                                     fileExt === "ogv" || fileExt === "ogg" || fileExt === "mp3" || fileExt === "wav" || 
                                     fileExt === "flac" || fileExt === "m4a" || fileExt === "aac" || fileExt === "wma" || fileExt === "opus")
            property bool isCurrent: index === ListView.view.currentIndex
            onIsCurrentChanged: {
                if (!isCurrent && isVideo) {
                    console.log("Stopping background video:", filePath)
                    player.stop()
                } else if (isCurrent && isVideo) {
                    console.log("Starting video (isCurrent):", filePath)
                }
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
            
            function reloadImage() {
                var p = filePath
                if (!p) return;
                img.source = ""
                thumbImg.source = ""
                img.source = "image://async/" + p + "?retry=" + Date.now()
                thumbImg.source = "image://async/" + p + "?retry=" + Date.now()
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

                    Timer {
                        id: prefetchTimer
                        interval: 300
                        running: !isVideo && filePath !== "" && !isCurrent
                        onTriggered: {
                            if (img.source.toString() === "") {
                                img.source = "image://async/" + filePath
                            }
                        }
                    }

                    Image {
                        id: thumbImg
                        source: (!isVideo && filePath) ? "image://async/" + filePath : ""
                        sourceSize: Qt.size(256, 256)
                        width: flickable.width * flickable.zoom
                        height: flickable.height * flickable.zoom
                        anchors.centerIn: parent
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        cache: true
                        mipmap: true
                        autoTransform: true
                        opacity: img.status === Image.Ready ? 0 : 1
                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }

                    Image {
                        id: img
                        source: (!isVideo && filePath && isCurrent) ? "image://async/" + filePath : ""
                        
                        // Bind size to zoom
                        width: flickable.width * flickable.zoom
                        height: flickable.height * flickable.zoom
                        
                        anchors.centerIn: parent
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        cache: true
                        mipmap: true 
                        autoTransform: true
                        
                        property real startTime: 0
                        onSourceChanged: startTime = new Date().getTime()
                        onStatusChanged: {
                            if (status === Image.Ready) {
                                var endTime = new Date().getTime()
                                root.imageLoaded(endTime - startTime)
                            }
                        }
                    }

                    BusyIndicator {
                        anchors.centerIn: parent
                        width: 64
                        height: 64
                        running: img.status === Image.Loading && !isVideo
                        visible: running
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
                id: videoContainer
                anchors.fill: parent
                visible: isVideo
                property int currentRotation: filePath ? imageProcessor.getVirtualRotation(filePath) : 0
                
                MediaPlayer {
                    id: player
                    source: {
                        if (!isCurrent || !visible || !filePath) return ""
                        if (isVideo) {
                            console.log("PhotoViewer: Loading Video", filePath)
                            // Convert native path to proper QML file URL
                            var path = filePath.split('\\').join('/')
                            if (path.startsWith("//")) {
                                // Network UNC path: file://server/share/file.mp4
                                return "file:" + path
                            } else {
                                // Local drive path: file:///C:/folder/file.mp4
                                if (!path.startsWith("/")) path = "/" + path
                                return "file://" + path
                            }
                        }
                        return ""
                    }
                    audioOutput: AudioOutput {
                        id: audioOutput
                        volume: 1.0
                        muted: false
                    }
                    videoOutput: videoOutput
                    autoPlay: false
                    
                    onErrorOccurred: (error, errorString) => {
                        console.log("MediaPlayer Error: " + errorString + " (" + error + ")")
                    }
                    
                    onPlaybackStateChanged: {
                        if (playbackState === MediaPlayer.PlayingState) {
                            taskScheduler.pauseBackground(true)
                        } else {
                            taskScheduler.pauseBackground(false)
                        }
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
                
                Image {
                    id: videoThumbPlaceholder
                    anchors.centerIn: parent
                    width: (parent.currentRotation % 180 === 0) ? parent.width : parent.height
                    height: (parent.currentRotation % 180 === 0) ? parent.height : parent.width
                    rotation: parent.currentRotation
                    fillMode: Image.PreserveAspectFit
                    source: (isVideo && filePath) ? "image://async/" + filePath + "?idx=" + root.currentIndex : ""
                    visible: isVideo && player.playbackState !== MediaPlayer.PlayingState
                    asynchronous: true
                    cache: true
                    autoTransform: true
                }
                
                VideoOutput {
                    id: videoOutput
                    anchors.centerIn: parent
                    width: (parent.currentRotation % 180 === 0) ? parent.width : parent.height
                    height: (parent.currentRotation % 180 === 0) ? parent.height : parent.width
                    rotation: parent.currentRotation
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
                    visible: isVideo && player.playbackState !== MediaPlayer.PlayingState && root.controlsVisible
                    
                    MediaIcon {
                        anchors.centerIn: parent
                        width: 40
                        height: 40
                        iconType: "play"
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
                        spacing: 12
                        
                        // --- ICON SEQUENCE (FAR LEFT): Play, Speaker, Rotate ---

                        // 1. Play/Pause Flat Icon Button
                        StyledButton {
                            flatStyle: true
                            implicitWidth: 38
                            implicitHeight: 38
                            onClicked: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
                            ToolTip.visible: hovered
                            ToolTip.text: player.playbackState === MediaPlayer.PlayingState ? "Pause" : "Play"

                            MediaIcon {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                iconType: player.playbackState === MediaPlayer.PlayingState ? "pause" : "play"
                            }
                        }

                        // 2. Speaker Volume Icon Button + Popup Vertical Slider
                        Item {
                            id: volumeControlItem
                            implicitWidth: 38
                            implicitHeight: 38
                            Layout.alignment: Qt.AlignVCenter
                            z: 200

                            // Combined non-blocking hover area covering speaker button AND popup slider region
                            MouseArea {
                                id: combinedHoverArea
                                x: -5
                                y: -175
                                width: parent.width + 10
                                height: 175 + parent.height
                                hoverEnabled: true
                                acceptedButtons: Qt.NoButton // Allows clicks/drags to pass to speakerBtn and volumeSlider
                            }

                            StyledButton {
                                id: speakerBtn
                                anchors.fill: parent
                                flatStyle: true
                                onClicked: audioOutput.muted = !audioOutput.muted
                                ToolTip.visible: speakerBtn.hovered
                                ToolTip.text: audioOutput.muted ? "Unmute" : "Mute"

                                MediaIcon {
                                    anchors.centerIn: parent
                                    width: 22
                                    height: 22
                                    iconType: (audioOutput.muted || audioOutput.volume === 0) ? "mute" : "speaker"
                                }
                            }

                            // Vertical Volume Slider Popup
                            Rectangle {
                                id: volumePopup
                                anchors.bottom: speakerBtn.top
                                anchors.bottomMargin: 2
                                anchors.horizontalCenter: speakerBtn.horizontalCenter
                                width: 48
                                height: 170
                                color: "#E6181818"
                                radius: 10
                                border.color: "#50ffffff"
                                border.width: 1
                                visible: combinedHoverArea.containsMouse || volumeSlider.pressed || volumeSlider.hovered
                                z: 100

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 6

                                    Text {
                                        text: audioOutput.muted ? "MUTE" : Math.round(audioOutput.volume * 100) + "%"
                                        color: "#ffffff"
                                        font.pixelSize: 11
                                        font.bold: true
                                        Layout.alignment: Qt.AlignHCenter
                                    }

                                    Slider {
                                        id: volumeSlider
                                        orientation: Qt.Vertical
                                        Layout.fillHeight: true
                                        Layout.alignment: Qt.AlignHCenter
                                        implicitWidth: 26
                                        from: 0.0
                                        to: 1.0
                                        value: audioOutput.muted ? 0.0 : audioOutput.volume
                                        onMoved: {
                                            audioOutput.volume = value
                                            if (value === 0.0) {
                                                audioOutput.muted = true
                                            } else if (audioOutput.muted) {
                                                audioOutput.muted = false
                                            }
                                        }

                                        background: Rectangle {
                                            x: volumeSlider.leftPadding + volumeSlider.availableWidth / 2 - width / 2
                                            y: volumeSlider.topPadding
                                            implicitWidth: 6
                                            implicitHeight: 130
                                            width: 6
                                            height: volumeSlider.availableHeight
                                            radius: 3
                                            color: "#40ffffff" // Inactive grey track

                                            // White active bar from bottom (0 / mute) up to current volume position
                                            Rectangle {
                                                width: parent.width
                                                height: parent.height * volumeSlider.position
                                                anchors.bottom: parent.bottom
                                                color: "#ffffff"
                                                radius: 3
                                            }
                                        }

                                        handle: Rectangle {
                                            x: volumeSlider.leftPadding + volumeSlider.availableWidth / 2 - width / 2
                                            y: volumeSlider.topPadding + (1 - volumeSlider.position) * (volumeSlider.availableHeight - height)
                                            implicitWidth: 18
                                            implicitHeight: 18
                                            radius: 9
                                            color: volumeSlider.pressed ? "#e0e0e0" : "#ffffff"
                                            border.color: "#30000000"
                                            border.width: 1
                                        }
                                    }
                                }
                            }
                        }

                        // 3. Rotate Video Button Icon
                        StyledButton {
                            flatStyle: true
                            implicitWidth: 38
                            implicitHeight: 38
                            ToolTip.visible: hovered
                            ToolTip.text: "Rotate 90°"
                            onClicked: {
                                imageProcessor.rotateImageVirtual(filePath, 90)
                                imageProcessor.clearImageCache()
                                videoContainer.currentRotation = imageProcessor.getVirtualRotation(filePath)
                            }

                            MediaIcon {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                iconType: "rotate"
                            }
                        }

                        // --- TIME & POSITION SLIDER ---

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
                        
                        // Position Slider
                        Slider {
                            id: progressSlider
                            Layout.fillWidth: true
                            implicitHeight: 24
                            from: 0
                            to: player.duration > 0 ? player.duration : 1
                            value: player.position
                            onMoved: player.setPosition(value)

                            background: Rectangle {
                                x: progressSlider.leftPadding
                                y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                                implicitWidth: 200
                                implicitHeight: 6
                                width: progressSlider.availableWidth
                                height: 6
                                radius: 3
                                color: "#40ffffff" // Inactive grey track (remaining video duration)

                                // Active white bar from 0:00 (left) up to current position handle
                                Rectangle {
                                    width: progressSlider.position * parent.width
                                    height: parent.height
                                    color: "#ffffff"
                                    radius: 3
                                }
                            }

                            handle: Rectangle {
                                x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                                y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                                implicitWidth: 18
                                implicitHeight: 18
                                radius: 9
                                color: progressSlider.pressed ? "#e0e0e0" : "#ffffff"
                                border.color: "#30000000"
                                border.width: 1
                            }
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
            source: (root.model && root.currentIndex >= 0 && root.model.data(root.model.index(root.currentIndex, 0), 257).toLowerCase().indexOf(".mp4") === -1) ? "image://async/" + root.model.data(root.model.index(root.currentIndex, 0), 257) : "" // FilePathRole
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
            
            StyledButton { 
                text: "Save"
                isAccent: true
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
            StyledButton { text: "Cancel"; onClicked: root.isEditing = false }
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
        visible: root.controlsVisible && !root.isEditing && root.currentIndex < root.getModelCount() - 1
        
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
            if (currentIndex < root.getModelCount() - 1) currentIndex++
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
                            text: "📂"
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 30
                            flat: true
                            ToolTip.visible: hovered
                            ToolTip.text: "Locate image in folder"
                            onClicked: {
                                console.log("Opening in explorer: " + infoOverlay.meta[modelData])
                                desktopHelper.openInExplorer(infoOverlay.meta[modelData])
                            }
                        }
                    }
                }
                
                Item { Layout.fillHeight: true } // Spacer
                
                StyledButton {
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

        StyledButton {
            text: "Back"
            iconText: "‹"
            fontSize: 14
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 15
            onClicked: root.backClicked()
        }
        
        RowLayout {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 15
            spacing: 10
            
            StyledButton {
                text: "Rotate"
                iconText: "↻"
                fontSize: 14
                onClicked: {
                    var p = (typeof root.getCurrentFilePath === "function") ? root.getCurrentFilePath() : ""
                    if (listView.currentItem && listView.currentItem.filePath) {
                        p = listView.currentItem.filePath
                    }
                    if (p) {
                        imageProcessor.rotateImageVirtual(p, 90)
                        imageProcessor.clearImageCache()
                        if (listView.currentItem && typeof listView.currentItem.reloadImage === "function") {
                            listView.currentItem.reloadImage()
                        }
                    }
                }
            }

            StyledButton {
                text: "Info"
                iconText: "ℹ"
                fontSize: 14
                onClicked: {
                    infoOverlay.meta = root.getMetadataForIndex(root.currentIndex)
                    infoOverlay.visible = true
                }
            }

            StyledButton {
                text: "Edit"
                iconText: "✏"
                fontSize: 14
                visible: listView.currentItem && !listView.currentItem.isVideo
                onClicked: root.isEditing = true
            }
        }
    }
}
