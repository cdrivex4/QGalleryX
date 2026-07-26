import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: 250
    height: mainLayout.height + 20
    
    property int loadTime: 0
    property string apiName: "Unknown"
    property int fps: 0
    property bool isLoading: false
    property int loadedCount: 0
    property int totalCount: 0
    property int activeThreadCount: 0
    
    visible: true // Visible by default for testing
    
    focus: true
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Tab) {
            root.visible = !root.visible
            event.accepted = true
        }
    }
    
    DragHandler {
        target: root
    }

    Rectangle {
        anchors.fill: parent
        color: "#AA000000"
        radius: 10
        border.color: "#444"
        border.width: 1
    }
    
    Column {
        id: mainLayout
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 10
        
        // Header / Tab Bar
        RowLayout {
            width: parent.width
            spacing: 10
            
            Text {
                text: "Stats"
                color: root.currentTab === "Stats" ? "#00FF00" : "#888"
                font.bold: true
                font.pixelSize: 14
                MouseArea { anchors.fill: parent; onClicked: root.currentTab = "Stats" }
            }
            
            Text {
                text: "|"
                color: "#444"
            }
            
            Text {
                text: "Settings"
                color: root.currentTab === "Settings" ? "#00FF00" : "#888"
                font.bold: true
                font.pixelSize: 14
                MouseArea { anchors.fill: parent; onClicked: root.currentTab = "Settings" }
            }
            
            Item { Layout.fillWidth: true } // Spacer
            
            Text {
                text: "FPS: " + root.fps
                color: "white"
                font.pixelSize: 12
            }
        }
        
        Rectangle { height: 1; width: parent.width; color: "#444" }
        
        // STATS TAB
        Column {
            visible: root.currentTab === "Stats"
            width: parent.width
            spacing: 5
            
            Text { text: "API: " + root.apiName; color: "white"; font.pixelSize: 12 }
            
            Text {
                text: "Status: " + (root.isLoading ? "Scanning..." : "Idle")
                color: root.isLoading ? "#FFA500" : "#00FF00"
                font.pixelSize: 12
            }

            Text {
                text: "Loaded: " + root.loadedCount + " / " + (root.totalCount > 0 ? root.totalCount : "?")
                color: "white"
                font.pixelSize: 12
                visible: root.isLoading || root.totalCount > 0
            }

            Text {
                text: "Active Threads: " + root.activeThreadCount
                color: "white"
                font.pixelSize: 12
            }
            
            Text {
                text: "Last Load: " + root.loadTime + " ms"
                color: "white"
                font.pixelSize: 12
            }

            Text {
                text: "Avg: " + (root.loadCount > 0 ? Math.round(root.totalLoadTime / root.loadCount) : 0) + " ms"
                color: "#aaa"
                font.pixelSize: 12
            }

            Text {
                text: "Min: " + (root.minLoadTime === 99999 ? 0 : root.minLoadTime) + " ms / Max: " + root.maxLoadTime + " ms"
                color: "#aaa"
                font.pixelSize: 12
            }
            
            Text {
                text: "RAM Cache: " + (root.cacheCost / 1024).toFixed(1) + " / " + (root.cacheMax / 1024).toFixed(1) + " MB"
                color: "white"
                font.pixelSize: 12
            }
            
            Rectangle { height: 1; width: parent.width; color: "#444" }
            
            UsageGraph {
                id: cpuGraph
                width: parent.width; height: 40
                label: "CPU"
                color: "#00FF00"
                maxValue: 100
                dataPoints: root.cpuHistory
            }
            
            UsageGraph {
                id: ramGraph
                width: parent.width; height: 40
                label: "RAM"
                color: "#00FFFF"
                suffix: " MB"
                maxValue: 1024
                dataPoints: root.ramHistory
            }
            
            Text {
                text: "GPU: " + systemMonitor.gpuName
                color: "white"
                font.pixelSize: 10
                wrapMode: Text.Wrap
                width: parent.width
            }
            
            Text {
                text: "VRAM: " + Math.round(systemMonitor.gpuVramUsedMB) + " / " + Math.round(systemMonitor.gpuVramTotalMB) + " MB"
                color: "#aaa"
                font.pixelSize: 10
            }

            UsageGraph {
                id: gpuGraph
                width: parent.width; height: 40
                label: "GPU Load %"
                color: "#FF00FF"
                maxValue: 100
                dataPoints: root.gpuHistory
            }
            
            UsageGraph {
                id: vramGraph
                width: parent.width; height: 40
                label: "VRAM %"
                color: "#FFA500" // Orange
                maxValue: 100
                dataPoints: root.vramHistory
            }
        }
        
        // SETTINGS TAB
        Column {
            visible: root.currentTab === "Settings"
            width: parent.width
            spacing: 8
            
            // Thumbnail Resolution
            Text { text: "Thumb Resolution: " + Math.round(resSlider.value) + "px"; color: "white"; font.pixelSize: 11 }
            Slider {
                id: resSlider
                width: parent.width
                from: 20; to: 400
                stepSize: 1
                value: appSettings.thumbnailSize
                onPressedChanged: {
                    if (!pressed) {
                        appSettings.thumbnailSize = Math.round(value)
                    }
                }
            }

            // Grid Zoom
            Text { text: "Grid Resolution: " + Math.round(zoomSlider.value) + "px"; color: "white"; font.pixelSize: 11 }
            Slider {
                id: zoomSlider
                width: parent.width
                from: 20; to: 400
                stepSize: 1
                value: appSettings.gridResolution
                onPressedChanged: {
                    if (!pressed) {
                        appSettings.gridResolution = Math.round(value)
                    }
                }
            }

            // Cache Size
            Text { text: "RAM Cache: " + Math.round(cacheSlider.value) + " MB"; color: "white"; font.pixelSize: 11 }
            Slider {
                id: cacheSlider
                width: parent.width
                from: 128; to: 4096
                stepSize: 1
                value: appSettings.cacheSizeMB
                onPressedChanged: {
                    if (!pressed) {
                        appSettings.cacheSizeMB = Math.round(value)
                    }
                }
            }

            // Disk Cache Size
            Text { text: "Disk Cache: " + Math.round(diskCacheSlider.value) + " MB"; color: "white"; font.pixelSize: 11 }
            Slider {
                id: diskCacheSlider
                width: parent.width
                from: 512; to: 16384
                stepSize: 256
                value: appSettings.diskCacheSizeMB
                onPressedChanged: {
                    if (!pressed) {
                        appSettings.diskCacheSizeMB = Math.round(value)
                    }
                }
            }
            
            // Threads
            Text { text: "Threads: " + threadSlider.value + " (Restart)"; color: "white"; font.pixelSize: 11 }
            Slider {
                id: threadSlider
                width: parent.width
                from: 1; to: 16
                stepSize: 1
                value: appSettings.concurrentThreads
                onPressedChanged: {
                    if (!pressed) {
                        appSettings.concurrentThreads = Math.round(value)
                    }
                }
            }

            // Log Level
            Text { text: "Log Level: " + logLevelCombo.currentText; color: "white"; font.pixelSize: 11 }
            
            // Raw Acceleration
            Text { text: "RAW: " + (appSettings.rawAcceleration ? "Fast Preview" : "Full Decode"); color: "white"; font.pixelSize: 11 }
            CheckBox {
                text: "Use Embedded Preview"
                checked: appSettings.rawAcceleration
                onCheckedChanged: appSettings.rawAcceleration = checked
                width: parent.width
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#00FF00" : "#ccc"
                    leftPadding: parent.indicator.width + parent.spacing
                    verticalAlignment: Text.AlignVCenter
                }
            }

            ComboBox {
                id: logLevelCombo
                width: parent.width
                model: ["None", "Basic", "Verbose"]
                currentIndex: appSettings.logLevel
                onActivated: appSettings.logLevel = currentIndex
            }
        }
    }
    
    property string currentTab: "Stats"
    
    property var cpuHistory: []
    property var ramHistory: []
    property var gpuHistory: []
    property var vramHistory: []
    
    Timer {
        interval: 1000
        running: root.visible
        repeat: true
        onTriggered: {
            var cpu = systemMonitor.cpuUsage
            var ram = systemMonitor.memoryUsageMB
            var gpu = systemMonitor.gpuUsage
            
            // Update CPU History
            var newCpu = root.cpuHistory.slice()
            newCpu.push(cpu)
            if (newCpu.length > 50) newCpu.shift()
            root.cpuHistory = newCpu
            
            // Update RAM History
            var newRam = root.ramHistory.slice()
            newRam.push(ram)
            if (newRam.length > 50) newRam.shift()
            root.ramHistory = newRam
            
            // Update GPU History
            var newGpu = root.gpuHistory.slice()
            newGpu.push(gpu)
            if (newGpu.length > 50) newGpu.shift()
            root.gpuHistory = newGpu
            
            // Update VRAM History
            var vramPct = 0
            if (systemMonitor.gpuVramTotalMB > 0) {
                vramPct = (systemMonitor.gpuVramUsedMB / systemMonitor.gpuVramTotalMB) * 100.0
            }
            var newVram = root.vramHistory.slice()
            newVram.push(vramPct)
            if (newVram.length > 50) newVram.shift()
            root.vramHistory = newVram
            
            // Dynamic RAM Max
            if (ram > ramGraph.maxValue) ramGraph.maxValue = ram * 1.2
        }
    }
    
    property int minLoadTime: 99999
    property int maxLoadTime: 0
    property int totalLoadTime: 0
    property int loadCount: 0
    
    property int cacheCost: 0
    property int cacheMax: 0
    
    function reportLoadTime(timeMs) {
        root.loadTime = timeMs
        root.totalLoadTime += timeMs
        root.loadCount++
        if (timeMs < root.minLoadTime) root.minLoadTime = timeMs
        if (timeMs > root.maxLoadTime) root.maxLoadTime = timeMs
    }
    
    Timer {
        interval: 1000
        running: root.visible
        repeat: true
        onTriggered: {
            var stats = appSettings.getCacheStats()
            if (stats) {
                root.cacheCost = stats.totalCost
                root.cacheMax = stats.maxCost
            }
        }
    }
    
    // FPS Counter
    property int frameCount: 0
    property int lastTime: 0
    
    FrameAnimation {
        running: root.visible
        onRunningChanged: if (running) root.lastTime = new Date().getTime()
        onTriggered: {
            root.frameCount++
            var now = new Date().getTime()
            if (root.lastTime === 0) root.lastTime = now
            if (now - root.lastTime >= 1000) {
                root.fps = root.frameCount
                root.frameCount = 0
                root.lastTime = now
            }
        }
    }
}
