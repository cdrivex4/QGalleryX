import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 300
    height: 400
    color: "#D0000000"
    border.color: "#40FFFFFF"
    radius: 8
    
    // Target object to control
    property var target: null
    property var popupTarget: null // Specifically for the critical popup if separate

    // Expose properties to bind to
    property alias topMargin: topMarginSlider.value
    property alias leftMargin: leftMarginSlider.value
    property alias popupPadding: popupPaddingSlider.value
    property alias popupWidthScale: popupWidthSlider.value

    property bool diagnosticsHidden: false

    DragHandler {}

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10

        Text {
            text: "UI Layout Tuner"
            color: "white"
            font.bold: true
            font.pixelSize: 16
            Layout.alignment: Qt.AlignHCenter
        }
        
        CheckBox {
            text: "Hide Diagnostics Overlay"
            checked: root.diagnosticsHidden
            onCheckedChanged: root.diagnosticsHidden = checked
            contentItem: Text {
                text: parent.text
                color: "white"
                leftPadding: parent.indicator.width + parent.spacing
                verticalAlignment: Text.AlignVCenter
            }
        }

        Rectangle { height: 1; Layout.fillWidth: true; color: "#40FFFFFF" }

        // Top Margin
        Label { text: "Top Margin: " + topMarginSlider.value.toFixed(0); color: "white" }
        Slider {
            id: topMarginSlider
            Layout.fillWidth: true
            from: 0; to: 200
            value: 134
        }

        // Left Margin
        Label { text: "Left Margin: " + leftMarginSlider.value.toFixed(0); color: "white" }
        Slider {
            id: leftMarginSlider
            Layout.fillWidth: true
            from: 0; to: 200
            value: 22
        }

        Rectangle { height: 1; Layout.fillWidth: true; color: "#40FFFFFF" }
        Text { text: "Critical Popup"; color: "#AAAAAA"; font.pixelSize: 12 }

        // Popup Width Scale
        Label { text: "Width Scale: " + popupWidthSlider.value.toFixed(1) + "x"; color: "white" }
        Slider {
            id: popupWidthSlider
            Layout.fillWidth: true
            from: 0.5; to: 2.0
            value: 1.0
        }

        // Popup Padding
        Label { text: "Padding: " + popupPaddingSlider.value.toFixed(0); color: "white" }
        Slider {
            id: popupPaddingSlider
            Layout.fillWidth: true
            from: 0; to: 50
            value: 22
        }

        Item { Layout.fillHeight: true } // Spacer

        Button {
            text: "Reset Defaults"
            Layout.alignment: Qt.AlignHCenter
            onClicked: {
                topMarginSlider.value = 134
                leftMarginSlider.value = 22
                popupWidthSlider.value = 1.0
                popupPaddingSlider.value = 22
            }
        }
    }
}
