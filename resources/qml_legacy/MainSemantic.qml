import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.settings
import Qt.labs.platform
import QGalleryXTest 1.0

ApplicationWindow {
    id: window
    width: 1280
    height: 720
    visible: true
    title: qsTr("Samsung Gallery Clone (Semantic Zoom Test)")
    color: "#000000"

    property string currentPath: ""

    // Persistence
    Settings {
        id: settings
        property string lastFolder: ""
        property int graphicsApi: 0
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
            // galleryViewItem.scanFolder(settings.lastFolder) // Handled by binding
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        StackLayout {
            id: mainLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: bottomBar.currentIndex
            visible: !photoViewer.visible

            // Tab 0: Pictures
            Item {
                id: galleryTab
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                property bool useTiles: false
                
                Loader {
                    id: viewLoader
                    anchors.fill: parent
                    sourceComponent: galleryTab.useTiles ? tilesViewComponent : semanticViewComponent
                    
                    Binding {
                        target: viewLoader.item
                        property: "folderPath"
                        value: window.currentPath
                    }
                }
                
                Component {
                    id: semanticViewComponent
                    GalleryViewSemantic {
                        id: galleryViewSemantic
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
                        onImageClicked: (index) => {
                            photoViewer.model = galleryViewTiles.model
                            photoViewer.currentIndex = index
                            photoViewer.visible = true
                        }
                    }
                }
                
                // Floating Controls
                ColumnLayout {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.margins: 20
                    anchors.bottomMargin: 80 // Above bottom bar
                    z: 99
                    spacing: 10
                    width: 150 // Set a fixed width for consistent sizing
                    
                    Button {
                        Layout.fillWidth: true
                        text: galleryTab.useTiles ? "View: Tiles" : "View: Semantic"
                        onClicked: galleryTab.useTiles = !galleryTab.useTiles
                        
                        background: Rectangle {
                            color: "#333"
                            radius: 5
                            border.color: "#666"
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            padding: 10
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                    
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["Auto", "Day", "Week", "Month", "Year"]
                        currentIndex: 0
                        visible: !galleryTab.useTiles
                        onCurrentIndexChanged: {
                            if (viewLoader.item) {
                                viewLoader.item.groupingMode = currentIndex
                            }
                        }
                        
                        background: Rectangle {
                            color: "#333"
                            radius: 5
                            border.color: "#666"
                        }
                        contentItem: Text {
                            text: parent.displayText
                            color: "white"
                            padding: 10
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                        delegate: ItemDelegate {
                            width: parent.width
                            contentItem: Text {
                                text: modelData
                                color: "white"
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                            }
                            background: Rectangle {
                                color: parent.highlighted ? "#555" : "#333"
                            }
                        }
                    }
                }
            }

            // Tab 1: Albums
            AlbumsView {
                // Placeholder
            }

            // Tab 2: Stories
            Item {
                Text {
                    anchors.centerIn: parent
                    text: "Stories Feature Coming Soon"
                    color: "white"
                }
            }

            // Tab 3: Menu
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
                                            id: memUsageTextTopSem
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
                                    id: statsCheckboxTopSem
                                    checked: statsOverlay.visible
                                    onCheckedChanged: statsOverlay.visible = checked
                                    Layout.alignment: Qt.AlignVCenter
                                    contentItem: Text {
                                        text: "Performance\nStats Overlay"
                                        color: statsCheckboxTopSem.checked ? "#00FF00" : "#ccc"
                                        font.pixelSize: 11
                                        font.bold: true
                                        leftPadding: parent.indicator.width + parent.spacing
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }
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
                                appSettings.selectedApi = index
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
                                    text: modelData.name + ": " + ((appSettings && appSettings.isApiSupported(modelData.value)) ? "Available" : "Not Available")
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
            onTabSelected: (index) => mainLayout.currentIndex = index
        }
    }
    
    MessageDialog {
        id: restartDialog
        title: "Restart Required"
        text: "The application needs to restart to apply the graphics API change."
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: appSettings.restartApp()
    }

    // Stats Overlay
    StatsOverlay {
        id: statsOverlay
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        apiName: appSettings.graphicsApi
        scanEngine: imageModel ? imageModel.scanMethod : "Idle"
        scanDuration: imageModel ? imageModel.scanDurationMs : 0
        isLoading: imageModel ? imageModel.isLoading : false
        loadedCount: imageModel ? (imageModel.totalCount === 0 ? imageModel.scanProgress : imageModel.count) : 0
        totalCount: imageModel ? imageModel.totalCount : 0
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
                if (semanticView && typeof semanticView.scanFolder === "function") {
                    semanticView.scanFolder(window.currentPath)
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
            visible = false
        }
        
        // Pass stats to overlay
        onImageLoaded: (timeMs) => statsOverlay.reportLoadTime(timeMs)
    }
}
