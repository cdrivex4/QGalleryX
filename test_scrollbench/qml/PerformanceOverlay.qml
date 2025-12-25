import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: outerFrame
    color: "#1e1e1e"
    border.color: "#333"
    border.width: 1

    signal closeRequested()
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Header Area with Tabs and Close Button
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: "#252525"
            
            RowLayout {
                anchors.fill: parent
                spacing: 0
                
                TabBar {
                    id: tabBar
                    Layout.fillWidth: true
                    background: Rectangle { color: "transparent" }
                    
                    TabButton {
                        text: "Stats"
                        contentItem: Text {
                            text: parent.text
                            font.bold: true
                            color: parent.checked ? "#2196F3" : "#888"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    TabButton {
                        text: "Settings"
                        contentItem: Text {
                            text: parent.text
                            font.bold: true
                            color: parent.checked ? "#2196F3" : "#888"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
                
                Button {
                    text: "✕"
                    flat: true
                    Layout.preferredWidth: 50
                    Layout.fillHeight: true
                    contentItem: Text {
                        text: parent.text
                        color: parent.hovered ? "#ff5252" : "#888"
                        font.pixelSize: 20
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: outerFrame.closeRequested()
                }
            }
        }
        
        // Content Area
        StackLayout {
            currentIndex: tabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            // PAGE 1: STATS
            Flickable {
                contentHeight: statsCol.height
                clip: true
                ScrollBar.vertical: ScrollBar { }
                
                ColumnLayout {
                    id: statsCol
                    width: parent.width - 20
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 15
                    
                    Item { height: 10; width: 1 }

                    // FPS Section
                    ColumnLayout {
                        Text { text: "Rendering"; font.pixelSize: 14; font.bold: true; color: "#aaa" }
                        Text {
                            text: telemetry.fps + " FPS"
                            font.pixelSize: 32; font.bold: true
                            color: telemetry.fps > 55 ? "#4CAF50" : "#FFC107"
                        }
                    }

                    // Resources
                    ColumnLayout {
                        spacing: 8
                        Text { text: "Resources"; font.pixelSize: 14; font.bold: true; color: "#aaa" }
                        
                        // CPU
                        ColumnLayout {
                            Text { text: "CPU Usage (App / System)"; font.pixelSize: 11; color: "#888" }
                            UsageGraph {
                                Layout.fillWidth: true; height: 30
                                color: "#4CAF50"; maxValue: 100
                                dataPoints: telemetry.cpuHistory
                            }
                            Text { 
                                text: "App: " + systemMonitor.cpuUsage.toFixed(1) + "% | Sys: " + systemMonitor.systemCpuUsage.toFixed(1) + "%"
                                font.pixelSize: 10; color: "#fff"
                            }
                        }

                        // RAM
                        ColumnLayout {
                            Text { text: "RAM Usage (App)"; font.pixelSize: 11; color: "#888" }
                            UsageGraph {
                                Layout.fillWidth: true; height: 30
                                color: "#2196F3"; maxValue: 100
                                dataPoints: telemetry.ramHistory
                            }
                            Text { 
                                text: systemMonitor.memoryUsageMB.toFixed(0) + " MB / " + systemMonitor.totalSystemMemoryMB.toFixed(0) + " MB"
                                font.pixelSize: 10; color: "#fff"
                            }
                        }

                        // GPU
                        ColumnLayout {
                            Text { text: "GPU: " + systemMonitor.gpuName; font.pixelSize: 11; color: "#888" }
                            UsageGraph {
                                Layout.fillWidth: true; height: 30
                                color: "#FF00FF"; maxValue: 100
                                dataPoints: telemetry.gpuHistory
                            }
                            Text { 
                                text: "Load: " + systemMonitor.gpuUsage.toFixed(1) + "% | VRAM: " + systemMonitor.gpuVramUsedMB.toFixed(0) + " MB"
                                font.pixelSize: 10; color: "#fff"
                            }
                        }
                    }

                    // Task Queue
                    ColumnLayout {
                        Text { text: "Pipeline"; font.pixelSize: 14; font.bold: true; color: "#aaa" }
                        RowLayout {
                            ColumnLayout { Text { text: "Active"; font.pixelSize: 10; color: "#888" } Text { text: taskScheduler.activeTaskCount; color: "#fff"; font.bold: true } }
                            Item { Layout.fillWidth: true }
                            ColumnLayout { Text { text: "Pending"; font.pixelSize: 10; color: "#888" } Text { text: imageModel.remainingItems; color: "#fff"; font.bold: true } }
                        }
                    }

                    Item { height: 20; width: 1 }
                }
            }
            
            // PAGE 2: SETTINGS
            Flickable {
                contentHeight: settingsCol.height
                clip: true
                ScrollBar.vertical: ScrollBar { }
                
                ColumnLayout {
                    id: settingsCol
                    width: parent.width - 30
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 18
                    
                    Item { height: 10; width: 1 }

                    // Sliders
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Thumb Resolution: " + settings.thumbnailSize + "px"; color: "#fff"; font.pixelSize: 12 }
                        Slider {
                            Layout.fillWidth: true; from: 16; to: 512; stepSize: 4
                            value: settings.thumbnailSize
                            onMoved: settings.thumbnailSize = Math.round(value)
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Grid Zoom: " + settings.gridSize + "px"; color: "#fff"; font.pixelSize: 12 }
                        Slider {
                            Layout.fillWidth: true; from: 20; to: 400; stepSize: 4
                            value: settings.gridSize
                            onMoved: settings.gridSize = Math.round(value)
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Cache Size: " + settings.cacheSizeMB + " MB"; color: "#fff"; font.pixelSize: 12 }
                        Slider {
                            Layout.fillWidth: true; from: 128; to: 4096; stepSize: 128
                            value: settings.cacheSizeMB
                            onMoved: settings.cacheSizeMB = Math.round(value)
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Threads: " + settings.concurrentThreads + " (Restart)"; color: "#fff"; font.pixelSize: 12 }
                        Slider {
                            Layout.fillWidth: true; from: 1; to: 32; stepSize: 1
                            value: settings.concurrentThreads
                            onMoved: settings.concurrentThreads = Math.round(value)
                        }
                    }

                    // Options
                    ColumnLayout {
                        spacing: 10
                        RowLayout {
                            Text { text: "RAW: Fast Preview"; color: "#fff"; Layout.fillWidth: true }
                            Switch { checked: settings.rawAcceleration; onToggled: settings.rawAcceleration = checked }
                        }
                        RowLayout {
                            Text { text: "Use Disk Cache"; color: "#fff"; Layout.fillWidth: true }
                            Switch { checked: settings.useDiskCache; onToggled: settings.useDiskCache = checked }
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Log Level: None", "Log Level: Info", "Log Level: Debug", "Log Level: Trace"]
                            currentIndex: settings.logLevel
                            onActivated: (index) => settings.logLevel = index
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Accel: Auto", "Accel: D3D11VA", "Accel: Vulkan", "Accel: OpenCL"]
                            currentIndex: settings.videoAcceleration
                            onActivated: (index) => settings.videoAcceleration = index
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Clear Disk Cache"
                        onClicked: settings.clearDiskCache()
                    }

                    Item { height: 20; width: 1 }
                }
            }
        }
        
        // SECTION: VIEW TOGGLE (Always visible at bottom of overlay)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: "#252525"
            border.color: "#333"
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 10
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    text: root.currentView === 2 ? "View: Semantic" : "View: Standard Grid"
                    highlighted: true
                    onClicked: {
                        if (root.currentView === 0) root.currentView = 2
                        else root.currentView = 0
                    }
                }
                
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    text: {
                        if (!root.semanticView) return "Grouping"
                        if (root.semanticView.groupingAuto) return "Auto Grouping: ON"
                        if (root.semanticView.groupingMode === 1) return "Grouping: Day"
                        if (root.semanticView.groupingMode === 2) return "Grouping: Week"
                        if (root.semanticView.groupingMode === 3) return "Grouping: Month"
                        if (root.semanticView.groupingMode === 4) return "Grouping: Year"
                        return "Grouping: Manual"
                    }
                    visible: root.currentView === 2
                    onClicked: {
                        if (!root.semanticView) return
                        var sv = root.semanticView
                        if (sv.groupingAuto) {
                            sv.groupingAuto = false
                            sv.groupingMode = 1
                        } else if (sv.groupingMode === 1) {
                            sv.groupingMode = 2
                        } else if (sv.groupingMode === 2) {
                            sv.groupingMode = 3
                        } else if (sv.groupingMode === 3) {
                            sv.groupingMode = 4
                        } else {
                            sv.groupingAuto = true
                        }
                    }
                }
            }
        }
    }
}