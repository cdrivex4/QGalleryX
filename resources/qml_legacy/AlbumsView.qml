import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGalleryX 1.0

StackView {
    id: stack
    initialItem: albumGridComponent
    clip: true
    
    signal imageClicked(int index, var model)
    
    property var model
    property real cellSize: 220 // Controllable from outside via slider
    property var activeModel: stack.depth === 1 ? model : (stack.currentItem ? stack.currentItem.activeModel : null)
    
    Component {
        id: albumGridComponent
        Item {
            GridView {
                id: grid
                anchors.fill: parent
                anchors.margins: 10
                clip: true
                model: stack.model
                
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                    active: true
                }
                
                cellWidth: stack.cellSize
                cellHeight: stack.cellSize + 40 // Extra space for name label
                
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
                                sourceSize: Qt.size(appSettings.thumbnailSize, appSettings.thumbnailSize)
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
                                        sourceSize: Qt.size(appSettings.thumbnailSize, appSettings.thumbnailSize)
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
            property var activeModel: innerGalleryLoader.item ? innerGalleryLoader.item.model : null
            
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
                            text: "< Back"
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
                    
                    Loader {
                        id: innerGalleryLoader
                        anchors.fill: parent
                        source: window.useTiles ? "GalleryViewTiles.qml" : "GalleryViewSemantic.qml"
                        
                        Binding {
                            target: innerGalleryLoader.item
                            property: "folderPath"
                            value: detailRoot.folderPath
                        }
                        Binding {
                            target: innerGalleryLoader.item
                            property: "groupingMode"
                            value: window.groupingMode
                        }
                    }
                    
                    Connections {
                        target: innerGalleryLoader.item
                        function onImageClicked(index) {
                            if (innerGalleryLoader.item && innerGalleryLoader.item.model) {
                                stack.imageClicked(index, innerGalleryLoader.item.model)
                            }
                        }
                    }
                }
            }
        }
    }
}
