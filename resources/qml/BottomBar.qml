import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    height: 60
    color: "#000000"
    
    signal tabSelected(int index)
    property int currentIndex: 0

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Repeater {
            model: ["Pictures", "Albums", "Stories", "Menu"]
            
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 4
                        
                        // Icon placeholder (using text for now, would replace with proper icons)
                        Text {
                            text: modelData === "Pictures" ? "🖼️" : 
                                  modelData === "Albums" ? "📁" : 
                                  modelData === "Stories" ? "📖" : "☰"
                            font.pixelSize: 20
                            Layout.alignment: Qt.AlignHCenter
                            color: root.currentIndex === index ? "white" : "#888888"
                        }
                        
                        Text {
                            text: modelData
                            color: root.currentIndex === index ? "white" : "#888888"
                            font.pixelSize: 12
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.tabSelected(index)
                    }
                }
            }
        }
    }
}
