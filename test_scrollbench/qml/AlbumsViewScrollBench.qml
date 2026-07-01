import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ScrollBenchBackend 1.0

Item {
    id: root
    signal imageClicked(int index, var model)
    
    property var model: albumModel
    
    // DEBOUNCED loading resolution
    property int loadingResolution: settings.thumbnailSize
    Timer {
        id: resolutionDebounce
        interval: 400
        repeat: false
        onTriggered: root.loadingResolution = settings.thumbnailSize
    }
    Connections {
        target: settings
        function onThumbnailSizeChanged() { resolutionDebounce.restart() }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: albumGridComponent
    }
    
    Component {
        id: albumGridComponent
        Item {
            GridView {
                id: grid
                anchors.fill: parent
                anchors.margins: 10
                model: root.model
                
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                    active: true
                }
                
                cellWidth: 220
                cellHeight: 260
                
                delegate: Item {
                    width: grid.cellWidth
                    height: grid.cellHeight
                    
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 10
                        color: "#222"
                        radius: 8
                        
                        Rectangle {
                            id: coverContainer
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: width
                            color: "#333"
                            radius: 8
                            clip: true

                            // Single Image
                            Image {
                                anchors.fill: parent
                                visible: coverPath && coverPath.length === 1
                                source: (visible && coverPath.length > 0) ? "image://async/" + coverPath[0] : ""
                                sourceSize: Qt.size(root.loadingResolution, root.loadingResolution)
                                asynchronous: true
                                fillMode: Image.PreserveAspectCrop
                            }

                            // Grid (2+ images)
                            Grid {
                                anchors.fill: parent
                                columns: 2
                                visible: coverPath && coverPath.length >= 2
                                
                                Repeater {
                                    model: (coverPath && coverPath.length >= 2) ? coverPath : 0
                                    Image {
                                        width: coverContainer.width / 2
                                        height: coverContainer.height / 2
                                        source: "image://async/" + modelData
                                        sourceSize: Qt.size(root.loadingResolution, root.loadingResolution)
                                        asynchronous: true
                                        fillMode: Image.PreserveAspectCrop
                                    }
                                }
                            }
                        }
                        
                        Column {
                            anchors.top: coverContainer.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.margins: 8
                            spacing: 4
                            
                            Text {
                                text: name
                                color: "white"
                                font.bold: true
                                elide: Text.ElideRight
                                width: parent.width
                            }
                            
                            Text {
                                text: count + " items"
                                color: "#aaa"
                                font.pixelSize: 12
                            }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                stack.push(albumDetailComponent, { folderPath: path, albumName: name })
                            }
                        }
                    }
                }
            }
            
            Text {
                anchors.centerIn: parent
                text: "No Albums Found"
                color: "#666"
                visible: grid.count === 0
                font.pixelSize: 20
            }
        }
    }
    
    Component {
        id: albumDetailComponent
        Item {
            id: detailRoot
            property string folderPath
            property string albumName
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                
                // Header
                Rectangle {
                    Layout.fillWidth: true
                    height: 50
                    color: "#222"
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10
                        
                        Button {
                            text: "← Back"
                            onClicked: stack.pop()
                        }
                        
                        Text {
                            text: albumName
                            color: "white"
                            font.bold: true
                            font.pixelSize: 18
                        }
                    }
                }
                
                // Content
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    
                    // Album Setting State
                    property bool useSemanticView: true
                    
                    GalleryViewScrollBench {
                        id: albumGallery
                        anchors.fill: parent
                        model: albumImageModel
                        visible: !parent.useSemanticView
                        onImageClicked: (index) => root.imageClicked(index, albumImageModel)
                        
                        Component.onCompleted: albumImageModel.scanDirectory(detailRoot.folderPath)
                    }
                    
                    GalleryViewSemanticScrollBench {
                        id: albumSemanticGallery
                        anchors.fill: parent
                        model: albumImageModel
                        visible: parent.useSemanticView
                        onImageClicked: (index) => root.imageClicked(index, albumImageModel)
                        
                        Component.onCompleted: albumImageModel.scanDirectory(detailRoot.folderPath)
                    }
                    
                    DateScrubber {
                        id: albumScrubber
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.topMargin: 50
                        anchors.bottomMargin: 50
                        width: 150
                        z: 60
                        
                        listView: {
                            if (parent.useSemanticView) return albumSemanticGallery.findChildListView()
                            return albumGallery.findChildGridView()
                        }
                        rawModel: !parent.useSemanticView ? albumImageModel : null
                        proxyModel: parent.useSemanticView ? albumSemanticGallery.proxyModel : null
                    }
                    
                    ScrollBenchImageModel {
                        id: albumImageModel
                        filterQuery: imageModel.filterQuery
                        // Connect to frame budget for consistent performance
                        Component.onCompleted: setFrameScheduler(frameBudget)
                    }
                    
                    SelectionActionBar {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        model: albumImageModel
                        visible: model.selectedCount > 0
                        
                        onClearClicked: model.clearSelection()
                        onShareClicked: albumShareDialog.open()
                    }
                    
                    ShareDialog {
                        id: albumShareDialog
                        anchors.centerIn: parent
                    }

                    // Floating Controls (Bottom Right)
                    ColumnLayout {
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        anchors.margins: 25
                        anchors.rightMargin: 180 // Avoid scrubber
                        anchors.bottomMargin: 80 // Avoid action bar
                        spacing: 12
                        
                        Button {
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 45
                            text: parent.parent.useSemanticView ? "View: Semantic" : "View: Standard Grid"
                            onClicked: parent.parent.useSemanticView = !parent.parent.useSemanticView
                            
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
                                if (!albumSemanticGallery) return ""
                                if (albumSemanticGallery.groupingAuto) return "Grouping: Auto"
                                if (albumSemanticGallery.groupingMode === 1) return "Grouping: Day"
                                if (albumSemanticGallery.groupingMode === 2) return "Grouping: Week"
                                if (albumSemanticGallery.groupingMode === 3) return "Grouping: Month"
                                if (albumSemanticGallery.groupingMode === 4) return "Grouping: Year"
                                return "Grouping"
                            }
                            visible: parent.parent.useSemanticView
                            onClicked: {
                                if (!albumSemanticGallery) return
                                if (albumSemanticGallery.groupingAuto) {
                                    albumSemanticGallery.groupingAuto = false
                                    albumSemanticGallery.groupingMode = 1 // Start at Day
                                } else if (albumSemanticGallery.groupingMode === 1) {
                                    albumSemanticGallery.groupingMode = 2 // Week
                                } else if (albumSemanticGallery.groupingMode === 2) {
                                    albumSemanticGallery.groupingMode = 3 // Month
                                } else if (albumSemanticGallery.groupingMode === 3) {
                                    albumSemanticGallery.groupingMode = 4 // Year
                                } else {
                                    albumSemanticGallery.groupingAuto = true // Return to Auto
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
            }
        }
    }
}
