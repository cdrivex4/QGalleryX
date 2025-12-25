import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    width: 1280
    height: 800
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
    
    property alias semanticView: semanticView

    FrameAnimation {
        running: true
        onTriggered: telemetry.recordFrame()
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
                        root.viewerModel = imageModel
                        root.viewerIndex = index
                        root.viewerVisible = true
                    }
                }

                GalleryViewSemanticScrollBench {
                    id: semanticView
                    anchors.fill: parent
                    visible: root.useSemanticView
                    onImageClicked: (index) => {
                        root.viewerModel = imageModel
                        root.viewerIndex = index
                        root.viewerVisible = true
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
                    root.viewerModel = model
                    root.viewerIndex = index
                    root.viewerVisible = true
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

    // Floating Performance Stats (Top Left)
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 20
        width: 220
        height: 100
        color: "#AA000000"
        radius: 8
        z: 50
        visible: !root.viewerVisible && !overlayVisible

        ColumnLayout {
            anchors.centerIn: parent
            Text {
                color: "#ffffff"; font.pixelSize: 13; font.bold: true
                text: "FPS: " + telemetry.fps + " | Mem: " + systemMonitor.memoryUsageMB.toFixed(0) + "MB"
            }
            Text {
                color: "#aaa"; font.pixelSize: 11
                text: "Range: " + imageModel.visibleStartIndex + "-" + imageModel.visibleEndIndex
            }
            Text {
                color: "#aaa"; font.pixelSize: 11
                text: "Queue: " + (taskScheduler ? taskScheduler.activeTaskCount : "0")
            }
            Text {
                color: "#ccc"; font.pixelSize: 11; font.bold: true
                text: "Zoom: " + settings.gridSize + "px | Res: " + settings.thumbnailSize + "px"
            }
        }
    }

    // Scan Folder Button (Top Right)
    Button {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        text: "📁 Scan Folder"
        z: 50
        onClicked: folderDialog.open()
        visible: !root.viewerVisible && !overlayVisible
        
        background: Rectangle { color: "#222"; radius: 5; border.color: "#444" }
        contentItem: Text { text: parent.text; color: "white"; padding: 8 }
    }

    FolderDialog {
        id: folderDialog
        title: "Select Image Folder"
        onAccepted: {
            var rawPath = selectedFolder.toString()
            imageModel.clearData()
            imageModel.scanDirectory(rawPath)
            albumModel.scanAlbums(rawPath)
        }
    }

    // Settings / Performance Overlay Drawer
    Rectangle {
        id: perfOverlay
        width: 400
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
        currentIndex: root.viewerIndex
        onBackClicked: root.viewerVisible = false
        onVisibleChanged: { if (visible) photoViewer.forceActiveFocus() }
        onCurrentIndexChanged: root.viewerIndex = currentIndex
        z: 200
    }
}