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
        anchors.margins: 2
        color: "transparent"
        clip: true

        Flickable {
            id: flickable
            anchors.fill: parent
            anchors.rightMargin: 30
            anchors.margins: 5
            contentHeight: contentColumn.height
            clip: true

            ColumnLayout {
                id: contentColumn
                width: parent.width
                spacing: 15

                // SECTION: RENDERING
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    Text { text: "Rendering Performance"; font.pixelSize: 16; font.bold: true; color: "#ffffff" }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: telemetry.fps + " FPS"
                            font.pixelSize: 32
                            font.bold: true
                            color: telemetry.fps >= 50 ? "#4CAF50" : (telemetry.fps >= 30 ? "#FFC107" : "#F44336")
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: "Avg: " + telemetry.averageFps
                            font.pixelSize: 14
                            color: "#999999"
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#404040" }

                // SECTION: SYSTEM RESOURCES
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { text: "System Resources"; font.pixelSize: 16; font.bold: true; color: "#ffffff" }
                    
                    // CPU Usage (App vs System)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        
                        Text { text: "CPU Usage"; font.pixelSize: 12; color: "#999999" }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 5
                            
                            // Application CPU
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                height: 22
                                color: "#1e1e1e"
                                border.color: "#404040"
                                border.width: 1
                                
                                Rectangle {
                                    width: parent.width * (systemMonitor.cpuUsage / 100.0)
                                    height: parent.height
                                    color: systemMonitor.cpuUsage > 80 ? "#F44336" : 
                                           (systemMonitor.cpuUsage > 50 ? "#FFC107" : "#4CAF50")
                                }
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "App: " + systemMonitor.cpuUsage.toFixed(1) + "%"
                                    font.pixelSize: 10
                                    color: "#ffffff"
                                    font.bold: true
                                }
                            }
                            
                            // System CPU
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                height: 22
                                color: "#1e1e1e"
                                border.color: "#404040"
                                border.width: 1
                                
                                Rectangle {
                                    width: parent.width * (systemMonitor.systemCpuUsage / 100.0)
                                    height: parent.height
                                    color: "#2196F3"
                                }
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "System: " + systemMonitor.systemCpuUsage.toFixed(1) + "%"
                                    font.pixelSize: 10
                                    color: "#ffffff"
                                    font.bold: true
                                }
                            }
                        }
                    }
                    
                    // GPU Usage
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        
                        Text { text: "GPU: " + systemMonitor.gpuName; font.pixelSize: 12; color: "#999999" }
                        Text { text: "Accel: " + hwAccel.currentModeName(); font.pixelSize: 11; color: "#999" }
                        
                        Rectangle {
                            Layout.fillWidth: true
                            height: 22
                            color: "#1e1e1e"
                            border.color: "#404040"
                            border.width: 1
                            
                            Rectangle {
                                width: parent.width * (systemMonitor.gpuUsage / 100.0)
                                height: parent.height
                                color: systemMonitor.gpuUsage > 80 ? "#F44336" : 
                                       (systemMonitor.gpuUsage > 50 ? "#FFC107" : "#9C27B0")
                            }
                            
                            Text {
                                anchors.centerIn: parent
                                text: systemMonitor.gpuUsage.toFixed(1) + "% | VRAM: " + 
                                      systemMonitor.gpuVramUsedMB.toFixed(0) + " / " +
                                      systemMonitor.gpuVramTotalMB.toFixed(0) + " MB"
                                font.pixelSize: 10
                                color: "#ffffff"
                                font.bold: true
                            }
                        }
                    }

                    // CPU Instruction Sets
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Text { text: "CPU Features"; font.pixelSize: 12; color: "#999999" }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 40
                            color: "#1e1e1e"
                            border.color: "#404040"
                            border.width: 1
                            Text {
                                anchors.fill: parent
                                anchors.margins: 5
                                text: hwAccel.cpuInfo()
                                font.pixelSize: 10
                                color: "#4CAF50"
                                wrapMode: Text.Wrap
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    
                    // RAM Usage (App vs System)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        
                        Text { text: "Memory Usage"; font.pixelSize: 12; color: "#999999" }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 5
                            
                            // Application RAM
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                height: 22
                                color: "#1e1e1e"
                                border.color: "#404040"
                                border.width: 1
                                
                                Rectangle {
                                    width: parent.width * (systemMonitor.memoryUsageMB / Math.max(1, systemMonitor.totalSystemMemoryMB))
                                    height: parent.height
                                    color: "#00BCD4"
                                }
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "App: " + systemMonitor.memoryUsageMB.toFixed(0) + " MB"
                                    font.pixelSize: 10
                                    color: "#ffffff"
                                    font.bold: true
                                }
                            }
                            
                            // System RAM
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                height: 22
                                color: "#1e1e1e"
                                border.color: "#404040"
                                border.width: 1
                                
                                Rectangle {
                                    width: parent.width * ((systemMonitor.totalSystemMemoryMB - systemMonitor.availableSystemMemoryMB) / Math.max(1, systemMonitor.totalSystemMemoryMB))
                                    height: parent.height
                                    color: "#FF5722"
                                }
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "System: " + (systemMonitor.totalSystemMemoryMB - systemMonitor.availableSystemMemoryMB).toFixed(0) + 
                                          " / " + systemMonitor.totalSystemMemoryMB.toFixed(0) + " MB"
                                    font.pixelSize: 10
                                    color: "#ffffff"
                                    font.bold: true
                                }
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#404040" }

                // SECTION: PROCESSING & QUEUE
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { text: "Task Queue"; font.pixelSize: 16; font.bold: true; color: "#ffffff" }

                    RowLayout {
                        Layout.fillWidth: true
                        ColumnLayout {
                            Text { text: "Active Processing"; font.pixelSize: 12; color: "#999" }
                            Text {
                                text: taskScheduler ? taskScheduler.activeTaskCount : "0"
                                font.pixelSize: 24
                                font.bold: true
                                color: (taskScheduler && taskScheduler.activeTaskCount > 20) ? "#FFC107" : "#4CAF50"
                            }
                        }
                        Item { Layout.fillWidth: true }
                        ColumnLayout {
                            Text { text: "Remaining Work"; font.pixelSize: 12; color: "#999" }
                            Text {
                                text: imageModel.remainingItems
                                font.pixelSize: 24
                                font.bold: true
                                color: "#2196F3"
                            }
                        }
                    }

                    // Frame Budget
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Frame Budget Scheduler"; font.pixelSize: 13; color: "#ccc"; Layout.fillWidth: true }
                        Switch {
                            checked: frameBudget.enabled
                            onToggled: frameBudget.enabled = checked
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: frameBudget.enabled
                        Text { text: "Budget: " + frameBudget.frameBudget + " tasks/frame"; font.pixelSize: 11; color: "#999" }
                        Slider {
                            Layout.fillWidth: true; from: 1; to: 50; stepSize: 1
                            value: frameBudget.frameBudget
                            onMoved: frameBudget.frameBudget = Math.round(value)
                        }
                    }
                    
                    Text {
                        text: "Completions this frame: " + telemetry.completionsThisFrame
                        font.pixelSize: 11; color: "#888"
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#404040" }

                // SECTION: SYSTEM RESOURCES
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { text: "System Resources"; font.pixelSize: 16; font.bold: true; color: "#ffffff" }

                    // CPU
                    ColumnLayout {
                        Layout.fillWidth: true
                        RowLayout {
                            Text { text: "Total System CPU"; font.pixelSize: 13; color: "#ccc"; Layout.fillWidth: true }
                            Text { text: telemetry.cpuUsage.toFixed(1) + "%"; font.bold: true; color: "#ffffff" }
                        }
                        UsageGraph {
                            Layout.fillWidth: true; height: 40
                            color: "#4CAF50"; maxValue: 100
                            dataPoints: telemetry.cpuHistory
                        }
                        Text { 
                            text: "App Process: " + (systemMonitor ? systemMonitor.cpuUsage.toFixed(1) : "0.0") + "%"
                            font.pixelSize: 11; color: "#888" 
                        }
                    }

                    // RAM & Cache
                    ColumnLayout {
                        Layout.fillWidth: true
                        RowLayout {
                            Text { text: "System RAM Usage"; font.pixelSize: 13; color: "#ccc"; Layout.fillWidth: true }
                            Text { 
                                text: (telemetry.ramHistory.length > 0 ? telemetry.ramHistory[telemetry.ramHistory.length-1].toFixed(1) : "0.0") + "%"
                                font.bold: true; color: "#2196F3" 
                            }
                        }
                        UsageGraph {
                            Layout.fillWidth: true; height: 40
                            color: "#2196F3"; maxValue: 100
                            dataPoints: telemetry.ramHistory
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "App: " + telemetry.memoryUsageMB + " MB"; font.pixelSize: 11; color: "#888"; Layout.fillWidth: true }
                            Text { 
                                text: "Avail: " + (systemMonitor ? Math.round(systemMonitor.availableSystemMemoryMB) : "0") + " MB"
                                font.pixelSize: 11; color: "#888" 
                            }
                        }
                    }

                    // GPU Load
                    ColumnLayout {
                        Layout.fillWidth: true
                        RowLayout {
                            Text { text: "GPU Load"; font.pixelSize: 13; color: "#ccc"; Layout.fillWidth: true }
                            Text { text: systemMonitor.gpuUsage.toFixed(1) + "%"; font.bold: true; color: "#FF00FF" }
                        }
                        UsageGraph {
                            Layout.fillWidth: true; height: 40
                            color: "#FF00FF"; maxValue: 100
                            dataPoints: telemetry.gpuHistory
                        }
                    }

                    // VRAM Usage
                    ColumnLayout {
                        Layout.fillWidth: true
                        RowLayout {
                            Text { text: "VRAM Usage"; font.pixelSize: 13; color: "#ccc"; Layout.fillWidth: true }
                            Text { text: Math.round(systemMonitor.gpuVramUsedMB) + " MB"; font.bold: true; color: "#FF9800" }
                        }
                        Text { 
                            text: "Total VRAM: " + Math.round(systemMonitor.gpuVramTotalMB) + " MB"
                            font.pixelSize: 11; color: "#888" 
                        }
                    }

                    // Cache Control
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Image Cache Limit: " + settings.cacheSizeMB + " MB"; font.pixelSize: 12; color: "#999" }
                        Slider {
                            Layout.fillWidth: true; from: 64; to: 2048; stepSize: 64
                            value: settings.cacheSizeMB
                            onMoved: settings.cacheSizeMB = Math.round(value)
                        }
                        Button {
                            Layout.fillWidth: true
                            text: "Clear Disk Cache"
                            onClicked: settings.clearDiskCache()
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#404040" }

                // SECTION: DECODING & OPTIMIZATIONS
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Decoding Performance"; font.pixelSize: 16; font.bold: true; color: "#ffffff"; Layout.fillWidth: true }
                        Button {
                            text: "Reset"
                            onClicked: telemetry.resetStats()
                            implicitHeight: 20
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        ColumnLayout {
                            Text { text: "Last Load Latency"; font.pixelSize: 11; color: "#999" }
                            Text { text: telemetry.lastLoadTime + " ms"; font.pixelSize: 18; font.bold: true; color: "#fff" }
                        }
                        ColumnLayout {
                            Text { text: "Moving Avg"; font.pixelSize: 11; color: "#999" }
                            Text { text: telemetry.averageLoadTime + " ms"; font.pixelSize: 18; font.bold: true; color: "#fff" }
                        }
                    }
                    
                    Text {
                        text: "Cache Hit Rate: " + telemetry.cacheHitRate.toFixed(1) + "%"
                        font.pixelSize: 12; color: "#999"
                    }

                    // Toggles
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Persistent Disk Cache"; font.pixelSize: 12; color: "#ccc"; Layout.fillWidth: true }
                        Switch { checked: settings.useDiskCache; onToggled: settings.useDiskCache = checked }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "RAW Embedded Preview"; font.pixelSize: 12; color: "#ccc"; Layout.fillWidth: true }
                        Switch { checked: settings.rawAcceleration; onToggled: settings.rawAcceleration = checked }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["Auto", "D3D11VA", "Vulkan (experimental)", "OpenCL (experimental)"]
                        currentIndex: settings.videoAcceleration
                        onActivated: (index) => settings.videoAcceleration = index
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Viewport Culling"; font.pixelSize: 12; color: "#ccc"; Layout.fillWidth: true }
                        Switch { checked: imageModel.viewportCullingEnabled; onToggled: imageModel.viewportCullingEnabled = checked }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#404040" }

                // SECTION: VIEW SETTINGS
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { text: "View Settings"; font.pixelSize: 16; font.bold: true; color: "#ffffff" }

                    ComboBox {
                        Layout.fillWidth: true
                        model: ["Standard Grid", "Albums", "Semantic Rows"]
                        currentIndex: root.currentView
                        onActivated: (index) => root.currentView = index
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Grid Cell Size: " + settings.gridSize + "px"; font.pixelSize: 12; color: "#ccc" }
                        Slider {
                            Layout.fillWidth: true; from: 20; to: 400; stepSize: 5
                            value: settings.gridSize
                            onMoved: settings.gridSize = Math.round(value)
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Thumbnail Quality: " + settings.thumbnailSize + "px"; font.pixelSize: 12; color: "#ccc" }
                        Slider {
                            Layout.fillWidth: true; from: 20; to: 512; stepSize: 1
                            value: settings.thumbnailSize
                            onMoved: settings.thumbnailSize = Math.round(value)
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#404040" }

                // GPU Info & Help
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        text: "GPU: " + (systemMonitor ? systemMonitor.gpuName : "Unknown")
                        font.pixelSize: 11; color: "#888"; wrapMode: Text.Wrap; Layout.fillWidth: true
                    }
                    Text {
                        text: "📊 Scroll to see performance metrics\n" +
                              "🎯 Toggle optimizations to compare\n" +
                              "⚡ Target: 30+ FPS sustained"
                        font.pixelSize: 11
                        color: "#666666"
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

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