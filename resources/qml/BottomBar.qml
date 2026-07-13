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
            delegate: Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 4
                    
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: {
                            if (modelData === "Pictures") return "🖼️"
                            if (modelData === "Albums") return "📁"
                            return "☰"
                        }
                        font.pixelSize: 24
                        opacity: root.currentIndex === index ? 1.0 : 0.5
                    }
                    
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData
                        font.pixelSize: 10
                        color: root.currentIndex === index ? "#2196F3" : "#888"
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
