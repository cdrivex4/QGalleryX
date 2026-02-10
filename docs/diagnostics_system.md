# Automated Diagnostics System - Implementation Guide

## Overview

I've created a comprehensive automated diagnostics system that runs parallel to the application and continuously monitors key variables, detecting issues before they become problems.

## Components Created

### 1. **DiagnosticsMonitor (C++)** 
Location: `test_scrollbench/src/DiagnosticsMonitor.{h,cpp}`

**Purpose**: Real-time monitoring and validation of critical subsystems

**Features**:
- Runs every 1 second automatically
- Monitors viewport culling state
- Tracks load progress (loaded vs total items)
- Validates settings synchronization
- Checks Adaptive I/O task counts
- Detects anomalies (e.g., "19 items bug", stalled loads, settings mismatches)
- Emits warnings and critical alerts

**Key Checks Performed**:
1. **Viewport Culling**
   - Validates range size is reasonable (> 50 items when enabled)
   - Detects if viewport range is zero or suspiciously small
   - Monitors buffered range size
   
2. **Load Progress**
   - Counts loaded vs total items
   - Monitors pending and staged requests
   - Detects stalls (> 5 seconds with pending requests)
   
3. **Settings Synchronization**
   - Verif

ies disk cache setting matches between SettingsHelper and AsyncImageProvider
   - Alerts on mismatches
   
4. **Adaptive I/O**
   - Monitors active task count
   - Warns if queue gets too large (> 1000 tasks)

### 2. **DiagnosticsOverlay (QML)**
Location: `test_scrollbench/qml/DiagnosticsOverlay.qml`

**Purpose**: Visual display of diagnostics status

**Features**:
- Collapsible overlay (top-left corner)
- Color-coded health indicator (green/orange/red)
- Real-time status updates
- Expandable to show details
- Lists active warnings and critical issues
- Quick action buttons (Toggle Culling, Toggle Cache)
- Critical issue popup alerts

**Visual Indicators**:
- 🟢 Green dot = All systems operational  
- 🟠 Orange dot = Warnings present
- 🔴 Red dot (pulsing) = Critical issues detected

### 3. **DiagnosticRow (QML)**
Location: `test_scrollbench/qml/DiagnosticRow.qml`

**Purpose**: Reusable component for displaying individual diagnostic metrics

## Integration Points

### main_scrollbench.cpp
```cpp
// Create and attach
auto *diagnostics = new DiagnosticsMonitor();
diagnostics->attachModel(imageModel);
diagnostics->attachSettings(settings);

// Expose to QML
engine.rootContext()->setContextProperty("diagnostics", diagnostics);
```

### MainScrollBench.qml
```qml
DiagnosticsOverlay {
    id: diagnosticsOverlay
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.margins: 10
    width: expanded ? 450 : 300
    height: expanded ? 500 : 45
    z: 300
}
```

## What It Detects

### Critical Issues (Red Alert)
- ❌ Viewport range is ZERO items
- ❌ Load stalled for > 5 seconds with pending requests
- ❌ Settings mismatch (disk cache not synchronized)

### Warnings (Orange)
- ⚠️ Viewport range < 5 items (very small)
- ⚠️ Buffered range < 50 items (possible "19 items bug")
- ⚠️ Task queue > 1000 items (high load)

### Good Status (Green)
- ✓ Viewport culling working correctly
- ✓ Settings synchronized
- ✓ Normal I/O load
- ✓ All systems operational

## Console Output

The diagnostics system adds these log messages:

```
[Diagnostics] Monitor started - running checks every 1000ms
[Diagnostics] Attached to ScrollBenchImageModel
[Diagnostics] Attached to SettingsHelper
[Diagnostics] WARNING: Buffered range only 19 items - might be the '19 items' bug
[Diagnostics] CRITICAL: Viewport range is 0 - no items detected in viewport!
```

## UI Display Example

**Collapsed View:**
```
🟢 ✓ All systems operational  ▶
```

**Expanded View:**
```
🟢 ✓ All systems operational  ▼
────────────────────────────────────
🟢 Viewport Culling:  ✓ Culling ON: viewport 50 items, buffered 150 items  
🟦 Load Progress:     245/1000 loaded (24.5%) | Pending: 12 | Staged: 3
🟢 Settings Sync:     ✓ Disk cache: ON (synchronized)
🟢 Adaptive I/O:      Active: 45 tasks

[Toggle Culling] [Toggle Cache]
```

**With Critical Issue:**
```
🔴 ❌ CRITICAL (1 issues)  ▼
────────────────────────────────────
🔴 Viewport Culling:  ⚠️ Small buffered range: 19 items (expected >50)
🟦 Load Progress:     245/1000 loaded (24.5%) | Pending: 12 | Staged: 3
🟢 Settings Sync:     ✓ Disk cache: ON (synchronized)
🟢 Adaptive I/O:      Active: 45 tasks

❌ CRITICAL Issues:
  • Buffered range only 19 items - might be the '19 items' bug
  
[Popup Alert showing: "Buffered range only 19 items..."]
```

## Build Integration

Added to `CMakeLists.txt`:
- Source files: `src/DiagnosticsMonitor.cpp` and `.h`
- QML files: `qml/DiagnosticsOverlay.qml` and `qml/DiagnosticRow.qml`

## Usage

### No Manual Action Required
The diagnostics system starts automatically when the app launches and runs continuously every second.

### Click to Expand
Click the arrow icon to expand/collapse the overlay for detailed information.

### Quick Actions
When expanded, use the buttons to toggle settings and immediately see the results.

### Critical Alerts
When a critical issue is detected, a popup will appear alerting you to the problem.

## Testing the System

### Test 1: Verify It's Running
1. Launch the app
2. Look for the diagnostics overlay in the top-left corner
3. Should show green indicator with "✓ All systems operational"

### Test 2: Trigger "19 Items Bug"
1. If the old code is still active, scroll and observe
2. Diagnostics should detect: "⚠️ Small buffered range: 19 items"
3. Status should turn orange
4. Warning should appear in expanded view

### Test 3: Settings Mismatch
1. Manually toggle disk cache in Performance Overlay
2. Diagnostics should detect if AsyncImageProvider didn't update
3. Should show: "❌ MISMATCH: Settings=ON, Provider=OFF"
4. Critical popup should appear

### Test 4: Load Stall
1. If loads get stuck with pending requests
2. After 5 seconds, diagnostics should alert
3. Should show: "Load stalled for X seconds with Y pending"

## Benefits

✅ **No More Guessing**: Always know what's working and what's broken  
✅ **Early Detection**: Catch issues before they become user-visible problems  
✅ **Real-time Validation**: See the effect of toggles immediately  
✅ **Automatic**: Runs continuously without manual intervention  
✅ **Visual Feedback**: Color-coded, easy-to-understand status  
✅ **Actionable**: Shows exactly what's wrong and where  

## Next Steps to Complete

1. **Build the Application** with the new diagnostics system
2. **Add the DiagnosticsOverlay** to MainScrollBench.qml (manual step needed)
3. **Test the System** with real data
4. **Tune Thresholds** based on observed behavior
5. **Extend Checks** to cover more edge cases as they're discovered

The system is designed to be extensible - you can easily add more checks to `DiagnosticsMonitor::runDiagnostics()` as new issues are identified.
