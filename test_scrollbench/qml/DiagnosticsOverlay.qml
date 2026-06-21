import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Real-time Diagnostics Overlay - Always-on system health monitor
Rectangle {
    id: root
    color: "#CC000000"
    radius: 8
    border.color: diagnostics.healthColor === "green" ? "#4CAF50" :
                  diagnostics.healthColor === "orange" ? "#FF9800" :
                  diagnostics.healthColor === "red" ? "#F44336" : "#9E9E9E"
    border.width: 2
    
    property bool expanded: false
    onVisibleChanged: if (!visible) criticalPopup.close()
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8
        
        // Header with overall health
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            
            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: diagnostics.healthColor === "green" ? "#4CAF50" :
                       diagnostics.healthColor === "orange" ? "#FF9800" :
                       diagnostics.healthColor === "red" ? "#F44336" : "#9E9E9E"
                
                SequentialAnimation on opacity {
                    running: diagnostics.healthColor === "red"
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.3; duration: 500 }
                    NumberAnimation { to: 1.0; duration: 500 }
                }
            }
            
            Text {
                Layout.fillWidth: true
                text: "FPS: " + telemetry.fps + " | Mem: " + systemMonitor.memoryUsageMB.toFixed(0) + "MB\n" + diagnostics.healthStatus
                color: "white"
                font.bold: true
                font.pixelSize: 13
            }
            
            Text {
                text: root.expanded ? "▼" : "▶"
                color: "#AAA"
                font.pixelSize: 12
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.expanded = !root.expanded
                }
            }
        }
        
        // Divider
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#333"
        }
        
        // Expanded details
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: root.expanded
            
            // Viewport Culling Status
            DiagnosticRow {
                Layout.fillWidth: true
                label: "Viewport Culling"
                value: diagnostics.viewportStatus
                statusColor: diagnostics.viewportStatusColor
            }
            
            // Load Progress
            DiagnosticRow {
                Layout.fillWidth: true
                label: "Load Progress"
                value: diagnostics.loadProgressStatus
                statusColor: diagnostics.loadedItems === diagnostics.totalItems ? "green" : "cyan"
            }
            
            // Settings Sync
            DiagnosticRow {
                Layout.fillWidth: true
                label: "Settings Sync"
                value: diagnostics.settingsStatus
                statusColor: diagnostics.settingsStatusColor
            }
            
            // I/O Status
            DiagnosticRow {
                Layout.fillWidth: true
                label: "Adaptive I/O"
                value: diagnostics.ioStatus
                statusColor: diagnostics.activeIOTasks > 500 ? "orange" : "green"
            }
            
            // Warnings section
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                visible: diagnostics.activeWarnings.length > 0
                
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#FF9800"
                }
                
                Text {
                    text: "⚠️ Warnings:"
                    color: "#FF9800"
                    font.bold: true
                    font.pixelSize: 11
                }
                
                Repeater {
                    model: diagnostics.activeWarnings
                    delegate: Text {
                        text: "  • " + modelData
                        color: "#FFB74D"
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
            
            // Criticals section
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                visible: diagnostics.activeCriticals.length > 0
                
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#F44336"
                }
                
                Text {
                    text: "❌ CRITICAL Issues:"
                    color: "#F44336"
                    font.bold: true
                    font.pixelSize: 11
                }
                
                Repeater {
                    model: diagnostics.activeCriticals
                    delegate: Text {
                        text: "  • " + modelData
                        color: "#EF5350"
                        font.pixelSize: 10
                        font.bold: true
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
            
            // Quick Actions
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#333"
            }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                
                Button {
                    Layout.fillWidth: true
                    text: "Toggle Culling"
                    font.pixelSize: 10
                    onClicked: imageModel.viewportCullingEnabled = !imageModel.viewportCullingEnabled
                    background: Rectangle {
                        color: parent.pressed ? "#1976D2" : "#2196F3"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font: parent.font
                    }
                }
                
                Button {
                    Layout.fillWidth: true
                    text: "Toggle Cache"
                    font.pixelSize: 10
                    onClicked: settings.useDiskCache = !settings.useDiskCache
                    background: Rectangle {
                        color: parent.pressed ? "#388E3C" : "#4CAF50"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font: parent.font
                    }
                }
            }
        }
    }
    
    property alias popupWidth: criticalPopup.width
    property alias popupHeight: criticalPopup.height
    property real popupPadding: 20
    property alias popupX: criticalPopup.x
    property alias popupY: criticalPopup.y

    Connections {
        target: diagnostics
        function onCriticalIssueDetected(issue) {
            console.error("[DiagnosticsUI] CRITICAL ISSUE:", issue)
            // User requested removal of popup. Errors are shown in list.
            // criticalPopup.issue = issue
            // criticalPopup.open()
        }
    }
    
    Popup {
        id: criticalPopup
        anchors.centerIn: Overlay.overlay
        width: 400
        height: 250
        modal: false // Prevent blocking interaction
        dim: false
        focus: false // Don't steal focus
        closePolicy: Popup.NoAutoClose // Keep it open until acknowledged
        
        property string issue: ""
        
        background: Rectangle {
            color: "#1E1E1E"
            border.color: "#F44336"
            border.width: 2
            radius: 8
        }
        
        ColumnLayout {
            id: popupContent
            anchors.fill: parent
            anchors.margins: root.popupPadding
            spacing: 15
            
            Text {
                text: "❌ CRITICAL ISSUE DETECTED"
                color: "#F44336"
                font.bold: true
                font.pixelSize: 16
            }
            
            Text {
                Layout.fillWidth: true
                text: criticalPopup.issue
                color: "white"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            
            Item { Layout.fillHeight: true }
            
            Button {
                Layout.alignment: Qt.AlignRight
                text: "Acknowledge"
                onClicked: criticalPopup.close()
                background: Rectangle {
                    color: parent.pressed ? "#D32F2F" : "#F44336"
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    padding: 8
                    font.bold: true
                }
            }
        }
    }
}
