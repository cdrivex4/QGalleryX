import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    
    signal feedbackSubmitted(string issueType, string comment)
    signal closeClicked()
    
    Rectangle {
        anchors.centerIn: parent
        width: 300
        height: 350
        color: "#202020"
        radius: 10
        border.color: "#FF4040"
        border.width: 1
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15
            
            Text {
                text: "Report Video Issue"
                color: "white"
                font.pixelSize: 18
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            
            Text {
                text: "What's wrong with this video?"
                color: "#CCCCCC"
                font.pixelSize: 14
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            
            GridLayout {
                columns: 2
                Layout.fillWidth: true
                columnSpacing: 10
                rowSpacing: 10
                
                Button {
                    text: "Black Screen"
                    Layout.fillWidth: true
                    onClicked: root.feedbackSubmitted("Black Screen", "")
                }
                
                Button {
                    text: "No Sound"
                    Layout.fillWidth: true
                    onClicked: root.feedbackSubmitted("No Sound", "")
                }
                
                Button {
                    text: "Stuttering"
                    Layout.fillWidth: true
                    onClicked: root.feedbackSubmitted("Stuttering", "")
                }
                
                Button {
                    text: "Artifacts"
                    Layout.fillWidth: true
                    onClicked: root.feedbackSubmitted("Visual Artifacts", "")
                }
                
                Button {
                    text: "Audio Sync"
                    Layout.fillWidth: true
                    onClicked: root.feedbackSubmitted("Audio Sync", "")
                }
                
                Button {
                    text: "Won't Play"
                    Layout.fillWidth: true
                    onClicked: root.feedbackSubmitted("Wont Play", "")
                }
            }
            
            Item { Layout.fillHeight: true }
            
            Button {
                text: "Cancel"
                Layout.alignment: Qt.AlignHCenter
                onClicked: root.closeClicked()
            }
        }
    }
}
