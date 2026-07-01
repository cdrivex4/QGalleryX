import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.settings

ApplicationWindow {
    id: root
    visible: true
    title: "ScrollBench Lead Development Environment"
    color: "#121212"

    // View States
    property int currentTab: 0 // 0: Pictures, 1: Albums
    property bool useSemanticView: true
    
    property bool overlayVisible: false
    property bool viewerVisible: false
    property int viewerIndex: 0
    property var viewerModel: imageModel
    
    Settings {
        id: localSettings
        category: "ScrollBench"
        property string lastFolder: ""
        property int windowWidth: 1280
        property int windowHeight: 800
        property int windowX: 100
        property int windowY: 100
    }
    
    width: localSettings.windowWidth
    height: localSettings.windowHeight
    x: localSettings.windowX
    y: localSettings.windowY
    
    onWidthChanged: if (visible) localSettings.windowWidth = width
    onHeightChanged: if (visible) localSettings.windowHeight = height
    onXChanged: if (visible) localSettings.windowX = x
    onYChanged: if (visible) localSettings.windowY = y
    
    function openViewer(index, model) {
        root.viewerModel = model
        root.viewerIndex = index
        photoViewer.currentIndex = index
        root.viewerVisible = true
    }
    
    property alias semanticView: semanticView
    
    // Finalized Layout Values
    property int overlayWidth: 390
    property int contentPadding: 66

    // Debug Grouping Thresholds
    property int thresholdYear: 65
    property int thresholdMonth: 105
    property int thresholdWeek: 165

    FrameAnimation {
        running: true
        onTriggered: telemetry.recordFrame()
    }

    Component.onCompleted: {
        if (localSettings.lastFolder !== "") {
            console.log("[ScrollBench] Restoring last folder:", localSettings.lastFolder)
            imageModel.scanDirectory(localSettings.lastFolder)
            albumModel.scanAlbums(localSettings.lastFolder)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        StackLayout {
            id: mainStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentTab

            // Tab 0: Pictures (Toggle between Grid and Semantic)
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                GalleryViewScrollBench {
                    id: standardView
                    anchors.fill: parent
                    visible: !root.useSemanticView
                    onImageClicked: (index) => {
                        root.openViewer(index, imageModel)
                    }
                }

                GalleryViewSemanticScrollBench {
                    id: semanticView
                    anchors.fill: parent
                    visible: root.useSemanticView
                    onImageClicked: (index) => {
                        root.openViewer(index, imageModel)
                    }
                }
                
                // Floating Controls (Bottom Right)
                ColumnLayout {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.margins: 25
                    anchors.bottomMargin: 80 // Above bottom bar
                    spacing: 12
                    visible: !root.viewerVisible
                    
                    Button {
                        Layout.preferredWidth: 160
                        Layout.preferredHeight: 45
                        text: root.useSemanticView ? "View: Semantic" : "View: Standard Grid"
                        onClicked: root.useSemanticView = !root.useSemanticView
                        
                        background: Rectangle {
                            color: "#333333"; radius: 25
                            border.color: "#444444"; border.width: 1
                            opacity: parent.pressed ? 0.7 : 0.9
                        }
                        contentItem: Text {
                            text: parent.text; color: "white"; font.bold: true
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        }
                    }
                    
                    Button {
                        Layout.preferredWidth: 160
                        Layout.preferredHeight: 45
                        text: {
                            if (!semanticView) return ""
                            if (semanticView.groupingAuto) return "Grouping: Auto"
                            if (semanticView.groupingMode === 1) return "Grouping: Day"
                            if (semanticView.groupingMode === 2) return "Grouping: Week"
                            if (semanticView.groupingMode === 3) return "Grouping: Month"
                            if (semanticView.groupingMode === 4) return "Grouping: Year"
                            if (semanticView.groupingMode === 5) return "Grouping: Type"
                            return "Grouping"
                        }
                        visible: root.useSemanticView
                        onClicked: {
                            if (!semanticView) return
                            if (semanticView.groupingAuto) {
                                semanticView.groupingAuto = false
                                semanticView.groupingMode = 1 // Start at Day
                            } else if (semanticView.groupingMode === 1) {
                                semanticView.groupingMode = 2 // Week
                            } else if (semanticView.groupingMode === 2) {
                                semanticView.groupingMode = 3 // Month
                            } else if (semanticView.groupingMode === 3) {
                                semanticView.groupingMode = 4 // Year
                            } else if (semanticView.groupingMode === 4) {
                                semanticView.groupingMode = 5 // Type
                            } else {
                                semanticView.groupingAuto = true // Return to Auto
                            }
                        }
                        
                        background: Rectangle {
                            color: "#333333"; radius: 25
                            border.color: "#444444"; border.width: 1
                            opacity: parent.pressed ? 0.7 : 0.9
                        }
                        contentItem: Text {
                            text: parent.text; color: "white"; font.bold: true
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            // Tab 1: Albums
            AlbumsViewScrollBench {
                id: albumsView
                onImageClicked: (index, model) => {
                    root.openViewer(index, model)
                }
            }
        }

        // Bottom Navigation Bar
        Rectangle {
            id: bottomBar
            Layout.fillWidth: true
            Layout.preferredHeight: 65
            color: "#181818"
            visible: !root.viewerVisible
            
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#222" }

            RowLayout {
                anchors.fill: parent
                spacing: 0
                
                Item {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    ColumnLayout {
                        anchors.centerIn: parent; spacing: 4
                        Text { text: "🖼️"; font.pixelSize: 22; opacity: root.currentTab === 0 ? 1 : 0.4; Layout.alignment: Qt.AlignHCenter }
                        Text { text: "Pictures"; color: root.currentTab === 0 ? "#2196F3" : "#888"; font.pixelSize: 11; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                    }
                    MouseArea { anchors.fill: parent; onClicked: root.currentTab = 0 }
                }
                
                Item {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    ColumnLayout {
                        anchors.centerIn: parent; spacing: 4
                        Text { text: "📁"; font.pixelSize: 22; opacity: root.currentTab === 1 ? 1 : 0.4; Layout.alignment: Qt.AlignHCenter }
                        Text { text: "Albums"; color: root.currentTab === 1 ? "#2196F3" : "#888"; font.pixelSize: 11; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                    }
                    MouseArea { anchors.fill: parent; onClicked: root.currentTab = 1 }
                }
                
                Item {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    ColumnLayout {
                        anchors.centerIn: parent; spacing: 4
                        Text { text: "⚙️"; font.pixelSize: 22; opacity: root.overlayVisible ? 1 : 0.4; Layout.alignment: Qt.AlignHCenter }
                        Text { text: "Settings"; color: root.overlayVisible ? "#2196F3" : "#888"; font.pixelSize: 11; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                    }
                    MouseArea { anchors.fill: parent; onClicked: root.overlayVisible = !root.overlayVisible }
                }
            }
        }
    }
    
    // Global Selection Action Bar (For Pictures Tab)
    SelectionActionBar {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        z: 100 // Above bottom bar
        
        model: imageModel
        visible: model.selectedCount > 0 && root.currentTab === 0 && !root.viewerVisible
        
        onClearClicked: model.clearSelection()
        onRotateClicked: model.rotateSelected(90)
        onShareClicked: {
            shareDialog.targetPaths = []
            shareDialog.open()
        }
    }
    
    ShareDialog {
        id: shareDialog
        anchors.centerIn: parent
    }

    // Date Scrubber Overlay
    DateScrubber {
        id: scrubber
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 100
        anchors.bottomMargin: 100
        width: 150
        z: 60
        visible: !root.viewerVisible && root.currentTab === 0
        
        listView: {
            if (root.useSemanticView) return semanticView.findChildListView()
            return standardView.findChildGridView()
        }
        
        proxyModel: root.useSemanticView ? semanticView.proxyModel : null
        rawModel: !root.useSemanticView ? imageModel : null
    }

    // Top Right Controls (Search & Scan)
    RowLayout {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        spacing: 15
        z: 50
        visible: !root.viewerVisible && !overlayVisible

        // Legacy vs New Rendering Toggle Removed from Header

        // Global Search Bar
        TextField {
            id: searchField
            Layout.preferredWidth: 250
            Layout.preferredHeight: 35
            placeholderText: "🔍 Filter filenames or folders..."
            color: "white"
            placeholderTextColor: "#888888"
            
            background: Rectangle {
                color: "#222222"
                radius: 4
                border.color: parent.activeFocus ? "#4A90E2" : "#444444"
                border.width: 1
            }
            
            Button {
                anchors.right: parent.right
                anchors.rightMargin: 5
                anchors.verticalCenter: parent.verticalCenter
                width: 25; height: 25
                text: "✕"
                flat: true
                visible: searchField.text.length > 0
                onClicked: searchField.text = ""
                contentItem: Text {
                    text: parent.text
                    color: parent.hovered ? "#ff5252" : "#888"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 14
                    font.bold: true
                }
            }
            
            onTextChanged: {
                imageModel.filterQuery = text
                albumModel.filterQuery = text
                if (text.length > 0) {
                    var dirs = imageModel.getActiveDirectories();
                    console.log("[QML Filter] text: '" + text + "', activeDirs: " + dirs);
                    albumModel.applyFilterFromPaths(dirs)
                }
            }
        }

        Connections {
            target: albumModel
            function onFilterNeedsReapply() {
                if (searchField.text.length > 0) {
                    var dirs = imageModel.getActiveDirectories();
                    albumModel.applyFilterFromPaths(dirs)
                }
            }
        }

        // Scan Folder Button
        Button {
            Layout.preferredWidth: 140
            Layout.preferredHeight: 35
            text: "📁 Scan Folder"
            
            background: Rectangle {
                color: "#2A2A2A"
                radius: 4
                border.color: "#444444"
            }
            contentItem: Text {
                text: parent.text
                color: "white"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: folderDialog.open()
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Select Image Folder"
        onAccepted: {
            var rawPath = selectedFolder.toString()
            imageModel.clearData()
            imageModel.scanDirectory(rawPath)
            albumModel.scanAlbums(rawPath)
            
            var path = rawPath.replace(/^(file:\/{3})|(file:)/, "")
            if (Qt.platform.os === "windows") {
                if (path.startsWith("/") && path.indexOf(":") === 2) path = path.substring(1);
            }
            localSettings.lastFolder = path
        }
    }



    // Settings / Performance Overlay Drawer
    Rectangle {
        id: perfOverlay
        width: root.overlayWidth
        height: parent.height
        x: overlayVisible ? parent.width - width : parent.width
        y: 0
        z: 100
        color: "#1e1e1e"
        border.color: "#333"
        border.width: 1
        visible: x < parent.width
        
        Behavior on x { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
        
        PerformanceOverlay { 
            anchors.fill: parent 
            onCloseRequested: root.overlayVisible = false
        }
    }

    // Photo Viewer
    PhotoViewerScrollBench {
        id: photoViewer
        anchors.fill: parent
        visible: root.viewerVisible
        model: root.viewerModel
        // Removed currentIndex binding to prevent destruction on swipe
        onBackClicked: {
            root.viewerIndex = -1 // Reset local tracking
            root.viewerVisible = false
        }
        onVisibleChanged: { if (visible) photoViewer.forceActiveFocus() }
        onCurrentIndexChanged: {
            if (visible && currentIndex >= 0) {
                root.viewerIndex = currentIndex
            }
        }
        z: 200
    }
    
    // Real-time Diagnostics Overlay
    DiagnosticsOverlay {
        id: diagnosticsOverlay
        visible: settings.showDiagnostics
        
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10 // Base margin
        
        anchors.topMargin: 134
        anchors.leftMargin: 22
        
        popupPadding: 22
        popupWidth: 400
        
        width: expanded ? 450 : 300
        height: expanded ? 500 : 45
        z: 300
    }

    // SCANNING PROGRESS INDICATOR (Especially for Network Folders)
    Rectangle {
        id: scanIndicator
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 100
        height: 40
        width: scanInfoRow.width + 40
        color: "#AA000000"
        radius: 20
        border.color: "#444"
        visible: imageModel.isLoading
        z: 150

        Row {
            id: scanInfoRow
            anchors.centerIn: parent
            spacing: 12

            BusyIndicator {
                width: 24; height: 24
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "Scanning folder: <b>" + imageModel.scannedCount + "</b> items discovered..."
                color: "white"
                font.pixelSize: 13
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}

