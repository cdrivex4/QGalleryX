import QtQuick 2.15
import QtQuick.Layouts 1.15

// Single diagnostic row component
RowLayout {
    id: root
    spacing: 8
    
    required property string label
    required property string value
    required property string statusColor
    
    Rectangle {
        width: 8
        height: 8
        radius: 4
        color: root.statusColor === "green" ? "#4CAF50" :
               root.statusColor === "cyan" ? "#00BCD4" :
               root.statusColor === "orange" ? "#FF9800" :
               root.statusColor === "red" ? "#F44336" : "#9E9E9E"
    }
    
    Text {
        text: root.label + ":"
        color: "#AAA"
        font.pixelSize: 11
        Layout.minimumWidth: 120
    }
    
    Text {
        Layout.fillWidth: true
        text: root.value
        color: "white"
        font.pixelSize: 11
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
    }
}
