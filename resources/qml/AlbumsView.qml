import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGalleryX 1.0

StackView {
    id: stack
    initialItem: albumGridComponent
    
    signal imageClicked(int index, var model)
    
    property var model
    
    Component {
        id: albumGridComponent
        Item {
            GridView {
                id: grid
                anchors.fill: parent
                anchors.margins: 10
                model: stack.model
                
                ScrollBar.vertical: ScrollBar {
                    id: scrollBar
                    policy: ScrollBar.AsNeeded
                    active: true
                }
                
                cellWidth: 220
                cellHeight: 280
                
                flow: GridView.FlowLeftToRight
                clip: true
                
                delegate: AlbumCard {
                    name: model.name
                    path: model.path
                    coverPaths: model.coverPath
                    count: model.count
                    
                    onClicked: {
                        stack.push(albumDetailComponent, { 
                            folderPath: model.path, 
                            albumName: model.name 
                        })
                    }
                }

                // Add animations for model changes
                add: Transition {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 300 }
                }
            }
            
            ColumnLayout {
                anchors.centerIn: parent
                visible: grid.count === 0 && !stack.model.isLoading
                spacing: 15
                
                Text {
                    text: "📁"
                    font.pixelSize: 64
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Text {
                    text: "No Albums Found"
                    color: "#888"
                    font.pixelSize: 20
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Text {
                    text: "Choose a different folder to see your memories"
                    color: "#666"
                    font.pixelSize: 14
                    Layout.alignment: Qt.AlignHCenter
                }
            }
            
            BusyIndicator {
                anchors.centerIn: parent
                running: stack.model.isLoading && grid.count === 0
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
                    height: 60
                    color: "black"
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 15
                        
                        ToolButton {
                            text: "←"
                            font.pixelSize: 24
                            onClicked: stack.pop()
                        }
                        
                        Column {
                            Layout.fillWidth: true
                            Text {
                                text: detailRoot.albumName
                                color: "white"
                                font.bold: true
                                font.pixelSize: 20
                            }
                            Text {
                                text: detailRoot.folderPath
                                color: "#888"
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                                width: parent.width - 100
                            }
                        }
                    }
                    
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: "#333"
                    }
                }
                
                // Content
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    
                    GalleryViewTiles {
                        anchors.fill: parent
                        folderPath: detailRoot.folderPath
                        onImageClicked: (index) => stack.imageClicked(index, model)
                    }
                }
            }
        }
    }
}
