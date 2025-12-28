# System Monitoring Status Report

## Current Implementation

### ✅ What's Being Tracked (SystemMonitor.cpp/h):

| Metric | Type | Property Name | Status |
|--------|------|---------------|--------|
| **CPU - App** | Application-specific | `cpuUsage` | ✅ Tracked |
| **CPU - System** | System-wide | `systemCpuUsage` | ✅ Tracked |
| **RAM - App** | Application-specific | `memoryUsageMB` | ✅ Tracked |
| **RAM - Total** | System-wide | `totalSystemMemoryMB` | ✅ Tracked |
| **RAM - Available** | System-wide | `availableSystemMemoryMB` | ✅ Tracked |
| **GPU Load** | System-wide (all processes) | `gpuUsage` | ✅ Tracked |
| **VRAM - Used** | Application-specific | `gpuVramUsedMB` | ✅ Tracked |
| **VRAM - Total** | Hardware limit | `gpuVramTotalMB` | ✅ Tracked |
| **GPU Name** | Hardware info | `gpuName` | ✅ Tracked |

### ❌ What's Missing from Display:

**PerformanceOverlay.qml does NOT show SystemMonitor metrics!**

Currently displayed:
- FPS (from TelemetryMonitor)
- Frame times
- Pending decodes
- **NO CPU/GPU/RAM stats visible**

---

## Sample Log Output (Currently Hidden from UI):

```
[00:37:01.503][SystemMonitor] Stats Update:
  CPU App: 10.5903 % | Sys: 66.4931 %
  | RAM App: 161.301 MB
  | Sys Avail: 34308.3 MB / 65466.5 MB

[SystemMonitor] Resource Update:
  RAM: 161.301 MB (Delta: 161.301 )
  | VRAM: 29.2773 MB (Delta: 29.2773 )
```

**This rich data exists but isn't shown in the UI!**

---

## Proposed Solution:

### Add System Stats Display to PerformanceOverlay

```qml
// SECTION: SYSTEM RESOURCES
ColumnLayout {
    Layout.fillWidth: true
    spacing: 5
    
    Text {
        text: "System Resources"
        font.pixelSize: 16
        font.bold: true
        color: "#ffffff"
    }
    
    // CPU Usage
    RowLayout {
        Layout.fillWidth: true
        Text {
            text: "CPU"
            font.pixelSize: 12
            color: "#999999"
            Layout.minimumWidth: 80
        }
        
        // Application CPU
        Rectangle {
            Layout.fillWidth: true
            height: 20
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
            }
        }
        
        // System CPU
        Rectangle {
            Layout.fillWidth: true
            height: 20
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
                text: "Sys: " + systemMonitor.systemCpuUsage.toFixed(1) + "%"
                font.pixelSize: 10
                color: "#ffffff"
            }
        }
    }
    
    // GPU Usage
    Row Layout {
        Layout.fillWidth: true
        Text {
            text: "GPU"
            font.pixelSize: 12
            color: "#999999"
            Layout.minimumWidth: 80
        }
        
        Rectangle {
            Layout.fillWidth: true
            height: 20
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
                text: systemMonitor.gpuUsage.toFixed(1) + "% | " + 
                      systemMonitor.gpuVramUsedMB.toFixed(0) + " / " +
                      systemMonitor.gpuVramTotalMB.toFixed(0) + " MB"
                font.pixelSize: 10
                color: "#ffffff"
            }
        }
    }
    
    // RAM Usage
    RowLayout {
        Layout.fillWidth: true
        Text {
            text: "RAM"
            font.pixelSize: 12
            color: "#999999"
            Layout.minimumWidth: 80
        }
        
        // Application RAM
        Rectangle {
            Layout.fillWidth: true
            height: 20
            color: "#1e1e1e"
            border.color: "#404040"
            border.width: 1
            
            Rectangle {
                width: parent.width * (systemMonitor.memoryUsageMB / systemMonitor.totalSystemMemoryMB)
                height: parent.height
                color: "#00BCD4"
            }
            
            Text {
                anchors.centerIn: parent
                text: "App: " + systemMonitor.memoryUsageMB.toFixed(0) + " MB"
                font.pixelSize: 10
                color: "#ffffff"
            }
        }
        
        // System RAM
        Rectangle {
            Layout.fillWidth: true
            height: 20
            color: "#1e1e1e"
            border.color: "#404040"
            border.width: 1
            
            Rectangle {
                width: parent.width * ((systemMonitor.totalSystemMemoryMB - systemMonitor.availableSystemMemoryMB) / systemMonitor.totalSystemMemoryMB)
                height: parent.height
                color: "#FF5722"
            }
            
            Text {
                anchors.centerIn: parent
                text: "Sys: " + (systemMonitor.totalSystemMemoryMB - systemMonitor.availableSystemMemoryMB).toFixed(0) + " / " +
                      systemMonitor.totalSystemMemoryMB.toFixed(0) + " MB"
                font.pixelSize: 10
                color: "#ffffff"
            }
        }
    }
}
```

---

## Benefits:

1. **Clear Separation**: App vs System usage side-by-side
2. **Visual Bars**: Instant understanding of resource utilization
3. **Color Coding**: Warnings when resources are high
4. **Comprehensive**: CPU, GPU, and RAM in one view
5. **Already Implemented**: Just needs UI wiring

---

## Recommendation:

**Add this stats section to PerformanceOverlay.qml** (~5 minutes work)

All the data is already being tracked and logged, it's just not visible in the UI!
