import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Outer frame - Transparent Overlay Frame
Rectangle {
    id: outerFrame
    color: "#252525"
    border.color: "#404040"
    border.width: 1
    
    // Inner frame - contains scroll
    Rectangle {
        anchors.fill: parent
        anchors.margins: 2  // Final tested value
        color: "transparent"
        clip: true  // Clip scrollbar to this inner frame

        // Flickable and ScrollBar as SIBLINGS (not attached property)
        Flickable {
            id: flickable
            anchors.fill: parent
            anchors.rightMargin: 30  // Final tested value for scrollbar space
            anchors.margins: 5  // Final tested value
            contentHeight: contentColumn.height
            clip: true

            ColumnLayout {
                id: contentColumn
                width: parent.width
                spacing: 15

                // Title
                Text {
                    text: "Performance Monitor"
                    font.pixelSize: 18
                    font.bold: true
                    color: "#ffffff"
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#404040"
                }

                // FPS Display
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    Text {
                        text: "Frame Rate"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#cccccc"
                    }

                    Text {
                        text: telemetry.fps + " FPS"
                        font.pixelSize: 32
                        font.bold: true
                        color: {
                            if (telemetry.fps >= 50) return "#4CAF50"
                            if (telemetry.fps >= 30) return "#FFC107"
                            return "#F44336"
                        }
                    }

                    Text {
                        text: "Avg: " + telemetry.averageFps + " FPS"
                        font.pixelSize: 12
                        color: "#999999"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#404040"
                }

                // Queue Depth
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    Text {
                        text: "Queue Depth"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#cccccc"
                    }

                    Text {
                        text: imageModel.pendingDecodeCount.toString()
                        font.pixelSize: 24
                        font.bold: true
                        color: imageModel.pendingDecodeCount > 50 ? "#F44336" : "#4CAF50"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#404040"
                }

                // Memory Usage
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    Text {
                        text: "Memory Usage"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#cccccc"
                    }

                    Text {
                        text: telemetry.memoryUsageMB + " MB"
                        font.pixelSize: 24
                        font.bold: true
                        color: "#2196F3"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#404040"
                }

                // Optimizations Section
                Text {
                    text: "Optimizations"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#ffffff"
                    Layout.topMargin: 10
                }

                // Viewport Culling Toggle
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: "Viewport Culling"
                        font.pixelSize: 13
                        color: "#cccccc"
                        Layout.fillWidth: true
                    }

                    Switch {
                        checked: imageModel.viewportCullingEnabled
                        onToggled: imageModel.viewportCullingEnabled = checked
                    }
                }

                // Frame Budget Toggle
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: "Frame Budget"
                        font.pixelSize: 13
                        color: "#cccccc"
                        Layout.fillWidth: true
                    }

                    Switch {
                        checked: frameBudget.enabled
                        onToggled: frameBudget.enabled = checked
                    }
                }

                // Frame Budget Slider
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    visible: frameBudget.enabled

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: "Budget: " + frameBudget.frameBudget + " uploads/frame"
                            font.pixelSize: 12
                            color: "#999999"
                            Layout.fillWidth: true
                        }
                    }

                    Slider {
                        Layout.fillWidth: true
                        from: 1
                        to: 50
                        stepSize: 1
                        value: frameBudget.frameBudget
                        onMoved: frameBudget.frameBudget = value
                    }

                    Text {
                        text: "1 (slow) ← → 50 (fast)"
                        font.pixelSize: 10
                        color: "#666666"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#404040"
                }

                // Grid Size (UI Zoom)
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    Text {
                        text: "Grid Size (UI Zoom)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#cccccc"
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            id: gridSizeText
                            text: gridSizeSlider.value + "px (Ctrl+Wheel)"
                            font.pixelSize: 12
                            color: "#999999"
                            Layout.fillWidth: true
                        }
                    }

                    Slider {
                        id: gridSizeSlider
                        Layout.fillWidth: true
                        from: 20
                        to: 400
                        stepSize: 5
                        value: root.gridSize
                        onMoved: {
                            root.gridSize = value
                        }
                    }

                    Text {
                        text: "20px (tiny) ← → 400px (huge)"
                        font.pixelSize: 10
                        color: "#666666"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#404040"
                }

                // Thumb Resolution (Quality)
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    Text {
                        text: "Thumb Resolution (Quality)"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#cccccc"
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            id: thumbResText
                            text: thumbResSlider.value + "px (default ⭐)"
                            font.pixelSize: 12
                            color: "#999999"
                            Layout.fillWidth: true
                        }
                    }

                    Slider {
                        id: thumbResSlider
                        Layout.fillWidth: true
                        from: 20
                        to: 512
                        stepSize: 64
                        value: root.thumbResolution
                        onMoved: {
                            root.thumbResolution = value
                        }
                    }

                    Text {
                        text: "20px | 128px | 192px | 256px ⭐ | 320px | 384px | 512px"
                        font.pixelSize: 10
                        color: "#666666"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#404040"
                }

                // Current Frame
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    Text {
                        text: "Current Frame"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#cccccc"
                    }

                    Text {
                        text: "Completions: " + telemetry.completionsThisFrame + " / " + frameBudget.frameBudget
                        font.pixelSize: 12
                        color: "#999999"
                    }
                }

                // Help Text
                Text {
                    text: "📊 Scroll to see performance metrics\n" +
                          "🎯 Toggle optimizations to compare\n" +
                          "⚡ Target: 30+ FPS sustained"
                    font.pixelSize: 11
                    color: "#666666"
                    Layout.topMargin: 20
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
            }
        }

        // Standalone ScrollBar as SIBLING to Flickable
        ScrollBar {
            id: scrollBar
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.rightMargin: 2
            anchors.topMargin: 5
            anchors.bottomMargin: 5
            policy: ScrollBar.AsNeeded
            orientation: Qt.Vertical
            size: flickable.height / flickable.contentHeight
            position: flickable.contentY / flickable.contentHeight
            onPositionChanged: {
                if (pressed) {
                    flickable.contentY = position * flickable.contentHeight
                }
            }
        }
    }
}
