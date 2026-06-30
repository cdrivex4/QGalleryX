import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

ApplicationWindow {
    id: window
    visible: true
    width: 800
    height: 600
    title: "Hello World - LMStudio Test"
    color: "#2c3e50"
    
    // Center the window on screen
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2
    
    // Main content
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20
        
        // Hello World text
        Text {
            text: "Hello World!"
            color: "white"
            font {
                family: "Arial"
                pixelSize: 48
                bold: true
            }
            Layout.alignment: Qt.AlignHCenter
            
            // Add some animation
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { from: 0.7; to: 1.0; duration: 1000; easing.type: Easing.InOutQuad }
                NumberAnimation { from: 1.0; to: 0.7; duration: 1000; easing.type: Easing.InOutQuad }
            }
        }
        
        // Subtitle
        Text {
            text: "LMStudio Test Application"
            color: "#ecf0f1"
            font {
                family: "Arial"
                pixelSize: 24
            }
            Layout.alignment: Qt.AlignHCenter
        }
        
        // Info panel
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 400
            Layout.preferredHeight: 100
            color: "#34495e"
            radius: 10
            border.color: "#7f8c8d"
            border.width: 2
            
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 5
                
                Text {
                    text: "Qt Framework Test"
                    color: "#3498db"
                    font {
                        family: "Arial"
                        pixelSize: 16
                        bold: true
                    }
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Text {
                    text: "Built with Qt 6.4"
                    color: "#95a5a6"
                    font {
                        family: "Arial"
                        pixelSize: 14
                    }
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
        
        // Test button
        Button {
            text: "Test Button"
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 150
            
            onClicked: {
                console.log("Button clicked!")
                testLabel.text = "Button clicked at " + new Date().toLocaleTimeString()
            }
            
            background: Rectangle {
                color: "#3498db"
                radius: 5
            }
            
            contentItem: Text {
                text: parent.text
                color: "white"
                font {
                    family: "Arial"
                    pixelSize: 16
                    bold: true
                }
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        
        // Test label
        Text {
            id: testLabel
            text: "Click the button above"
            color: "#e74c3c"
            font {
                family: "Arial"
                pixelSize: 14
            }
            Layout.alignment: Qt.AlignHCenter
        }
    }
    
    // Status bar
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 30
        color: "#2c3e50"
        border.color: "#7f8c8d"
        border.width: 1
        
        Text {
            anchors.centerIn: parent
            text: "LMStudio Test Application v1.0.0"
            color: "#95a5a6"
            font {
                family: "Arial"
                pixelSize: 12
            }
        }
    }
    
    // Console output for debugging
    Component.onCompleted: {
        console.log("Hello World QML loaded successfully")
        console.log("Window size:", width, "x", height)
        console.log("Qt version:", Qt.version)
    }
}