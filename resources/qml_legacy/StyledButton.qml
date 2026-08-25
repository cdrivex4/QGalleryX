import QtQuick
import QtQuick.Controls

Button {
    id: control

    property bool flatStyle: false
    property color backgroundColor: flatStyle ? "transparent" : "#333333"
    property color hoverColor: flatStyle ? "#30ffffff" : "#424244"
    property color pressedColor: flatStyle ? "#50ffffff" : "#252527"
    property color textColor: "#ffffff"
    property color accentColor: "#1976D2"
    property color accentHoverColor: "#2196F3"
    property color accentPressedColor: "#1565C0"
    property bool isAccent: false
    property real cornerRadius: 8
    property int fontSize: 14
    property bool fontBold: false
    property string iconText: ""
    property real iconSize: fontSize + 2

    implicitWidth: Math.max(36, contentRow.implicitWidth + (text !== "" ? 24 : 12))
    implicitHeight: 38

    background: Rectangle {
        color: {
            if (!control.enabled) return flatStyle ? "transparent" : "#222222"
            if (control.pressed) return control.isAccent ? control.accentPressedColor : control.pressedColor
            if (control.hovered) return control.isAccent ? control.accentHoverColor : control.hoverColor
            return control.isAccent ? control.accentColor : control.backgroundColor
        }
        radius: control.cornerRadius
        border.color: control.activeFocus ? "#38BDF8" : (control.hovered ? "#60A5FA" : (flatStyle ? "transparent" : "#20ffffff"))
        border.width: control.activeFocus ? 2 : (flatStyle ? 0 : 1)

        // Windows Explorer / Samsung Gallery Blue Focus Halo
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            color: "transparent"
            border.color: "#38BDF8"
            border.width: 1.5
            radius: control.cornerRadius + 3
            opacity: 0.85
            visible: control.activeFocus
        }

        Behavior on color { ColorAnimation { duration: 120 } }
    }

    contentItem: Row {
        id: contentRow
        spacing: 6
        anchors.centerIn: parent

        Text {
            visible: control.iconText !== ""
            text: control.iconText
            color: control.enabled ? control.textColor : "#777777"
            font.pixelSize: control.iconSize
            font.bold: control.fontBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            visible: control.text !== ""
            text: control.text
            color: control.enabled ? control.textColor : "#777777"
            font.pixelSize: control.fontSize
            font.bold: control.fontBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
