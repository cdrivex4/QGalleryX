import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    
    property string name: ""
    property string path: ""
    property var coverPaths: []
    property int count: 0
    
    signal clicked()
    
    width: 200
    height: 250
    
    Rectangle {
        anchors.fill: parent
        anchors.margins: 5
        color: "transparent"
        
        Rectangle {
            id: coverContainer
            width: parent.width
            height: width
            color: "#222"
            radius: 12
            clip: true
            
            // Background shadow/border
            border.color: "#333"
            border.width: 1

            // Single Image
            Image {
                anchors.fill: parent
                visible: coverPaths && coverPaths.length === 1
                source: visible ? "image://async/" + coverPaths[0] : ""
                asynchronous: true
                fillMode: Image.PreserveAspectCrop
            }

            // Grid (2+ images)
            Grid {
                anchors.fill: parent
                columns: 2
                visible: coverPaths && coverPaths.length >= 2
                spacing: 2
                
                Repeater {
                    model: (coverPaths && coverPaths.length >= 2) ? (coverPaths.length > 4 ? coverPaths.slice(0, 4) : coverPaths) : 0
                    Image {
                        width: (coverContainer.width - 2) / 2
                        height: (coverContainer.height - 2) / 2
                        source: "image://async/" + modelData
                        asynchronous: true
                        fillMode: Image.PreserveAspectCrop
                    }
                }
            }
            
            // Hover Overlay
            Rectangle {
                anchors.fill: parent
                color: "white"
                opacity: mouseArea.containsMouse ? 0.1 : 0
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }
        }
        
        Column {
            anchors.top: coverContainer.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 8
            spacing: 2
            
            Text {
                text: root.name
                color: "white"
                font.pixelSize: 15
                font.bold: true
                elide: Text.ElideRight
                width: parent.width
            }
            
            Text {
                text: root.count
                color: "#888"
                font.pixelSize: 13
            }
        }
        
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.clicked()
            onPressed: coverContainer.scale = 0.98
            onReleased: coverContainer.scale = 1.0
            onCanceled: coverContainer.scale = 1.0
            
            Behavior on scale { NumberAnimation { duration: 100 } }
        }
    }
}
