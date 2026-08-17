import QtQuick
import QtQuick.Controls

Item {
    id: toastRoot
    anchors.fill: parent
    z: 999
    enabled: false

    function showMessage(msg, isWarning) {
        toastText.text = msg
        toastRect.color = isWarning ? "#CCB00000" : "#CC222222"
        toastRect.borderColor = isWarning ? "#FF4444" : "#555555"
        hideTimer.restart()
        toastAnim.restart()
    }

    function showToast(msg) {
        showMessage(msg, false)
    }

    Rectangle {
        id: toastRect
        property color borderColor: "#555555"
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 80
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(toastRow.implicitWidth + 32, toastRoot.width - 40)
        height: 42
        radius: 21
        color: "#CC222222"
        border.color: borderColor
        border.width: 1
        opacity: 0

        Row {
            id: toastRow
            anchors.centerIn: parent
            spacing: 8

            Text {
                id: toastIcon
                text: toastRect.color == "#CCB00000" ? "⚠️" : "⏳"
                font.pixelSize: 14
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: toastText
                text: ""
                color: "white"
                font.pixelSize: 13
                font.weight: Font.Medium
                elide: Text.ElideRight
                width: Math.min(implicitWidth, toastRoot.width - 100)
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Timer {
        id: hideTimer
        interval: 3500
        onTriggered: fadeOutAnim.start()
    }

    ParallelAnimation {
        id: toastAnim
        NumberAnimation { target: toastRect; property: "opacity"; to: 1.0; duration: 200; easing.type: Easing.OutCubic }
        NumberAnimation { target: toastRect; property: "scale"; from: 0.9; to: 1.0; duration: 200; easing.type: Easing.OutCubic }
    }

    NumberAnimation {
        id: fadeOutAnim
        target: toastRect
        property: "opacity"
        to: 0.0
        duration: 300
        easing.type: Easing.InCubic
    }

    Connections {
        target: typeof latencyGuard !== "undefined" ? latencyGuard : null

        function onSingleLatencySpike(fileName, latencyMs) {
            if (typeof appSettings !== "undefined" && !appSettings.showLatencyToasts) return
            toastRoot.showMessage("'" + fileName + "' took a little long to find/load (" + latencyMs + "ms)", false)
        }

        function onDriveLatencyWarning(driveRoot, spikeCount) {
            if (typeof appSettings !== "undefined" && !appSettings.showLatencyToasts) return
            toastRoot.showMessage("You might have disk issues bruh! (" + driveRoot + " has " + spikeCount + " slow reads)", true)
        }
    }
}
