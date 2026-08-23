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
    property real cellSize: 100 // Controllable from outside via slider
    property var activeModel: stack.depth === 1 ? model : (stack.currentItem ? stack.currentItem.activeModel : null)
    property var activeGrid: stack.currentItem ? (stack.currentItem.grid || stack.currentItem.activeGrid) : null
    
    Component {
        id: albumGridComponent
        Item {
            property alias grid: grid
            GridView {
                id: grid
                anchors.fill: parent
                anchors.margins: 10
                clip: true
                model: stack.model
                focus: true
                keyNavigationEnabled: false

                Component.onCompleted: grid.forceActiveFocus()

                Keys.onPressed: (event) => {
                    var cols = Math.max(1, Math.floor(grid.width / grid.cellWidth))
                    var rowsPerPage = Math.max(1, Math.floor(grid.height / grid.cellHeight))
                    var pageStep = rowsPerPage * cols
                    var count = grid.count

                    if (count === 0) return
                    if (grid.currentIndex < 0) grid.currentIndex = 0

                    if (event.key === Qt.Key_Left) {
                        grid.currentIndex = Math.max(0, grid.currentIndex - 1)
                        grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Right) {
                        grid.currentIndex = Math.min(count - 1, grid.currentIndex + 1)
                        grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Up) {
                        grid.currentIndex = Math.max(0, grid.currentIndex - cols)
                        grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Down) {
                        grid.currentIndex = Math.min(count - 1, grid.currentIndex + cols)
                        grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        event.accepted = true
                    } else if (event.key === Qt.Key_PageUp) {
                        grid.currentIndex = Math.max(0, grid.currentIndex - pageStep)
                        grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        event.accepted = true
                    } else if (event.key === Qt.Key_PageDown) {
                        grid.currentIndex = Math.min(count - 1, grid.currentIndex + pageStep)
                        grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Home) {
                        grid.currentIndex = 0
                        grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        event.accepted = true
                    } else if (event.key === Qt.Key_End) {
                        grid.currentIndex = count - 1
                        grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                        if (grid.currentIndex >= 0 && grid.currentIndex < count) {
                            var targetPath = grid.currentItem ? grid.currentItem.albumPath : ""
                            var targetName = grid.currentItem ? grid.currentItem.albumName : ""
                            if (targetPath) {
                                stack.push(albumDetailComponent, { folderPath: targetPath, albumName: targetName })
                            }
                        }
                        event.accepted = true
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                    active: true
                }

                cellWidth: stack.cellSize
                cellHeight: stack.cellSize + 40 // Extra space for name label

                delegate: Item {
                    width: grid.cellWidth
                    height: grid.cellHeight
                    property string albumPath: path
                    property string albumName: name

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 10
                        color: index === grid.currentIndex ? "#2A3B55" : "#222"
                        radius: 8
                        border.color: index === grid.currentIndex ? "#38BDF8" : "transparent"
                        border.width: index === grid.currentIndex ? 3 : 0

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: -2
                            color: "transparent"
                            border.color: "#FFFFFF"
                            border.width: 1
                            radius: 10
                            opacity: 0.6
                            visible: index === grid.currentIndex
                        }

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
                                cache: true
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
                                        cache: true
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
                                grid.currentIndex = index
                                stack.push(albumDetailComponent, { folderPath: path, albumName: name })
                            }
                        }
                    }
                }
            }
            
            DateScrubber {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                
                listView: grid
                proxyModel: null
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
            property var activeGrid: innerGalleryLoader.item ? (innerGalleryLoader.item.grid || innerGalleryLoader.item.gridView) : null
            
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
                        
                        StyledButton {
                            text: "Back"
                            iconText: "‹"
                            fontSize: 14
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
                        Binding {
                            target: innerGalleryLoader.item ? innerGalleryLoader.item.model : null
                            property: "filterQuery"
                            value: (typeof searchField !== "undefined") ? searchField.text : ""
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
