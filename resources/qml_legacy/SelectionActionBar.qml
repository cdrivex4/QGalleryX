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
    property int selectedCount: (model && model.selectedCount !== undefined) ? model.selectedCount : 0
    
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
            id: cancelBtn
            text: "✕"
            flat: true
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            onClicked: root.clearClicked()
            contentItem: Text {
                text: parent.text
                font.pixelSize: 18
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: parent.pressed ? "#44FFFFFF" : (parent.hovered || parent.activeFocus ? "#30FFFFFF" : "transparent")
                radius: 18
                border.color: parent.activeFocus ? "#00E5FF" : (parent.hovered ? "#60A5FA" : "transparent")
                border.width: parent.activeFocus ? 2 : (parent.hovered ? 1 : 0)

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -2
                    color: "transparent"
                    border.color: "#38BDF8"
                    border.width: 1.5
                    radius: 20
                    opacity: 0.85
                    visible: cancelBtn.activeFocus
                }
            }
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
            id: selectAllBtn
            text: "Select All"
            flat: true
            onClicked: root.model.selectAll()
            contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle {
                color: parent.pressed ? "#44FFFFFF" : (parent.hovered || parent.activeFocus ? "#30FFFFFF" : "#15FFFFFF")
                radius: 6
                border.color: parent.activeFocus ? "#00E5FF" : (parent.hovered ? "#60A5FA" : "transparent")
                border.width: parent.activeFocus ? 2 : (parent.hovered ? 1 : 0)

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -2
                    color: "transparent"
                    border.color: "#38BDF8"
                    border.width: 1.5
                    radius: 8
                    opacity: 0.85
                    visible: selectAllBtn.activeFocus
                }
            }
        }

        Button {
            id: invertBtn
            text: "Invert"
            flat: true
            onClicked: {
                if (root.model) {
                     root.model.invertSelection()
                }
            }
            contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle {
                color: parent.pressed ? "#44FFFFFF" : (parent.hovered || parent.activeFocus ? "#30FFFFFF" : "#15FFFFFF")
                radius: 6
                border.color: parent.activeFocus ? "#00E5FF" : (parent.hovered ? "#60A5FA" : "transparent")
                border.width: parent.activeFocus ? 2 : (parent.hovered ? 1 : 0)

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -2
                    color: "transparent"
                    border.color: "#38BDF8"
                    border.width: 1.5
                    radius: 8
                    opacity: 0.85
                    visible: invertBtn.activeFocus
                }
            }
        }
        
        Rectangle { width: 1; height: 30; color: "#FFFFFF"; opacity: 0.3 } // Separator
        
        Button {
            id: rotateBtn
            text: "Rotate"
            flat: true
            onClicked: root.rotateClicked()
            contentItem: Text { text: "Rotate"; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle {
                color: parent.pressed ? "#44FFFFFF" : (parent.hovered || parent.activeFocus ? "#30FFFFFF" : "#15FFFFFF")
                radius: 6
                border.color: parent.activeFocus ? "#00E5FF" : (parent.hovered ? "#60A5FA" : "transparent")
                border.width: parent.activeFocus ? 2 : (parent.hovered ? 1 : 0)

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -2
                    color: "transparent"
                    border.color: "#38BDF8"
                    border.width: 1.5
                    radius: 8
                    opacity: 0.85
                    visible: rotateBtn.activeFocus
                }
            }
        }

        Button {
            id: resizeBtn
            text: "Resize"
            flat: true
            onClicked: root.resizeClicked()
            contentItem: Text { text: "Resize"; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle {
                color: parent.pressed ? "#44FFFFFF" : (parent.hovered || parent.activeFocus ? "#30FFFFFF" : "#15FFFFFF")
                radius: 6
                border.color: parent.activeFocus ? "#00E5FF" : (parent.hovered ? "#60A5FA" : "transparent")
                border.width: parent.activeFocus ? 2 : (parent.hovered ? 1 : 0)

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -2
                    color: "transparent"
                    border.color: "#38BDF8"
                    border.width: 1.5
                    radius: 8
                    opacity: 0.85
                    visible: resizeBtn.activeFocus
                }
            }
        }
        
        Button {
            id: shareBtn
            text: "Share"
            Layout.preferredHeight: 36
            Layout.preferredWidth: 100
            onClicked: root.shareClicked()
            
            background: Rectangle {
                color: parent.pressed ? "#DDDDDD" : "white"
                radius: 18
                border.color: parent.activeFocus ? "#00E5FF" : "transparent"
                border.width: parent.activeFocus ? 2 : 0

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -3
                    color: "transparent"
                    border.color: "#38BDF8"
                    border.width: 2
                    radius: 21
                    opacity: 0.9
                    visible: shareBtn.activeFocus
                }
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
