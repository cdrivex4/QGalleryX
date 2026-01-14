import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ScrollBenchBackend 1.0

Item {
    id: root
    signal imageClicked(int index, var model)
    
    property var model: albumModel
    
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
                                sourceSize: Qt.size(settings.thumbnailSize, settings.thumbnailSize)
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
                                        sourceSize: Qt.size(settings.thumbnailSize, settings.thumbnailSize)
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
                    
                    GalleryViewScrollBench {
                        id: albumGallery
                        anchors.fill: parent
                        model: albumImageModel
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
                        
                        listView: albumGallery.findChildGridView()
                        rawModel: albumImageModel
                        proxyModel: null // No semantic zoom in album detail yet
                    }
                    
                    ScrollBenchImageModel {
                        id: albumImageModel
                        // Connect to frame budget for consistent performance
                        Component.onCompleted: setFrameScheduler(frameBudget)
                    }
                    
                    SelectionActionBar {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        model: albumImageModel
                        
                        onClearClicked: model.clearSelection()
                        onShareClicked: albumShareDialog.open()
                    }
                    
                    ShareDialog {
                        id: albumShareDialog
                        anchors.centerIn: parent
                        // Bind to local model? ShareDialog uses global imageModel by default?
                        // ShareDialog.qml needs modification to support injected model or we use property binding alias
                    }
                }
            }
        }
    }
}
