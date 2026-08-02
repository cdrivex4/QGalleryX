import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    height: 65
    color: "#0D47A1" // Darker Blue for action context
    
    // External Signals
    signal shareClicked()
    signal clearClicked()
    signal rotateClicked()
    signal resizeClicked()
    
    // Model binding
    property var model: null
    property int selectedCount: (model !== null && model.selectedCount !== undefined) ? model.selectedCount : 0
    
    // Animation properties
    Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }
    Behavior on opacity { NumberAnimation { duration: 200 } }
    
    // Only visible when items are selected
    visible: selectedCount > 0
    opacity: visible ? 1 : 0
    
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        spacing: 15
        
        // Close / Clear Selection
        Button {
            text: "✕"
            flat: true
            Layout.preferredWidth: 40
            onClicked: root.clearClicked()
            contentItem: Text {
                text: parent.text
                font.pixelSize: 20
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Item {}
        }
        
        Text {
            text: root.selectedCount + " Selected"
            color: "white"
            font.bold: true
            font.pixelSize: 18
            Layout.alignment: Qt.AlignVCenter
        }
        
        Item { Layout.fillWidth: true } // Spacer
        
        // Selection Tools
        Button {
            text: "Select All"
            flat: true
            onClicked: root.model.selectAll()
            contentItem: Text { text: parent.text; color: "white"; font.bold: true }
            background: Rectangle { color: parent.pressed ? "#22FFFFFF" : "transparent"; radius: 4 }
        }

        Button {
            text: "Invert"
            flat: true
            onClicked: {
                if (root.model) {
                     root.model.invertSelection()
                }
            }
            contentItem: Text { text: parent.text; color: "white"; font.bold: true }
            background: Rectangle { color: parent.pressed ? "#22FFFFFF" : "transparent"; radius: 4 }
        }
        
        Rectangle { width: 1; height: 30; color: "#FFFFFF"; opacity: 0.3 } // Separator
        
        Button {
            id: rotateBtn
            text: "Rotate"
            flat: true
            onClicked: root.rotateClicked()
            contentItem: Text { text: "Rotate"; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { color: parent.pressed ? "#22FFFFFF" : "transparent"; radius: 4 }
        }

        Button {
            id: resizeBtn
            text: "Resize/Crop"
            flat: true
            onClicked: root.resizeClicked()
            contentItem: Text { text: "Resize"; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { color: parent.pressed ? "#22FFFFFF" : "transparent"; radius: 4 }
        }
        
        Button {
            text: "Share"
            Layout.preferredHeight: 36
            Layout.preferredWidth: 100
            onClicked: root.shareClicked()
            
            background: Rectangle {
                color: parent.pressed ? "#DDDDDD" : "white"
                radius: 18
            }
            contentItem: Text {
                text: parent.text
                color: "#0D47A1"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
