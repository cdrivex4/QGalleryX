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
                    width: parent.width - root.contentPadding
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
                        Text { text: "Avg: " + telemetry.averageFps + " FPS"; font.pixelSize: 11; color: "#888" }
                    }

                    // Latency Metrics
                    ColumnLayout {
                        Text { text: "Pipeline & Latency"; font.pixelSize: 14; font.bold: true; color: "#aaa" }
                        RowLayout {
                            ColumnLayout { 
                                Text { text: "Avg Latency"; font.pixelSize: 10; color: "#888" } 
                                Text { text: telemetry.averageLoadTime + " ms"; color: "#fff"; font.bold: true } 
                            }
                            Item { Layout.fillWidth: true }
                            ColumnLayout { 
                                Text { text: "Cache Hit"; font.pixelSize: 10; color: "#888" } 
                                Text { text: telemetry.cacheHitRate.toFixed(1) + "%"; color: "#4CAF50"; font.bold: true } 
                            }
                            Item { Layout.fillWidth: true }
                            ColumnLayout { 
                                Text { text: "Last Load"; font.pixelSize: 10; color: "#888" } 
                                Text { text: telemetry.lastLoadTime + " ms"; color: "#fff"; font.bold: true } 
                            }
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
                                text: "App: " + systemMonitor.cpuUsage.toFixed(1) + "% | Total: " + telemetry.cpuUsage.toFixed(1) + "%"
                                font.pixelSize: 10; color: "#fff"
                            }
                        }

                        // RAM
                        ColumnLayout {
                            Text { text: "RAM Usage (App / System)"; font.pixelSize: 11; color: "#888" }
                            UsageGraph {
                                Layout.fillWidth: true; height: 30
                                color: "#2196F3"; maxValue: 100
                                dataPoints: telemetry.ramHistory
                            }
                            Text { 
                                text: "App: " + systemMonitor.memoryUsageMB.toFixed(0) + " MB | Sys: " + (systemMonitor.totalSystemMemoryMB - systemMonitor.availableSystemMemoryMB).toFixed(0) + " / " + systemMonitor.totalSystemMemoryMB.toFixed(0) + " MB"
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
                                text: "Load: " + systemMonitor.gpuUsage.toFixed(1) + "% | VRAM: " + systemMonitor.gpuVramUsedMB.toFixed(0) + " / " + systemMonitor.gpuVramTotalMB.toFixed(0) + " MB"
                                font.pixelSize: 10; color: "#fff"
                            }
                        }
                    }

                    // CPU Features
                    ColumnLayout {
                        Text { text: "Hardware Engine"; font.pixelSize: 14; font.bold: true; color: "#aaa" }
                        Rectangle {
                            Layout.fillWidth: true; height: 45; color: "#111"; radius: 4
                            Text {
                                anchors.fill: parent; anchors.margins: 6
                                text: hwAccel.cpuInfo()
                                font.pixelSize: 10; color: "#8f8"; wrapMode: Text.Wrap; verticalAlignment: Text.AlignVCenter
                            }
                        }
                        Text { text: "Active Mode: " + hwAccel.currentModeName(); font.pixelSize: 10; color: "#888" }
                    }

                    // Task Queue
                    ColumnLayout {
                        Text { text: "Workload Status"; font.pixelSize: 14; font.bold: true; color: "#aaa" }
                        RowLayout {
                            ColumnLayout { 
                                Text { text: "Active"; font.pixelSize: 10; color: "#888" } 
                                Text { text: taskScheduler.activeTaskCount; color: "#fff"; font.bold: true } 
                            }
                            Item { Layout.fillWidth: true }
                            ColumnLayout { 
                                Text { text: "Pending"; font.pixelSize: 10; color: "#888" } 
                                Text { text: imageModel.remainingItems; color: "#fff"; font.bold: true } 
                            }
                            Item { Layout.fillWidth: true }
                            ColumnLayout { 
                                Text { text: "Frame Comp"; font.pixelSize: 10; color: "#888" } 
                                Text { text: telemetry.completionsThisFrame; color: "#fff"; font.bold: true } 
                            }
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
                    width: parent.width - root.contentPadding
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 18
                    
                    Item { height: 10; width: 1 }

                    // Sliders
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Thumb Resolution: " + Math.round(thumbSlider.value) + "px"; color: "#fff"; font.pixelSize: 12 }
                        Slider {
                            id: thumbSlider
                            Layout.fillWidth: true; from: 16; to: 128; stepSize: 4
                            value: settings.thumbnailSize
                            onPressedChanged: {
                                if (!pressed) {
                                    settings.thumbnailSize = Math.round(value)
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Grid Resolution: " + settings.gridResolution + "px"; color: "#fff"; font.pixelSize: 12 }
                        Slider {
                            Layout.fillWidth: true; from: 20; to: 400; stepSize: 4
                            value: settings.gridResolution
                            onPressedChanged: {
                                if (!pressed) {
                                    settings.gridResolution = Math.round(value)
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Cache Size: " + settings.cacheSizeMB + " MB"; color: "#fff"; font.pixelSize: 12 }
                        Slider {
                            Layout.fillWidth: true; from: 128; to: 4096; stepSize: 128
                            value: settings.cacheSizeMB
                            onPressedChanged: {
                                if (!pressed) {
                                    settings.cacheSizeMB = Math.round(value)
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Threads: " + settings.concurrentThreads + " (Restart)"; color: "#fff"; font.pixelSize: 12 }
                        Slider {
                            Layout.fillWidth: true; from: 1; to: 32; stepSize: 1
                            value: settings.concurrentThreads
                            onPressedChanged: {
                                if (!pressed) {
                                    let oldVal = settings.concurrentThreads
                                    let newVal = Math.round(value)
                                    if (oldVal === newVal) return
                                    settings.concurrentThreads = newVal
                                    restartDialog.revertAction = function() { settings.concurrentThreads = oldVal }
                                    restartDialog.open()
                                }
                            }
                        }
                    }

                    // Advanced Optimizations
                    ColumnLayout {
                        spacing: 12
                        Layout.fillWidth: true
                        Text { text: "Optimization Engine"; font.pixelSize: 14; font.bold: true; color: "#aaa" }
                        
                        RowLayout {
                            Text { text: "Viewport Culling: " + (imageModel.viewportCullingEnabled ? "ON" : "OFF"); color: imageModel.viewportCullingEnabled ? "#4CAF50" : "#fff"; font.bold: imageModel.viewportCullingEnabled; Layout.fillWidth: true }
                            Switch { checked: imageModel.viewportCullingEnabled; onToggled: imageModel.viewportCullingEnabled = checked }
                        }
                        
                        RowLayout {
                            Text { text: "Image Engine: " + (settings.useFastImage ? "FastImage (Bypass GC)" : "Standard Image"); color: settings.useFastImage ? "#4CAF50" : "#fff"; font.bold: settings.useFastImage; Layout.fillWidth: true }
                            Switch { checked: settings.useFastImage; onToggled: {
                                let oldVal = !checked
                                settings.useFastImage = checked
                                restartDialog.revertAction = function() { settings.useFastImage = oldVal }
                                restartDialog.open()
                            } }
                        }
                        
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            RowLayout {
                                Text { text: "Frame Budget Scheduler: " + (frameBudget.enabled ? "ON" : "OFF"); color: frameBudget.enabled ? "#4CAF50" : "#fff"; font.bold: frameBudget.enabled; Layout.fillWidth: true }
                                Switch { checked: frameBudget.enabled; onToggled: frameBudget.enabled = checked }
                            }
                            Text { 
                                text: "Target: " + frameBudget.frameBudget + " tasks/frame"
                                font.pixelSize: 10; color: "#888"
                                visible: frameBudget.enabled
                            }
                            Slider {
                                Layout.fillWidth: true; from: 1; to: 100; stepSize: 1
                                value: frameBudget.frameBudget
                                visible: frameBudget.enabled
                                onPressedChanged: {
                                    if (!pressed) {
                                        frameBudget.frameBudget = Math.round(value)
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Text { text: "RAW Preview: " + (settings.rawAcceleration ? "Fast" : "Standard"); color: settings.rawAcceleration ? "#4CAF50" : "#fff"; font.bold: settings.rawAcceleration; Layout.fillWidth: true }
                            Switch { checked: settings.rawAcceleration; onToggled: settings.rawAcceleration = checked }
                        }
                        RowLayout {
                            Text { text: "Disk Cache: " + (settings.useDiskCache ? "ON" : "OFF"); color: settings.useDiskCache ? "#4CAF50" : "#fff"; font.bold: settings.useDiskCache; Layout.fillWidth: true }
                            Switch { checked: settings.useDiskCache; onToggled: {
                                let oldVal = !checked
                                settings.useDiskCache = checked
                                restartDialog.revertAction = function() { settings.useDiskCache = oldVal }
                                restartDialog.open()
                            } }
                        }
                        Text {
                            text: "Path: " + settings.diskCachePath
                            font.pixelSize: 10
                            color: "#888"
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Disk Cache: Native QHash", "Disk Cache: LMDB"]
                            currentIndex: settings.diskCacheDatabaseType
                            onActivated: (index) => {
                                let oldVal = settings.diskCacheDatabaseType
                                if (oldVal === index) return
                                settings.diskCacheDatabaseType = index
                                restartDialog.revertAction = function() { settings.diskCacheDatabaseType = oldVal }
                                restartDialog.open()
                            }
                        }
                        Dialog {
                            id: restartDialog
                            title: "Restart Required"
                            modal: true
                            standardButtons: Dialog.Ok | Dialog.Cancel
                            anchors.centerIn: parent
                            
                            property var revertAction: null
                            Text {
                                text: "These changes require an application restart.\nRestart now?"
                                color: "#fff"
                            }
    
                            onAccepted: {
                                desktopHelper.requestRestart()
                            }
                            
                            onRejected: {
                                if (revertAction) {
                                    revertAction()
                                }
                                outerFrame.closeRequested()
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "Location: " + settings.getDiskCacheLocation()
                            color: "#888"
                            font.pixelSize: 10
                            wrapMode: Text.WrapAnywhere
                        }
                        RowLayout {
                            Text { text: "Show Diagnostics Overlay: " + (settings.showDiagnostics ? "ON" : "OFF"); color: settings.showDiagnostics ? "#4CAF50" : "#fff"; font.bold: settings.showDiagnostics; Layout.fillWidth: true }
                            Switch { checked: settings.showDiagnostics; onToggled: settings.showDiagnostics = checked }
                        }
                        
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Video Accel: Auto", "Video Accel: D3D11VA", "Video Accel: Vulkan", "Video Accel: OpenCL"]
                            currentIndex: settings.videoAcceleration
                            onActivated: (index) => settings.videoAcceleration = index
                        }

                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Log Level: None", "Log Level: Info", "Log Level: Debug", "Log Level: Trace"]
                            currentIndex: settings.logLevel
                            onActivated: (index) => settings.logLevel = index
                        }
                    }

                    // GROUPING DEBUGGER (Requested by USER)
                    ColumnLayout {
                        spacing: 12
                        Layout.fillWidth: true
                        Layout.topMargin: 10
                        Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }
                        Text { text: "Grouping Thresholds"; font.pixelSize: 14; font.bold: true; color: "#FFEB3B" }
                        
                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Year switch < " + root.thresholdYear + "px"; color: "#fff"; font.pixelSize: 11 }
                            Slider {
                                Layout.fillWidth: true; from: 20; to: 400; stepSize: 1
                                value: root.thresholdYear
                                onPressedChanged: {
                                    if (!pressed) {
                                        root.thresholdYear = Math.round(value)
                                    }
                                }
                            }
                        }
                        
                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Month switch < " + root.thresholdMonth + "px"; color: "#fff"; font.pixelSize: 11 }
                            Slider {
                                Layout.fillWidth: true; from: 21; to: 400; stepSize: 1
                                value: root.thresholdMonth
                                onPressedChanged: {
                                    if (!pressed) {
                                        root.thresholdMonth = Math.round(value)
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Week switch < " + root.thresholdWeek + "px"; color: "#fff"; font.pixelSize: 11 }
                            Slider {
                                Layout.fillWidth: true; from: 22; to: 400; stepSize: 1
                                value: root.thresholdWeek
                                onPressedChanged: {
                                    if (!pressed) {
                                        root.thresholdWeek = Math.round(value)
                                    }
                                }
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Clear Disk Cache"
                        onClicked: settings.clearDiskCache()
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5
                        Text {
                            text: "📊 Performance metrics update every 1s"
                            font.pixelSize: 11; color: "#666"
                        }
                        Text {
                            text: "🎯 Toggle optimizations to compare rendering speed"
                            font.pixelSize: 11; color: "#666"
                        }
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
                    text: root.useSemanticView ? "View: Semantic" : "View: Standard Grid"
                    highlighted: true
                    onClicked: {
                        root.useSemanticView = !root.useSemanticView
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
                    visible: root.useSemanticView
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
