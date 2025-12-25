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
    property int currentView: 0 // 0: Grid, 1: Albums, 2: Semantic
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
            currentIndex: root.currentView

            // View 0: Standard GridView
            GalleryViewScrollBench {
                id: standardView
                onImageClicked: (index) => {
                    root.viewerModel = imageModel
                    root.viewerIndex = index
                    root.viewerVisible = true
                }
            }

            // View 1: Albums
            AlbumsViewScrollBench {
                id: albumsView
                onImageClicked: (index, model) => {
                    root.viewerModel = model
                    root.viewerIndex = index
                    root.viewerVisible = true
                }
            }

            // View 2: Semantic Grouping
            GalleryViewSemanticScrollBench {
                id: semanticView
                onImageClicked: (index) => {
                    root.viewerModel = imageModel
                    root.viewerIndex = index
                    root.viewerVisible = true
                }
            }
        }

        // BottomBar
        Rectangle {
            id: bottomBar
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#1e1e1e"
            visible: !root.viewerVisible

            RowLayout {
                anchors.centerIn: parent
                spacing: 20

                Button {
                    text: "Standard Grid"
                    highlighted: root.currentView === 0
                    onClicked: root.currentView = 0
                }
                Button {
                    text: "Albums"
                    highlighted: root.currentView === 1
                    onClicked: root.currentView = 1
                }
                Button {
                    text: "Semantic Zoom"
                    highlighted: root.currentView === 2
                    onClicked: root.currentView = 2
                }

                // Grouping Modes (only for Semantic view)
                RowLayout {
                    visible: root.currentView === 2
                    spacing: 5
                    Rectangle { width: 1; height: 30; color: "#444"; Layout.leftMargin: 10; Layout.rightMargin: 10 }
                    
                    Text { text: "Group By:"; color: "#888"; font.pixelSize: 12 }
                    
                    Button { 
                        text: "Auto"; flat: true; 
                        highlighted: root.semanticView && root.semanticView.groupingAuto
                        onClicked: if (root.semanticView) root.semanticView.groupingAuto = true
                    }
                    Button { 
                        text: "Day"; flat: true; 
                        highlighted: root.semanticView && !root.semanticView.groupingAuto && root.semanticView.groupingMode === 1
                        onClicked: if (root.semanticView) { root.semanticView.groupingAuto = false; root.semanticView.groupingMode = 1 }
                    }
                    Button { 
                        text: "Week"; flat: true; 
                        highlighted: root.semanticView && !root.semanticView.groupingAuto && root.semanticView.groupingMode === 2
                        onClicked: if (root.semanticView) { root.semanticView.groupingAuto = false; root.semanticView.groupingMode = 2 }
                    }
                    Button { 
                        text: "Month"; flat: true; 
                        highlighted: root.semanticView && !root.semanticView.groupingAuto && root.semanticView.groupingMode === 3
                        onClicked: if (root.semanticView) { root.semanticView.groupingAuto = false; root.semanticView.groupingMode = 3 }
                    }
                    Button { 
                        text: "Year"; flat: true; 
                        highlighted: root.semanticView && !root.semanticView.groupingAuto && root.semanticView.groupingMode === 4
                        onClicked: if (root.semanticView) { root.semanticView.groupingAuto = false; root.semanticView.groupingMode = 4 }
                    }
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
        anchors.bottomMargin: 80
        width: 150
        z: 60
        visible: !root.viewerVisible && (root.currentView === 0 || root.currentView === 2)
        
        listView: {
            if (root.currentView === 0) return standardView.findChildGridView()
            if (root.currentView === 2) return semanticView.findChildListView()
            return null
        }
        
        proxyModel: root.currentView === 2 ? semanticView.proxyModel : null
        rawModel: root.currentView === 0 ? imageModel : null
    }

    // Floating Info Label
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 20
        width: infoText.width + 20
        height: infoText.height + 20
        color: "#aa000000"
        radius: 5
        z: 50
        visible: !root.viewerVisible && !overlayVisible

        Text {
            id: infoText
            anchors.centerIn: parent
            color: "#ffffff"
            font.pixelSize: 14
            font.bold: true
            text: "ScrollBench LEAD | " + telemetry.fps + " FPS\n" +
                  imageModel.totalItems + " items | Remaining: " + imageModel.remainingItems + " | Queue: " + (taskScheduler ? taskScheduler.activeTaskCount : "0") + "\n" +
                  "Range: " + imageModel.visibleStartIndex + "-" + imageModel.visibleEndIndex + "\n" +
                  "Grid: " + settings.gridSize + "px | Res: " + settings.thumbnailSize + "px"
            horizontalAlignment: Text.AlignHCenter
        }
    }

    // Folder Selection
    Button {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        text: "📁 Scan Folder"
        z: 50
        onClicked: folderDialog.open()
        visible: !root.viewerVisible && !overlayVisible
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

    // Performance Overlay (Left Drawer)
    Rectangle {
        id: perfOverlay
        width: 380
        height: parent.height
        x: overlayVisible ? 0 : -width
        y: 0
        z: 100
        color: "#252525"
        border.color: "#404040"
        border.width: 1
        clip: true
        Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }
        PerformanceOverlay { anchors.fill: parent }
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

    // Toggle Button
    Rectangle {
        id: toggleButton
        width: 40
        height: 80
        x: overlayVisible ? perfOverlay.width : 0
        y: (parent.height - height) / 2
        z: 101
        color: "#ffffff"
        opacity: overlayVisible ? 0.8 : 0.4
        radius: 5
        visible: !root.viewerVisible
        Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }
        Behavior on opacity { NumberAnimation { duration: 150 } }
        Text { anchors.centerIn: parent; text: overlayVisible ? "◀" : "⚙️"; color: "#000000"; font.pixelSize: 20; font.bold: true }
        MouseArea { anchors.fill: parent; onClicked: root.overlayVisible = !root.overlayVisible; cursorShape: Qt.PointingHandCursor }
    }
}