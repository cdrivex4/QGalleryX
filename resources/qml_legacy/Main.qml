import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.settings
import Qt.labs.platform
import QGalleryX 1.0

ApplicationWindow {
    id: window
    visible: true
    title: qsTr("Samsung Gallery Clone")
    color: "#000000"

    property string currentPath: ""
    property int previousTab: 0
    property bool openedViaDrop: false
    property var activeModel: (mainLayout.currentIndex === 0 && viewLoader.item) ? viewLoader.item.model : (mainLayout.currentIndex === 1 ? albumsView.activeModel : null)
    
    // Global keyboard handling for seamless Windows Explorer style caret navigation
    Keys.forwardTo: [photoViewer.visible ? photoViewer : (window.activeGrid ? window.activeGrid : searchField)]

    // Global view preferences
    property bool useTiles: false
    property int groupingMode: 0

    onActiveModelChanged: {
        if (activeModel) {
            activeModel.filterQuery = searchField.text
        }
    }


    // Persistence
    Settings {
        id: settings
        property string lastFolder: ""
        property int graphicsApi: 0
        property int windowWidth: 1280
        property int windowHeight: 720
        property int windowX: 100
        property int windowY: 100
    }
    
    width: settings.windowWidth
    height: settings.windowHeight
    x: settings.windowX
    y: settings.windowY
    
    onWidthChanged: if (visible) settings.windowWidth = width
    onHeightChanged: if (visible) settings.windowHeight = height
    onXChanged: if (visible) settings.windowX = x
    onYChanged: if (visible) settings.windowY = y

    ImageModel {
        id: imageModel
    }

    Connections {
        target: imageModel
        function onPassOneCompleted(scanId) { window.syncPendingFileToModel() }
        function onItemsPopulated(scanId) { window.syncPendingFileToModel() }
        function onCountChanged() {
            if (window.pendingFileToOpen !== "" || (photoViewer.visible && photoViewer.model !== imageModel)) {
                window.syncPendingFileToModel()
            }
        }
        function onIsLoadingChanged() {
            if (!imageModel.isLoading && (window.pendingFileToOpen !== "" || (photoViewer.visible && photoViewer.model !== imageModel))) {
                window.syncPendingFileToModel()
            }
        }
    }

    function syncPendingFileToModel() {
        if (!photoViewer.visible || !imageModel || imageModel.count === 0) return;
        
        var activePath = (typeof photoViewer.getCurrentFilePath === "function") 
                         ? photoViewer.getCurrentFilePath() : "";
        if (activePath === "" && window.pendingFileToOpen !== "") {
            activePath = window.pendingFileToOpen;
        }
        if (activePath === "") return;

        var idx = imageModel.indexOfPath(activePath);
        if (idx !== -1) {
            photoViewer.model = imageModel;
            photoViewer.currentIndex = idx;
            window.pendingFileToOpen = "";
            console.log("[Main.qml] Upgraded PhotoViewer model to full folder with " + imageModel.count + " images at index " + idx);
        }
    }

    AlbumModel {
        id: albumModel
        sourceModel: imageModel
    }

    function formatScanPath(rawPath) {
        if (!rawPath || rawPath.length === 0) return "Scan Folder"
        var str = rawPath.toString()
        str = str.replace(/^(file:\/{3})|(file:)/, "")
        var isNetwork = str.startsWith("\\\\") || str.startsWith("//")
        str = str.replace(/\\+/g, "\\").replace(/\//g, "\\")
        if (isNetwork && !str.startsWith("\\\\")) {
            str = "\\" + str
        }
        return str
    }

    onCurrentPathChanged: {
        if (currentPath !== "") {
            console.log("[Main.qml] Path changed to:", currentPath)
            if (imageModel) {
                imageModel.scanDirectory(currentPath)
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Select Image Folder"
        currentFolder: settings.lastFolder !== "" ? "file:///" + settings.lastFolder : StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        onAccepted: {
            var path = folderDialog.folder.toString()
            // Robust path cleaning for Windows
            path = path.replace(/^(file:\/{3})|(file:)/, "")
            if (Qt.platform.os === "windows") {
                 // Handle /C:/Users... -> C:/Users...
                 if (path.startsWith("/") && path.indexOf(":") === 2) path = path.substring(1);
            }
            
            console.log("Selected folder: " + path)
            window.currentPath = path
            settings.lastFolder = path // Save to settings
            // galleryViewItem.scanFolder(path) // Handled by binding
        }
    }

    Component.onCompleted: {
        // Refresh Graphics Info
        appSettings.refreshGraphicsInfo(window)

        // Load last folder
        if (settings.lastFolder !== "") {
            console.log("Loading last folder: " + settings.lastFolder)
            window.currentPath = settings.lastFolder
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Top Bar (opaque background to prevent z-bleed from content below)
        Rectangle {
            z: 200
            Layout.fillWidth: true
            color: "#111"
            implicitHeight: topBarRow.implicitHeight + 20
            visible: !photoViewer.visible && (mainLayout.currentIndex === 0 || mainLayout.currentIndex === 1)

            RowLayout {
                id: topBarRow
                anchors.fill: parent
                anchors.margins: 10
                anchors.rightMargin: 20
                anchors.leftMargin: 20
            
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: "🔍 Filter filenames or folders..."
                    color: "white"
                    font.pixelSize: 14
                    background: Rectangle {
                        color: "#222"
                        radius: 8
                        border.color: searchField.activeFocus ? "#4A90E2" : "#444"
                    }
                    onTextChanged: {
                        if (window.activeModel) {
                            window.activeModel.filterQuery = text
                        }
                        
                        // Also filter the global image model so we can extract valid directories for Albums
                        if (viewLoader.item && viewLoader.item.model) {
                            viewLoader.item.model.filterQuery = text
                            
                            if (typeof viewLoader.item.model.getActiveDirectories === "function") {
                                var dirs = viewLoader.item.model.getActiveDirectories()
                                albumModel.applyFilterFromPaths(dirs)
                            }
                        }
                    }
                    
                    Button {
                        text: "✖"
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 10
                        visible: searchField.text.length > 0
                        width: 24
                        height: 24
                        background: Rectangle { color: "transparent" }
                        contentItem: Text {
                            text: parent.text
                            color: parent.hovered ? "#FFF" : "#888"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: searchField.text = ""
                    }
                }
                
                Button {
                    id: scanBtn
                    Layout.preferredWidth: 180
                    Layout.maximumWidth: 200
                    background: Rectangle {
                        color: parent.hovered ? "#444" : "#333"
                        radius: 8
                    }
                    contentItem: Text {
                        text: "📁 " + window.formatScanPath(window.currentPath)
                        color: "white"
                        font.bold: true
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: folderDialog.open()
                    ToolTip.visible: hovered && window.currentPath !== ""
                    ToolTip.text: window.currentPath
                }

                Item {
                    id: snailContainer
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48

                    Canvas {
                        id: progressRing
                        anchors.fill: parent
                        visible: imageModel && imageModel.precacheMode !== 0

                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);
                            var centerX = width / 2;
                            var centerY = height / 2;
                            var radius = width / 2 - 3;
                            
                            // Background ring track
                            ctx.beginPath();
                            ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI, false);
                            ctx.lineWidth = 3;
                            ctx.strokeStyle = "#333333";
                            ctx.stroke();
                            
                            // Progress arc
                            if (imageModel && imageModel.crawlerTotal > 0) {
                                var progress = imageModel.crawlerProgress;
                                var startAngle = -Math.PI / 2;
                                var endAngle = startAngle + (progress * 2 * Math.PI);
                                
                                ctx.beginPath();
                                ctx.arc(centerX, centerY, radius, startAngle, endAngle, false);
                                ctx.lineWidth = 3;
                                ctx.strokeStyle = imageModel.precacheMode === 1 ? "#FFD700" : "#FF4444";
                                ctx.stroke();
                            }
                        }

                        Connections {
                            target: imageModel ? imageModel : null
                            function onCrawlerProgressChanged() { progressRing.requestPaint() }
                            function onPrecacheModeChanged() { progressRing.requestPaint() }
                        }
                    }

                    Button {
                        id: snailButton
                        anchors.centerIn: parent
                        width: 38
                        height: 38
                        property int mode: imageModel ? imageModel.precacheMode : 1
                        
                        background: Rectangle {
                            color: parent.hovered ? "#444" : "#222"
                            radius: 19
                        }
                        contentItem: SvgIcon {
                            iconName: "snail"
                            size: 20
                            color: snailButton.mode === 0 ? "white" : (snailButton.mode === 1 ? "yellow" : "red")
                            anchors.centerIn: parent
                        }
                        onClicked: {
                            if (imageModel) {
                                var newMode = (imageModel.precacheMode + 1) % 3
                                imageModel.precacheMode = newMode
                                viewportGovernor.batterySaverMode = (newMode === 0)
                            }
                        }
                        ToolTip.visible: hovered
                        ToolTip.text: {
                            if (!imageModel) return ""
                            if (mode === 0) return "Battery Saver (Offscreen Crawler Paused)"
                            var pct = Math.round(imageModel.crawlerProgress * 100)
                            var modeStr = (mode === 1) ? "Yellow (Lookahead Window)" : "Red (Aggressive Full Crawler)"
                            return modeStr + "\nOffscreen Crawled: " + imageModel.crawlerIndex + " / " + imageModel.crawlerTotal + " (" + pct + "%)\nActive In-Flight Tasks: " + imageModel.activeJobs
                        }
                    }
                }
                
                Button {
                    text: window.useTiles ? "View: Tiles" : "View: Semantic"
                    onClicked: window.useTiles = !window.useTiles
                    Layout.preferredWidth: 120
                    background: Rectangle { color: parent.hovered ? "#444" : "#333"; radius: 8 }
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                
                ComboBox {
                    Layout.preferredWidth: 120
                    model: ["Auto", "Day", "Week", "Month", "Year"]
                    currentIndex: window.groupingMode
                    visible: !window.useTiles
                    onCurrentIndexChanged: {
                        window.groupingMode = currentIndex
                    }
                    background: Rectangle { color: "#333"; radius: 8; border.color: "#444" }
                    contentItem: Text {
                        text: "Group: " + parent.displayText
                        color: "white"
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                    delegate: ItemDelegate {
                        width: parent.width
                        contentItem: Text { text: modelData; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                        background: Rectangle { color: parent.highlighted ? "#555" : "#333" }
                    }
                }
            }
        }

        // Album Size Slider (only visible on Albums tab)
        RowLayout {
            z: 100
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 4
            visible: !photoViewer.visible && mainLayout.currentIndex === 1
            spacing: 10

            Text {
                text: "Size:"
                color: "#aaa"
                font.pixelSize: 12
            }

            Slider {
                id: albumSizeSlider
                Layout.fillWidth: true
                from: 100
                to: 400
                value: 220
                stepSize: 10
                onValueChanged: albumsView.cellSize = value

                background: Rectangle {
                    x: albumSizeSlider.leftPadding
                    y: albumSizeSlider.topPadding + albumSizeSlider.availableHeight / 2 - height / 2
                    width: albumSizeSlider.availableWidth
                    height: 4
                    radius: 2
                    color: "#333"

                    Rectangle {
                        width: albumSizeSlider.visualPosition * parent.width
                        height: parent.height
                        color: "#4A90E2"
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: albumSizeSlider.leftPadding + albumSizeSlider.visualPosition * (albumSizeSlider.availableWidth - width)
                    y: albumSizeSlider.topPadding + albumSizeSlider.availableHeight / 2 - height / 2
                    width: 16
                    height: 16
                    radius: 8
                    color: albumSizeSlider.pressed ? "#6AB0FF" : "#4A90E2"
                    border.color: "#222"
                    border.width: 1
                }
            }

            Text {
                text: Math.round(albumSizeSlider.value) + "px"
                color: "#aaa"
                font.pixelSize: 12
                Layout.preferredWidth: 40
            }
        }

        StackLayout {
            id: mainLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            currentIndex: bottomBar.currentIndex
            visible: !photoViewer.visible

            // Tab 0: Pictures
            Item {
                id: galleryTab
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                Loader {
                    id: viewLoader
                    anchors.fill: parent
                    sourceComponent: window.useTiles ? tilesViewComponent : semanticViewComponent
                    
                    Binding {
                        target: viewLoader.item
                        property: "folderPath"
                        value: window.currentPath
                    }
                    Binding {
                        target: viewLoader.item
                        property: "groupingMode"
                        value: window.groupingMode
                    }
                }
                
                Component {
                    id: semanticViewComponent
                    GalleryViewSemantic {
                        id: galleryViewSemantic
                        model: imageModel
                        onImageClicked: (index) => {
                            photoViewer.model = galleryViewSemantic.model
                            photoViewer.currentIndex = index
                            photoViewer.visible = true
                        }
                        onImageLoaded: (timeMs) => statsOverlay.reportLoadTime(timeMs)
                    }
                }
                
                Component {
                    id: tilesViewComponent
                    GalleryViewTiles {
                        id: galleryViewTiles
                        model: imageModel
                        onImageClicked: (index) => {
                            photoViewer.model = galleryViewTiles.model
                            photoViewer.currentIndex = index
                            photoViewer.visible = true
                        }
                    }
                }
                
                // Floating Controls
            }

            // Tab 1: Albums
            AlbumsView {
                id: albumsView
                model: albumModel
                onImageClicked: (index, model) => {
                    photoViewer.model = model
                    photoViewer.currentIndex = index
                    photoViewer.visible = true
                }
            }


            // Tab 2: Menu
            Item {
                id: menuTabItem
                onVisibleChanged: {
                    if (visible && typeof cacheStatsRepeater !== "undefined") {
                        cacheStatsRepeater.refreshStats()
                    }
                }

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 20
                    contentWidth: availableWidth
                    clip: true

                    ColumnLayout {
                        width: Math.min(parent.width, 400)
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 20
                        
                        Item { height: 20; width: 1 } // Top Spacer
                        
                        Text {
                            text: "Menu"
                            color: "white"
                            font.pixelSize: 28
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                        }

                        // --- Compact High-Density System Info & Stats Card (Top of Menu) ---
                        Rectangle {
                            Layout.fillWidth: true
                            height: 64
                            color: "#1e1e1e"
                            radius: 8
                            border.color: "#383838"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 12

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 3

                                    Text {
                                        text: "GPU: " + (appSettings ? appSettings.getGpuName(window) : "GPU")
                                        color: "#e0e0e0"
                                        font.pixelSize: 12
                                        font.bold: true
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    RowLayout {
                                        spacing: 12
                                        Text {
                                            id: memUsageTextTop
                                            text: "App RAM: " + (systemMonitor ? systemMonitor.memoryUsageMB.toFixed(1) : "0") + " MB"
                                            color: "#00FFFF"
                                            font.pixelSize: 11
                                            font.bold: true
                                        }
                                        Text {
                                            text: "VRAM: " + (systemMonitor ? Math.round(systemMonitor.gpuVramUsedMB) : "0") + " MB"
                                            color: "#FFA500"
                                            font.pixelSize: 11
                                            font.bold: true
                                        }
                                        Text {
                                            text: "Threads: " + (appSettings ? appSettings.concurrentThreads : "0")
                                            color: "#aaa"
                                            font.pixelSize: 11
                                        }
                                    }
                                }

                                Rectangle {
                                    width: 1
                                    Layout.fillHeight: true
                                    color: "#383838"
                                }

                                CheckBox {
                                    id: statsCheckboxTop
                                    checked: statsOverlay.visible
                                    onCheckedChanged: statsOverlay.visible = checked
                                    Layout.alignment: Qt.AlignVCenter
                                    contentItem: Text {
                                        text: "Performance\nStats Overlay"
                                        color: statsCheckboxTop.checked ? "#00FF00" : "#ccc"
                                        font.pixelSize: 11
                                        font.bold: true
                                        leftPadding: parent.indicator.width + parent.spacing
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }
                        }
                        
                        StyledButton {
                            Layout.fillWidth: true
                            text: "Select Folder"
                            iconText: "📁"
                            fontSize: 14
                            onClicked: {
                                console.log("Opening folder dialog...")
                                folderDialog.open()
                            }
                        }
                        
                        StyledButton {
                            Layout.fillWidth: true
                            text: "Rebuild Cache"
                            iconText: "⟳"
                            fontSize: 14
                            onClicked: {
                                if (window.currentPath !== "") {
                                    console.log("Rebuilding cache for: " + window.currentPath)
                                    if (imageModel) imageModel.reCrawl()
                                    if (viewLoader.item && typeof viewLoader.item.scanFolder === "function") {
                                        viewLoader.item.scanFolder(window.currentPath)
                                    }
                                } else {
                                    folderDialog.open()
                                }
                            }
                        }
                        
                        StyledButton {
                            Layout.fillWidth: true
                            text: "NUKE ALL CACHE DB & FILES"
                            iconText: "💥"
                            fontBold: true
                            backgroundColor: "#882222"
                            hoverColor: "#AA2E2E"
                            pressedColor: "#661818"
                            onClicked: {
                                if (appSettings) {
                                    appSettings.nukeDiskCache()
                                    cacheStatsRepeater.refreshStats()
                                }
                            }
                        }

                        // --- Per-Drive Cache DB Breakdown ---
                        Text {
                            text: "Cache DB Storage & Drives"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 16
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Text {
                            text: "DB File: " + (appSettings ? appSettings.getDiskCachePath() : "")
                            color: "#888"
                            font.pixelSize: 11
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                        }

                        ColumnLayout {
                            id: cacheStatsContainer
                            Layout.fillWidth: true
                            spacing: 8

                            Repeater {
                                id: cacheStatsRepeater
                                property var statsMap: ({})
                                property var statKeys: []

                                function refreshStats() {
                                    if (appSettings && typeof appSettings.getTrackedRootPathStats === "function") {
                                        var res = appSettings.getTrackedRootPathStats()
                                        statsMap = res
                                        statKeys = Object.keys(res)
                                    }
                                }

                                Component.onCompleted: refreshStats()

                                model: statKeys
                                delegate: Rectangle {
                                    Layout.fillWidth: true
                                    height: 48
                                    color: "#252525"
                                    radius: 8
                                    border.color: "#3d3d3d"

                                    property var itemData: cacheStatsRepeater.statsMap[modelData]
                                    property int count: itemData ? (itemData.count || 0) : 0
                                    property real mb: itemData ? ((itemData.bytes || 0) / (1024 * 1024)) : 0.0

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 12

                                        Text {
                                            text: modelData === "__total__" ? "Total Cache Stats" : ("Drive: " + modelData)
                                            color: modelData === "__total__" ? "#FFD700" : "white"
                                            font.bold: true
                                            font.pixelSize: 13
                                            Layout.preferredWidth: 130
                                        }

                                        Text {
                                            text: count + " items (" + mb.toFixed(1) + " MB)"
                                            color: "#aaa"
                                            font.pixelSize: 12
                                            Layout.fillWidth: true
                                        }

                                        StyledButton {
                                            text: modelData === "__total__" ? "Refresh" : "Nuke"
                                            iconText: modelData === "__total__" ? "⟳" : "🗑"
                                            visible: count > 0 || modelData === "__total__"
                                            Layout.preferredWidth: 85
                                            Layout.preferredHeight: 30
                                            fontSize: 12
                                            onClicked: {
                                                if (modelData === "__total__") {
                                                    cacheStatsRepeater.refreshStats()
                                                } else {
                                                    appSettings.nukeCacheForPath(modelData)
                                                    cacheStatsRepeater.refreshStats()
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // --- Connected Devices & Drives (Auto-Enumerated Array) ---
                        Text {
                            text: "Connected Storage Devices & Drives"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 16
                            Layout.alignment: Qt.AlignHCenter
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Repeater {
                                id: mainMountedDrivesRepeater
                                model: (typeof desktopHelper !== "undefined" && typeof desktopHelper.getMountedDrives === "function") ? desktopHelper.getMountedDrives() : []
                                delegate: Rectangle {
                                    Layout.fillWidth: true
                                    height: 52
                                    color: "#252525"
                                    radius: 8
                                    border.color: "#3d3d3d"

                                    property string root: modelData.rootPath || ""
                                    property string dName: modelData.name || ""
                                    property string dType: modelData.driveType || "FIXED"
                                    property real freeGB: (modelData.bytesFree || 0) / (1024 * 1024 * 1024)
                                    property real totalGB: (modelData.bytesTotal || 0) / (1024 * 1024 * 1024)

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 12

                                        Rectangle {
                                            width: 42
                                            height: 24
                                            radius: 4
                                            color: dType === "REMOVABLE" ? "#1a3a2a" : (dType === "REMOTE" ? "#3a2a1a" : "#1a2a3a")
                                            border.color: dType === "REMOVABLE" ? "#00FF7F" : (dType === "REMOTE" ? "#FFA500" : "#38BDF8")

                                            Text {
                                                anchors.centerIn: parent
                                                text: dType === "REMOVABLE" ? "USB" : (dType === "REMOTE" ? "NET" : "DRV")
                                                color: dType === "REMOVABLE" ? "#00FF7F" : (dType === "REMOTE" ? "#FFA500" : "#38BDF8")
                                                font.pixelSize: 10
                                                font.bold: true
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2

                                            Text {
                                                text: (dName.length > 0 && dName !== root ? dName + " (" + root + ")" : root)
                                                color: "white"
                                                font.bold: true
                                                font.pixelSize: 13
                                            }

                                            Text {
                                                text: freeGB.toFixed(1) + " GB free of " + totalGB.toFixed(1) + " GB (" + (modelData.fileSystemType || "") + ")"
                                                color: "#888"
                                                font.pixelSize: 11
                                            }
                                        }

                                        StyledButton {
                                            text: "Open"
                                            iconText: "📁"
                                            Layout.preferredWidth: 80
                                            Layout.preferredHeight: 30
                                            fontSize: 12
                                            onClicked: {
                                                window.currentPath = root
                                                mainLayout.currentIndex = 0 // Switch to Pictures tab
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#444"
                            Layout.topMargin: 10
                            Layout.bottomMargin: 10
                        }
                        
                        Text {
                            text: "Graphics API (Requires Restart)"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 16
                            Layout.alignment: Qt.AlignHCenter
                        }
                        
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Auto", "Direct3D 11", "Vulkan", "OpenGL", "Software"]
                            currentIndex: appSettings.selectedApi
                            onActivated: (index) => {
                                let oldVal = appSettings.selectedApi
                                if (oldVal === index) return
                                appSettings.selectedApi = index
                                restartDialog.revertAction = function() { appSettings.selectedApi = oldVal }
                                restartDialog.open()
                            }
                        }
                        
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 5
                            Text {
                                text: "Active API:"
                                color: "#aaa"
                                font.pixelSize: 14
                            }
                            Text {
                                text: appSettings.graphicsApi
                                color: "#00FF00" // Green for visibility
                                font.bold: true
                                font.pixelSize: 14
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#444"
                            Layout.topMargin: 10
                            Layout.bottomMargin: 10
                        }

                        Text {
                            text: "Supported APIs:"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 16
                            Layout.alignment: Qt.AlignHCenter
                        }
                        
                        Column {
                            Layout.fillWidth: true
                            spacing: 5
                            
                            Repeater {
                                model: [
                                    {name: "Direct3D 11", value: 1},
                                    {name: "Vulkan", value: 2},
                                    {name: "OpenGL", value: 3},
                                    {name: "Software", value: 4}
                                ]
                                
                                Text {
                                    text: modelData.name + ": " + (appSettings && appSettings.isApiSupported(modelData.value) ? "Available" : "Not Available")
                                    color: (appSettings && appSettings.isApiSupported(modelData.value)) ? "#8f8" : "#f88"
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    font.pixelSize: 14
                                }
                            }
                        }

                        Item { height: 20; width: 1 } // Bottom Spacer
                    }
                }
            }
        }

        BottomBar {
            id: bottomBar
            Layout.fillWidth: true
            height: 60
            currentIndex: mainLayout.currentIndex
            onTabSelected: (index) => {
                if (mainLayout.currentIndex !== 3) {
                    window.previousTab = mainLayout.currentIndex
                }
                mainLayout.currentIndex = index
                if (index === 2 && typeof cacheStatsRepeater !== "undefined") {
                    cacheStatsRepeater.refreshStats()
                }
            }
        }
    }
    
    MessageDialog {
        id: restartDialog
        title: "Restart Required"
        text: "The application needs to restart to apply the graphics API change."
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        
        property var revertAction: null
        onAccepted: appSettings.restartApp()
        onRejected: {
            if (revertAction) {
                revertAction()
            }
            mainLayout.currentIndex = window.previousTab
        }
    }

    // Stats Overlay
    StatsOverlay {
        id: statsOverlay
        x: parent.width - width - 10
        y: 10
        apiName: appSettings.graphicsApi
        scanEngine: imageModel ? imageModel.scanMethod : "Idle"
        scanDuration: imageModel ? imageModel.scanDurationMs : 0
        isLoading: imageModel.isLoading
        loadedCount: imageModel.totalCount === 0 ? imageModel.scanProgress : imageModel.count
        totalCount: imageModel.totalCount
        activeThreadCount: appSettings ? appSettings.concurrentThreads : 0
        precacheMode: imageModel ? imageModel.precacheMode : 1
        crawlerIndex: imageModel ? imageModel.crawlerIndex : 0
        crawlerTotal: imageModel ? imageModel.crawlerTotal : 0
        crawlerProgress: imageModel ? imageModel.crawlerProgress : 0.0
        activeJobs: imageModel ? imageModel.activeJobs : 0
        z: 100
        onRebuildCacheRequested: {
            if (window.currentPath !== "") {
                console.log("OSD: Rebuilding cache for: " + window.currentPath)
                if (imageModel) imageModel.reCrawl()
                if (viewLoader.item && typeof viewLoader.item.scanFolder === "function") {
                    viewLoader.item.scanFolder(window.currentPath)
                }
            } else {
                folderDialog.open()
            }
        }
    }

    // Photo Viewer Overlay (Full Screen)
    PhotoViewer {
        id: photoViewer
        anchors.fill: parent
        visible: false
        z: 10 // Ensure it's on top
        onBackClicked: {
            var lastActivePath = (typeof photoViewer.getCurrentFilePath === "function") 
                                 ? photoViewer.getCurrentFilePath() : ""
            visible = false
            
            if (window.openedViaDrop) {
                window.openedViaDrop = false
                bottomBar.currentIndex = 0
            }
            
            if (window.activeGrid && imageModel && lastActivePath !== "") {
                var gridIdx = imageModel.indexOfPath(lastActivePath)
                if (gridIdx !== -1) {
                    if (viewLoader.item && typeof viewLoader.item.caretIndex !== "undefined") {
                        viewLoader.item.caretIndex = gridIdx
                    }
                }
                window.activeGrid.forceActiveFocus()
            } else if (window.activeGrid) {
                window.activeGrid.forceActiveFocus()
            }
        }
        
        // Pass stats to overlay
        onImageLoaded: (timeMs) => statsOverlay.reportLoadTime(timeMs)
    }

    // Selection Action Bar
    SelectionActionBar {
        id: selectionActionBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        z: 90
        
        model: window.activeModel
        
        onClearClicked: {
            if (window.activeModel) window.activeModel.clearSelection()
        }
        
        onShareClicked: {
            if (window.activeModel && window.activeModel.selectedCount > 0) {
                var paths = window.activeModel.getSelectedPaths()
                shareDialog.targetPaths = paths
                shareDialog.open()
            }
        }
        onRotateClicked: {
            if (window.activeModel && window.activeModel.selectedCount > 0) {
                var paths = window.activeModel.getSelectedPaths()
                for (var i = 0; i < paths.length; ++i) {
                    imageProcessor.rotateImageVirtual(paths[i], 90)
                }
                imageProcessor.clearImageCache()
                window.activeModel.clearSelection()
                // Force a model refresh to show the rotated images
                if (window.activeModel.scanDirectory) {
                    window.activeModel.scanDirectory(window.currentPath)
                }
            }
        }
        
        onResizeClicked: {
            if (window.activeModel && window.activeModel.selectedCount > 0) {
                resizeEditor.targetPaths = window.activeModel.getSelectedPaths()
                resizeEditor.open()
            }
        }
    }
    
    // Share Dialog
    ShareDialog {
        id: shareDialog
        model: window.activeModel
        anchors.centerIn: parent
    }
    
    // Resize Editor
    ResizeEditor {
        id: resizeEditor
        anchors.centerIn: parent
    }

    // Scanning Overlay (Restored Visuals, Non-blocking)
    Rectangle {
        id: scanningOverlay
        anchors.fill: parent
        color: "#AA000000" // Semi-transparent black
        z: 200 // Topmost
        visible: albumModel.isLoading && albumModel.count === 0

        // Note: No MouseArea here, so clicks pass through to the UI below.

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20

            BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 64
                Layout.preferredHeight: 64
                running: scanningOverlay.visible
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Scanning selected folder..."
                color: "white"
                font.pixelSize: 20
                font.bold: true
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Please wait, this may take a moment."
                color: "#ccc"
                font.pixelSize: 14
            }
        }
    }

    // Toast Notification Overlay for Disk Delay Alerts
    ToastOverlay {
        id: toastOverlay
    }

    function urlToPath(urlStr) {
        if (!urlStr) return "";
        var str = urlStr.toString();
        if (str.startsWith("file:///")) {
            str = str.substring(8);
        } else if (str.startsWith("file://")) {
            str = str.substring(7);
        } else if (str.startsWith("file:")) {
            str = str.substring(5);
        }
        return decodeURIComponent(str).replace(/\//g, "\\");
    }

    function isDirectoryPath(path) {
        if (!path || path.length === 0) return false;
        if (typeof desktopHelper !== "undefined" && typeof desktopHelper.isDirectory === "function") {
            return desktopHelper.isDirectory(path);
        }
        var extIdx = path.lastIndexOf(".");
        var slashIdx = Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\"));
        return extIdx === -1 || extIdx < slashIdx;
    }

    function handleDroppedFiles(paths) {
        if (!paths || paths.length === 0) return;
        var firstPath = paths[0];
        firstPath = firstPath.replace(/\\/g, "/");

        console.log("[Main.qml] Handling dropped path:", firstPath);
        if (typeof toastOverlay !== "undefined" && typeof toastOverlay.showToast === "function") {
            toastOverlay.showToast("Dropped: " + firstPath);
        }

        var isDir = window.isDirectoryPath(firstPath);
        var folder = firstPath;
        if (!isDir) {
            var lastSlash = firstPath.lastIndexOf("/");
            if (lastSlash !== -1) {
                folder = firstPath.substring(0, lastSlash);
            }
        }
        if (folder.length > 0 && folder[folder.length - 1] === ':') {
            folder += "/";
        }

        if (!isDir) {
            window.openedViaDrop = true;
            console.log("[Main.qml] Discovered immediate neighbors for 0ms navigation:", firstPath);
            var neighbors = (typeof desktopHelper !== "undefined" && typeof desktopHelper.getAdjacentFiles === "function")
                            ? desktopHelper.getAdjacentFiles(firstPath, 15) : [];
            if (neighbors.length > 0) {
                photoViewer.openWithNeighbors(firstPath, neighbors);
            } else {
                photoViewer.openSingleFile(firstPath);
            }
            window.pendingFileToOpen = firstPath;

            if (window.activeModel) {
                var idx = window.activeModel.indexOfPath(firstPath);
                if (idx !== -1) {
                    photoViewer.model = window.activeModel;
                    photoViewer.currentIndex = idx;
                    window.pendingFileToOpen = "";
                }
            }
        }

        if (window.currentPath !== folder) {
            console.log("[Main.qml] Updating currentPath to folder:", folder);
            window.currentPath = folder;
            settings.lastFolder = folder;
        } else if (imageModel) {
            console.log("[Main.qml] Forcing scanDirectory on dropped folder:", folder);
            imageModel.scanDirectory(folder);
        }
    }

    // Prominent Red Center Drop Zone Overlay
    Rectangle {
        id: dropZoneOverlay
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.7, 560)
        height: Math.min(parent.height * 0.5, 340)
        radius: 20
        color: dropArea.containsDrag ? "#f2e60000" : "#d9cc0000" // Vibrant red, brighter on drag
        border.color: "#ffffff"
        border.width: dropArea.containsDrag ? 4 : 2
        visible: Boolean(dropArea.containsDrag || (window.currentPath === "") || (window.activeModel && window.activeModel.totalCount === 0 && !imageModel.isLoading))
        z: 9999

        Behavior on color { ColorAnimation { duration: 150 } }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 16

            Text {
                text: "📁"
                font.pixelSize: 56
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: "RED DROP ZONE"
                color: "#ffffff"
                font.pixelSize: 24
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: "Drag & drop files or folders anywhere here to open"
                color: "#ffffff"
                font.pixelSize: 15
                Layout.alignment: Qt.AlignHCenter
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 180
                height: 42
                radius: 10
                color: mouseAreaBrowse.containsMouse ? "#ffffff" : "#f0f0f0"

                Text {
                    anchors.centerIn: parent
                    text: "Browse Folder"
                    color: "#cc0000"
                    font.bold: true
                    font.pixelSize: 14
                }

                MouseArea {
                    id: mouseAreaBrowse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: folderDialog.open()
                }
            }
        }
    }

    DropArea {
        id: dropArea
        anchors.fill: parent
        onEntered: (drag) => {
            if (drag.hasUrls) {
                drag.accept(Qt.CopyAction)
            }
        }
        onDropped: (drop) => {
            if (drop.hasUrls) {
                var urls = []
                for (var i = 0; i < drop.urls.length; i++) {
                    urls.push(window.urlToPath(drop.urls[i].toString()))
                }
                window.handleDroppedFiles(urls)
            }
        }
    }
}
