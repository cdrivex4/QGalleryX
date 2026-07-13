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
            model: ["Pictures", "Albums", "Menu"]
            
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                Rectangle {
                    anchors.fill: parent
                    color: ma.pressed ? "#333333" : (ma.containsMouse ? "#222222" : "transparent")
                    
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 5
                        
                        // Icon placeholder
                        Text {
                            text: modelData === "Pictures" ? "🖼️" : 
                                  modelData === "Albums" ? "📁" : "⚙️"
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
                        id: ma
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.tabSelected(index)
                    }
                }
            }
        }
    }
}
