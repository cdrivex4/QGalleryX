import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: 380
    height: mainLayout.height + 20
    
    property int loadTime: 0
    property string apiName: "Unknown"
    property string scanEngine: ""
    property int scanDuration: 0
    property int fps: 0
    property bool isLoading: false
    property int loadedCount: 0
    property int totalCount: 0
    property int activeThreadCount: 0
    property int precacheMode: 1
    property int crawlerIndex: 0
    property int crawlerTotal: 0
    property double crawlerProgress: 0.0
    property int activeJobs: 0
    
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

            Text {
                text: "|"
                color: "#444"
            }
            
            Text {
                text: "Cache"
                color: root.currentTab === "Cache" ? "#00FF00" : "#888"
                font.bold: true
                font.pixelSize: 14
                MouseArea { anchors.fill: parent; onClicked: root.currentTab = "Cache" }
            }
            
            Item { Layout.fillWidth: true } // Spacer
            
            Text {
                text: "FPS: " + root.fps
                color: "white"
                font.pixelSize: 12
            }
            
            Button {
                text: "✖"
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
                background: Rectangle { color: "transparent" }
                contentItem: Text {
                    text: parent.text
                    color: parent.hovered ? "#FFF" : "#888"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.visible = false
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
                text: "Scan Engine: " + (root.scanEngine !== "" ? root.scanEngine : "Idle") + (root.scanDuration > 0 ? " (" + root.scanDuration + " ms)" : "")
                color: root.scanEngine.indexOf("MFT") !== -1 ? "#00FF00" : (root.scanEngine.indexOf("Cached") !== -1 ? "#00FFFF" : "#FFA500")
                font.pixelSize: 12
                font.bold: true
            }

            Text {
                text: "Directory Scan: " + (root.isLoading ? "Scanning..." : "Complete") + " (" + root.loadedCount + " / " + (root.totalCount > 0 ? root.totalCount : "?") + ")"
                color: root.isLoading ? "#FFA500" : "#00FF00"
                font.pixelSize: 12
            }

            Text {
                text: "Crawler: " + (root.precacheMode === 0 ? "Battery Saver (Off)" : (root.precacheMode === 1 ? "Lookahead Window (±50)" : "Aggressive Full Crawl"))
                color: root.precacheMode === 0 ? "#888" : (root.precacheMode === 1 ? "#FFD700" : "#FF4444")
                font.pixelSize: 12
                font.bold: true
            }

            Text {
                visible: root.precacheMode !== 0 && root.crawlerTotal > 0
                text: "Precached: " + root.crawlerIndex + " / " + root.crawlerTotal + " (" + Math.round(root.crawlerProgress * 100) + "%)" + (root.activeJobs > 0 ? " • " + root.activeJobs + " active tasks" : " • Idle")
                color: "white"
                font.pixelSize: 12
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
            
            Text {
                text: "Cache Hits: " + root.l1Hits + " (RAM) | " + root.l2Hits + " (Disk) | " + root.misses + " (Miss)"
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
            spacing: 10
            
            // Scan Engine Mode
            Text { text: "Scan Engine Mode:"; color: "#e0e0e0"; font.pixelSize: 11; font.bold: true }
            RowLayout {
                width: parent.width
                spacing: 5
                
                Repeater {
                    model: [
                        { label: "Auto (Cascade)", mode: 0 },
                        { label: "Force MFT", mode: 1 },
                        { label: "Force Iterator", mode: 2 }
                    ]
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        height: 24
                        radius: 4
                        color: appSettings.scanEngineMode === modelData.mode ? "#00FF00" : "#333"
                        border.color: "#555"
                        
                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: appSettings.scanEngineMode === modelData.mode ? "#000" : "#FFF"
                            font.pixelSize: 10
                            font.bold: true
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: appSettings.scanEngineMode = modelData.mode
                        }
                    }
                }
            }

            // Thumbnail Resolution
            Text { text: "Thumb Resolution: " + appSettings.thumbnailSize + "px"; color: "#e0e0e0"; font.pixelSize: 11; font.bold: true }
            Slider {
                id: resSlider
                width: parent.width
                from: 64; to: 384
                stepSize: 1
                value: appSettings.thumbnailSize
                onPressedChanged: if (!pressed) appSettings.thumbnailSize = appSettings.snapThumbnailResolution(value)

                background: Rectangle {
                    x: resSlider.leftPadding; y: resSlider.topPadding + resSlider.availableHeight / 2 - height / 2
                    width: resSlider.availableWidth; height: 6; radius: 3; color: "#30ffffff"
                    Rectangle { width: resSlider.position * parent.width; height: parent.height; color: "#ffffff"; radius: 3 }
                }
                handle: Rectangle {
                    x: resSlider.leftPadding + resSlider.visualPosition * (resSlider.availableWidth - width)
                    y: resSlider.topPadding + resSlider.availableHeight / 2 - height / 2
                    implicitWidth: 16; implicitHeight: 16; radius: 8; color: resSlider.pressed ? "#e0e0e0" : "#ffffff"
                }
            }

            // Grid Zoom
            Text { text: "Grid Resolution: " + appSettings.gridResolution + "px"; color: "#e0e0e0"; font.pixelSize: 11; font.bold: true }
            Slider {
                id: zoomSlider
                width: parent.width
                from: 64; to: 384
                stepSize: 1
                value: appSettings.gridResolution
                onPressedChanged: if (!pressed) appSettings.gridResolution = appSettings.snapThumbnailResolution(value)

                background: Rectangle {
                    x: zoomSlider.leftPadding; y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                    width: zoomSlider.availableWidth; height: 6; radius: 3; color: "#30ffffff"
                    Rectangle { width: zoomSlider.position * parent.width; height: parent.height; color: "#ffffff"; radius: 3 }
                }
                handle: Rectangle {
                    x: zoomSlider.leftPadding + zoomSlider.visualPosition * (zoomSlider.availableWidth - width)
                    y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                    implicitWidth: 16; implicitHeight: 16; radius: 8; color: zoomSlider.pressed ? "#e0e0e0" : "#ffffff"
                }
            }

            // RAM Cache Size
            Text { text: "RAM Cache: " + Math.round(cacheSlider.value) + " MB"; color: "#e0e0e0"; font.pixelSize: 11; font.bold: true }
            Slider {
                id: cacheSlider
                width: parent.width
                from: 128; to: 4096
                stepSize: 1
                value: appSettings.cacheSizeMB
                onPressedChanged: if (!pressed) appSettings.cacheSizeMB = Math.round(value)

                background: Rectangle {
                    x: cacheSlider.leftPadding; y: cacheSlider.topPadding + cacheSlider.availableHeight / 2 - height / 2
                    width: cacheSlider.availableWidth; height: 6; radius: 3; color: "#30ffffff"
                    Rectangle { width: cacheSlider.position * parent.width; height: parent.height; color: "#ffffff"; radius: 3 }
                }
                handle: Rectangle {
                    x: cacheSlider.leftPadding + cacheSlider.visualPosition * (cacheSlider.availableWidth - width)
                    y: cacheSlider.topPadding + cacheSlider.availableHeight / 2 - height / 2
                    implicitWidth: 16; implicitHeight: 16; radius: 8; color: cacheSlider.pressed ? "#e0e0e0" : "#ffffff"
                }
            }

            // Disk Cache Size
            Text { text: "Disk Cache: " + Math.round(diskCacheSlider.value) + " MB"; color: "#e0e0e0"; font.pixelSize: 11; font.bold: true }
            Slider {
                id: diskCacheSlider
                width: parent.width
                from: 512; to: 16384
                stepSize: 256
                value: appSettings.diskCacheSizeMB
                onPressedChanged: if (!pressed) appSettings.diskCacheSizeMB = Math.round(value)

                background: Rectangle {
                    x: diskCacheSlider.leftPadding; y: diskCacheSlider.topPadding + diskCacheSlider.availableHeight / 2 - height / 2
                    width: diskCacheSlider.availableWidth; height: 6; radius: 3; color: "#30ffffff"
                    Rectangle { width: diskCacheSlider.position * parent.width; height: parent.height; color: "#ffffff"; radius: 3 }
                }
                handle: Rectangle {
                    x: diskCacheSlider.leftPadding + diskCacheSlider.visualPosition * (diskCacheSlider.availableWidth - width)
                    y: diskCacheSlider.topPadding + diskCacheSlider.availableHeight / 2 - height / 2
                    implicitWidth: 16; implicitHeight: 16; radius: 8; color: diskCacheSlider.pressed ? "#e0e0e0" : "#ffffff"
                }
            }

            // Threads
            Text { text: "Threads: " + threadSlider.value + " (Restart)"; color: "#e0e0e0"; font.pixelSize: 11; font.bold: true }
            Slider {
                id: threadSlider
                width: parent.width
                from: 1; to: 16
                stepSize: 1
                value: appSettings.concurrentThreads
                onPressedChanged: if (!pressed) appSettings.concurrentThreads = Math.round(value)

                background: Rectangle {
                    x: threadSlider.leftPadding; y: threadSlider.topPadding + threadSlider.availableHeight / 2 - height / 2
                    width: threadSlider.availableWidth; height: 6; radius: 3; color: "#30ffffff"
                    Rectangle { width: threadSlider.position * parent.width; height: parent.height; color: "#ffffff"; radius: 3 }
                }
                handle: Rectangle {
                    x: threadSlider.leftPadding + threadSlider.visualPosition * (threadSlider.availableWidth - width)
                    y: threadSlider.topPadding + threadSlider.availableHeight / 2 - height / 2
                    implicitWidth: 16; implicitHeight: 16; radius: 8; color: threadSlider.pressed ? "#e0e0e0" : "#ffffff"
                }
            }

            // Disk Latency Warning Toasts
            CheckBox {
                text: "Disk Latency Warning Toasts"
                checked: appSettings.showLatencyToasts
                onCheckedChanged: appSettings.showLatencyToasts = checked
                width: parent.width
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#00FF00" : "#ccc"
                    leftPadding: parent.indicator.width + parent.spacing
                    font.pixelSize: 11
                    verticalAlignment: Text.AlignVCenter
                }
            }

            // Raw Acceleration
            CheckBox {
                text: "RAW: Use Embedded Preview"
                checked: appSettings.rawAcceleration
                onCheckedChanged: appSettings.rawAcceleration = checked
                width: parent.width
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#00FF00" : "#ccc"
                    leftPadding: parent.indicator.width + parent.spacing
                    font.pixelSize: 11
                    verticalAlignment: Text.AlignVCenter
                }
            }

            // Log Level
            Text { text: "Log Level:"; color: "#e0e0e0"; font.pixelSize: 11; font.bold: true }
            ComboBox {
                id: logLevelCombo
                width: parent.width
                model: ["None", "Basic", "Verbose"]
                currentIndex: appSettings.logLevel
                onActivated: appSettings.logLevel = currentIndex
            }

            // Thread Scheduler Governor
            Text { text: "Thread Governor:"; color: "#e0e0e0"; font.pixelSize: 11; font.bold: true }
            ComboBox {
                id: governorCombo
                width: parent.width
                model: ["Sequential (FIFO)", "Viewport First (LIFO)", "Adaptive (Hybrid)", "Round Robin (1:1)"]
                currentIndex: typeof taskScheduler !== "undefined" ? taskScheduler.schedulerGovernor : 0
                onActivated: {
                    if (typeof taskScheduler !== "undefined") {
                        taskScheduler.schedulerGovernor = currentIndex
                    }
                }
            }
        }

        // CACHE TAB
        Column {
            visible: root.currentTab === "Cache"
            width: parent.width
            spacing: 8
            
            Text { text: "Cache DB Storage & Drives:"; color: "#e0e0e0"; font.pixelSize: 12; font.bold: true }
            
            // Rebuild Cache Button (Re-crawl active directory and promote to RAM)
            Button {
                text: "⟳  Rebuild Cache / Re-Crawl"
                width: parent.width
                height: 32
                background: Rectangle {
                    color: parent.pressed ? "#113355" : (parent.hovered ? "#224466" : "#1a2a3a")
                    radius: 6
                    border.color: "#336699"
                }
                contentItem: Text {
                    text: parent.text
                    color: "#88ccff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 12
                    font.bold: true
                }
                onClicked: {
                    root.rebuildCacheRequested()
                }
            }

            // Per-Drive Cache Breakdown (synchronized with Menu)
            Repeater {
                id: osdCacheStatsRepeater
                property var statsMap: (typeof appSettings !== "undefined" && typeof appSettings.getTrackedRootPathStats === "function") ? appSettings.getTrackedRootPathStats() : ({})
                property var statKeys: Object.keys(statsMap)

                function refresh() {
                    if (typeof appSettings !== "undefined" && typeof appSettings.getTrackedRootPathStats === "function") {
                        var res = appSettings.getTrackedRootPathStats()
                        statsMap = res
                        statKeys = Object.keys(res)
                    }
                }

                model: statKeys
                delegate: Rectangle {
                    width: parent.width
                    height: 36
                    color: "#222"
                    border.color: "#3d3d3d"
                    radius: 6
                    
                    property var itemData: osdCacheStatsRepeater.statsMap[modelData]
                    property int count: itemData ? (itemData.count || 0) : 0
                    property real mb: itemData ? ((itemData.bytes || 0) / (1024 * 1024)) : 0.0

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 8

                        Text {
                            text: modelData === "__total__" ? "Total Stats" : ("Drive: " + modelData)
                            color: modelData === "__total__" ? "#FFD700" : "white"
                            font.bold: true
                            font.pixelSize: 11
                            Layout.preferredWidth: 80
                        }

                        Text {
                            text: count + " items (" + mb.toFixed(1) + " MB)"
                            color: "#aaa"
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            width: modelData === "__total__" ? 56 : 48
                            height: 24
                            radius: 4
                            color: nukeBtnMa.pressed ? "#661818" : (nukeBtnMa.containsMouse ? "#882222" : "#333")
                            border.color: modelData === "__total__" ? "#555" : "#772222"
                            visible: count > 0 || modelData === "__total__"

                            Text {
                                anchors.centerIn: parent
                                text: modelData === "__total__" ? "⟳ Ref" : "🗑 Nuke"
                                color: modelData === "__total__" ? "#FFD700" : "#ff7777"
                                font.pixelSize: 10
                                font.bold: true
                            }

                            MouseArea {
                                id: nukeBtnMa
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    if (modelData === "__total__") {
                                        osdCacheStatsRepeater.refresh()
                                    } else {
                                        if (typeof appSettings !== "undefined") {
                                            appSettings.nukeCacheForPath(modelData)
                                            osdCacheStatsRepeater.refresh()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // Nuke All Button
            Button {
                text: "💥 NUKE ALL CACHE DB & FILES"
                width: parent.width
                height: 32
                background: Rectangle {
                    color: parent.pressed ? "#661818" : (parent.hovered ? "#882222" : "#441111")
                    radius: 6
                    border.color: "#ff3333"
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ff9999"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 11
                    font.bold: true
                }
                onClicked: {
                    if (typeof appSettings !== "undefined") {
                        appSettings.nukeDiskCache()
                        osdCacheStatsRepeater.refresh()
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: "#383838"; visible: true }

            Text { text: "Connected Devices & Drives:"; color: "#e0e0e0"; font.pixelSize: 12; font.bold: true }

            Repeater {
                id: mountedDrivesRepeater
                model: (typeof desktopHelper !== "undefined" && typeof desktopHelper.getMountedDrives === "function") ? desktopHelper.getMountedDrives() : []
                delegate: Rectangle {
                    width: parent.width
                    height: 38
                    color: "#1e222a"
                    border.color: "#2f3846"
                    radius: 6

                    property string root: modelData.rootPath || ""
                    property string dName: modelData.name || ""
                    property string dType: modelData.driveType || "FIXED"
                    property real freeGB: (modelData.bytesFree || 0) / (1024 * 1024 * 1024)
                    property real totalGB: (modelData.bytesTotal || 0) / (1024 * 1024 * 1024)

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 8

                        Rectangle {
                            width: 32
                            height: 20
                            radius: 3
                            color: dType === "REMOVABLE" ? "#1a3a2a" : (dType === "REMOTE" ? "#3a2a1a" : "#1a2a3a")
                            border.color: dType === "REMOVABLE" ? "#00FF7F" : (dType === "REMOTE" ? "#FFA500" : "#38BDF8")

                            Text {
                                anchors.centerIn: parent
                                text: dType === "REMOVABLE" ? "USB" : (dType === "REMOTE" ? "NET" : "DRV")
                                color: dType === "REMOVABLE" ? "#00FF7F" : (dType === "REMOTE" ? "#FFA500" : "#38BDF8")
                                font.pixelSize: 9
                                font.bold: true
                            }
                        }

                        Column {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                text: (dName.length > 0 && dName !== root ? dName + " (" + root + ")" : root)
                                color: "white"
                                font.pixelSize: 11
                                font.bold: true
                                elide: Text.ElideRight
                                width: parent.width
                            }

                            Text {
                                text: freeGB.toFixed(1) + " GB free / " + totalGB.toFixed(1) + " GB (" + (modelData.fileSystemType || "") + ")"
                                color: "#888"
                                font.pixelSize: 9
                            }
                        }
                    }
                }
            }
        }
    }
    
    signal rebuildCacheRequested()
    property bool cacheRefreshTrigger: false
    
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
    property int l1Hits: 0
    property int l2Hits: 0
    property int misses: 0
    
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
                if (stats.l1Hits !== undefined) {
                    root.l1Hits = stats.l1Hits
                    root.l2Hits = stats.l2Hits
                    root.misses = stats.misses
                }
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
