import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: childrenRect.width
    height: childrenRect.height
    visible: true // Controlled by binding in Main.qml

    Rectangle {
        id: bg
        anchors.fill: contentCol
        anchors.margins: -5
        color: "#88000000" // Semi-transparent black
        radius: 5
        border.color: "#444"
        border.width: 1
    }

    Column {
        id: contentCol
        spacing: 2
        padding: 5

        Text {
            text: "Build #" + BuildInfo.build
            color: "#00FF00" // Green
            font.bold: true
            font.pixelSize: 12
            style: Text.Outline
            styleColor: "black"
        }

        Text {
            text: "v" + BuildInfo.major + "." + BuildInfo.minor + "." + BuildInfo.build
            color: "#CCCCCC"
            font.pixelSize: 10
            style: Text.Outline
            styleColor: "black"
        }

        Text {
            text: BuildInfo.buildTimestamp
            color: "#AAAAAA"
            font.pixelSize: 10
            style: Text.Outline
            styleColor: "black"
        }
    }
}
