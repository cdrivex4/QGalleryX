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
    property var activeModel: (mainLayout.currentIndex === 0 && viewLoader.item) ? viewLoader.item.model : (mainLayout.currentIndex === 1 ? albumsView.activeModel : null)
    
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

    AlbumModel {
        id: albumModel
        sourceModel: imageModel
    }

    onCurrentPathChanged: {
        if (currentPath !== "") {
            albumModel.scanAlbums(currentPath)
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
                    text: "📁 Scan Folder"
                    font.bold: true
                    font.pixelSize: 14
                    background: Rectangle {
                        color: parent.hovered ? "#444" : "#333"
                        radius: 8
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        font.bold: parent.font.bold
                        font.pixelSize: parent.font.pixelSize
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: folderDialog.open()
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
                        
                        Button {
                            Layout.fillWidth: true
                            text: "Select Folder"
                            onClicked: {
                                console.log("Opening folder dialog...")
                                folderDialog.open()
                            }
                        }
                        
                        Button {
                            Layout.fillWidth: true
                            text: "Rebuild Cache"
                            onClicked: {
                                if (window.currentPath !== "") {
                                    console.log("Rebuilding cache for: " + window.currentPath)
                                    if (viewLoader.item) {
                                        viewLoader.item.scanFolder(window.currentPath)
                                    }
                                } else {
                                    folderDialog.open()
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
                                    {name: "Direct3D 11", value: 2}, // QSGRendererInterface::Direct3D11
                                    {name: "Vulkan", value: 3},      // QSGRendererInterface::Vulkan
                                    {name: "OpenGL", value: 1},      // QSGRendererInterface::OpenGL
                                    {name: "Software", value: 5}     // QSGRendererInterface::Software
                                ]
                                
                                Text {
                                    text: modelData.name + ": " + (appSettings && appSettings.isApiSupported(modelData.value) ? "Available" : "Not Available")
                                    color: (appSettings && appSettings.isApiSupported(modelData.value)) ? "#8f8" : "#f88"
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    font.pixelSize: 14
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
                            text: "System Info"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 16
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Text {
                            text: "GPU: " + appSettings.getGpuName(window)
                            color: "#ccc"
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: 13
                        }

                        Text {
                            id: memUsageText
                            text: "Memory: Checking..."
                            color: "#ccc"
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: 13
                        }

                        Timer {
                            interval: 2000
                            running: true
                            repeat: true
                            onTriggered: {
                                memUsageText.text = "Memory: " + systemMonitor.memoryUsageMB.toFixed(1) + " MB"
                            }
                        }
                        
                        CheckBox {
                            text: "Show Performance Stats"
                            checked: statsOverlay.visible
                            onCheckedChanged: statsOverlay.visible = checked
                            Layout.alignment: Qt.AlignHCenter
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                leftPadding: parent.indicator.width + parent.spacing
                                verticalAlignment: Text.AlignVCenter
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
        isLoading: imageModel.isLoading
        loadedCount: imageModel.totalCount === 0 ? imageModel.scanProgress : imageModel.count
        totalCount: imageModel.totalCount
        activeThreadCount: appSettings ? appSettings.concurrentThreads : 0
        z: 100
    }

    // Photo Viewer Overlay (Full Screen)
    PhotoViewer {
        id: photoViewer
        anchors.fill: parent
        visible: false
        z: 10 // Ensure it's on top
        onBackClicked: {
            visible = false
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
}
