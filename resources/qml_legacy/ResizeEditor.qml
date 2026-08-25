import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QGalleryX 1.0

Dialog {
    id: resizeEditor
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent ? parent.width * 0.95 : 1200, 1260)
    height: Math.min(parent ? parent.height * 0.94 : 840, 880)
    padding: 0
    closePolicy: Popup.CloseOnEscape

    // Input target paths
    property var targetPaths: []
    property int batchCount: targetPaths.length

    // State & Target Settings
    property int resizeMode: 0 // 0: Dimensions/Scale, 1: Target File Size (e.g. 20MB limit)
    property int targetWidth: 1920
    property int targetHeight: 1080
    property bool lockAspectRatio: true
    property int quality: 85
    property int compression: 5
    property string activePreset: "1080p"
    property real scalePercent: 100

    // Target Total MB Mode (e.g. Email 20MB)
    property real targetTotalMB: 20.0

    // DPI Settings & Physical Resampling
    property int targetDpi: 72
    property int origDpi: 72
    property bool resampleDpi: false // If true, changing DPI scales pixel dimensions & file size proportionally

    // Destination & Naming
    property int destinationMode: 0 // 0: Subfolder (/Resized/), 1: Custom Folder, 2: Overwrite
    property string customDestinationPath: ""
    property string filenameSuffix: "_resized"

    // Metadata for First Selected Image (Live Preview)
    property string currentImagePath: ""
    property int origW: 1920
    property int origH: 1080
    property real origAspect: (origH > 0) ? (origW / origH) : (16 / 9)
    property real singleOrigSizeBytes: 0
    property real totalOrigSizeBytes: 0
    property real singleNewSizeBytes: 0
    property real totalNewSizeBytes: 0

    // Preview File URL
    property string livePreviewUrl: ""

    // Synchronized Pan & Ultra-Zoom State for Artifact Scrutiny (Down to Individual Pixel Level)
    property real syncZoom: 1.0 // 1.0 (Fit) to 64.0 (6400% ultra-macro pixel peep)
    property real syncPanX: 0.0 // Offset in pixels
    property real syncPanY: 0.0

    onOpened: {
        initializeData()
    }

    function initializeData() {
        if (!targetPaths || targetPaths.length === 0) return;
        var first = targetPaths[0].replace("file:///", "").replace("file:", "");
        currentImagePath = first;

        var dims = desktopHelper.getImageDimensions(first);
        if (dims.width > 0 && dims.height > 0) {
            origW = dims.width;
            origH = dims.height;
            origAspect = origW / origH;
        }
        if (dims.dpi && dims.dpi > 0) {
            origDpi = dims.dpi;
            targetDpi = dims.dpi;
        } else {
            origDpi = 72;
            targetDpi = 72;
        }

        singleOrigSizeBytes = desktopHelper.getFileSize(first);
        totalOrigSizeBytes = desktopHelper.getTotalSize(targetPaths);

        // Reset Pan & Zoom
        resetPanZoom();

        // Default to Full HD or Original
        if (origW > 1920 || origH > 1080) {
            setDimensionPreset("1080p");
        } else {
            targetWidth = origW;
            targetHeight = origH;
            activePreset = "Original";
            scalePercent = 100;
        }

        updateLivePreview();
    }

    function resetPanZoom() {
        syncZoom = 1.0;
        syncPanX = 0.0;
        syncPanY = 0.0;
    }

    function setScalePercent(pct) {
        scalePercent = pct;
        activePreset = pct + "%";
        targetWidth = Math.max(50, Math.round(origW * (pct / 100.0)));
        targetHeight = Math.max(50, Math.round(origH * (pct / 100.0)));
        updateLivePreview();
    }

    function setDimensionPreset(preset) {
        activePreset = preset;
        if (preset === "Original") {
            setScalePercent(100);
            return;
        }

        var maxW = 1920;
        var maxH = 1080;
        if (preset === "4K") { maxW = 3840; maxH = 2160; }
        else if (preset === "1080p") { maxW = 1920; maxH = 1080; }
        else if (preset === "720p") { maxW = 1280; maxH = 720; }
        else if (preset === "Web") { maxW = 1024; maxH = 768; }
        else if (preset === "Thumb") { maxW = 640; maxH = 480; }

        if (origAspect >= (maxW / maxH)) {
            targetWidth = Math.min(origW, maxW);
            targetHeight = Math.round(targetWidth / origAspect);
        } else {
            targetHeight = Math.min(origH, maxH);
            targetWidth = Math.round(targetHeight * origAspect);
        }
        scalePercent = Math.round((targetWidth / origW) * 100);
        updateLivePreview();
    }

    function applyDpi(newDpi) {
        targetDpi = newDpi;
        if (resampleDpi && origDpi > 0) {
            var ratio = targetDpi / origDpi;
            targetWidth = Math.max(50, Math.round(origW * ratio));
            targetHeight = Math.max(50, Math.round(origH * ratio));
            scalePercent = Math.round(ratio * 100);
            activePreset = targetDpi + " DPI";
        }
        updateLivePreview();
    }

    function applyTargetTotalMB(mbQuota) {
        targetTotalMB = mbQuota;
        if (totalOrigSizeBytes <= 0 || targetPaths.length === 0) return;

        var quotaBytes = mbQuota * 1024 * 1024;
        
        // Scenario A: Batch is ALREADY under the target quota (No compression needed!)
        if (totalOrigSizeBytes <= quotaBytes) {
            targetWidth = origW;
            targetHeight = origH;
            scalePercent = 100;
            quality = 90; // High quality preservation
            activePreset = "Within Budget (" + mbQuota + " MB)";
            updateLivePreview();
            return;
        }

        // Scenario B: Compression/Resampling is required to fit into quota
        var ratio = quotaBytes / totalOrigSizeBytes; // e.g. 0.35 = need to shrink to 35% of orig

        // Tier 1: Mild compression (>= 65% of original needed) -> Zero resolution loss, adjust JPEG entropy
        if (ratio >= 0.65) {
            targetWidth = origW;
            targetHeight = origH;
            scalePercent = 100;
            // JPEG quality between 72% and 85% saves 35-50% size with no loss of pixel dimensions
            quality = Math.max(70, Math.min(85, Math.round(68 + ratio * 20)));
        }
        // Tier 2: Moderate compression (35% to 65% of original) -> High resolution (80-92% scale) + balanced quality
        else if (ratio >= 0.35) {
            var scaleRatio = Math.min(0.92, Math.max(0.60, Math.sqrt(ratio / 0.70)));
            targetWidth = Math.max(320, Math.round(origW * scaleRatio));
            targetHeight = Math.max(240, Math.round(origH * scaleRatio));
            scalePercent = Math.round(scaleRatio * 100);
            quality = Math.max(72, Math.min(82, Math.round(72 + (ratio - 0.35) * 30)));
        }
        // Tier 3: Aggressive reduction (< 35% of original) -> Bicubic downsampling + crisp quality to avoid block artifacts
        else {
            var scaleRatio = Math.min(0.60, Math.max(0.15, Math.sqrt(ratio / 0.60)));
            targetWidth = Math.max(320, Math.round(origW * scaleRatio));
            targetHeight = Math.max(240, Math.round(origH * scaleRatio));
            scalePercent = Math.round(scaleRatio * 100);
            // Keeping quality >= 68 preserves edges far better than crushing quality to 20%
            quality = Math.max(68, Math.min(76, Math.round(65 + ratio * 30)));
        }

        activePreset = mbQuota + " MB Limit";
        updateLivePreview();
    }

    function updateLivePreview() {
        if (!currentImagePath) return;
        previewTimer.restart();
    }

    Timer {
        id: previewTimer
        interval: 60
        repeat: false
        onTriggered: {
            var res = desktopHelper.generateResizePreview(
                resizeEditor.currentImagePath,
                resizeEditor.targetWidth,
                resizeEditor.targetHeight,
                resizeEditor.quality,
                resizeEditor.compression,
                resizeEditor.targetDpi
            );

            if (res && res.path) {
                resizeEditor.livePreviewUrl = res.path + "?t=" + Date.now();
                resizeEditor.singleNewSizeBytes = res.size || 0;

                // Accurate Total Batch Estimation based on real dimensions of all batch images
                resizeEditor.totalNewSizeBytes = desktopHelper.estimateBatchSize(
                    resizeEditor.targetPaths,
                    resizeEditor.targetWidth,
                    resizeEditor.targetHeight,
                    resizeEditor.quality,
                    resizeEditor.compression,
                    res.size || 0,
                    resizeEditor.origW,
                    resizeEditor.origH
                );
            }
        }
    }

    // Destination Directory Solver
    function getResolvedDestination() {
        if (destinationMode === 1 && customDestinationPath) {
            return customDestinationPath;
        }
        if (!currentImagePath) return Qt.platform.homePath + "/Desktop";
        var baseDir = currentImagePath.substring(0, Math.max(currentImagePath.lastIndexOf("/"), currentImagePath.lastIndexOf("\\")));
        if (destinationMode === 2) {
            return baseDir; // Overwrite in same folder
        }
        // Subfolder /Resized/
        return baseDir + "/Resized";
    }

    FolderDialog {
        id: destinationFolderPicker
        title: "Select Destination Folder for Resized Images"
        currentFolder: "file:///" + getResolvedDestination().replace(/\\/g, "/")
        onAccepted: {
            var f = selectedFolder.toString().replace("file:///", "").replace("file:", "");
            resizeEditor.customDestinationPath = f;
            resizeEditor.destinationMode = 1;
        }
    }

    function performBatchResize() {
        if (!targetPaths || targetPaths.length === 0) return;

        var destDir = getResolvedDestination();
        var suffix = (destinationMode === 2) ? "" : filenameSuffix;

        var savedCount = desktopHelper.exportImages(
            targetPaths,
            destDir,
            targetWidth,
            targetHeight,
            quality,
            compression,
            suffix,
            resizeEditor.targetDpi
        );

        if (typeof toastOverlay !== "undefined") {
            toastOverlay.showToast("✓ Resized " + savedCount + " images to: " + destDir.split(/[\/\\]/).pop());
        }

        resizeEditor.accept();
    }

    background: Rectangle {
        color: "#181A20"
        radius: 12
        border.color: "#353842"
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 0

        // --- 1. MODAL HEADER ---
        Rectangle {
            Layout.fillWidth: true
            height: 56
            color: "#1F222A"
            radius: 12
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#30ffffff"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 12

                Text {
                    text: "📐 Batch Image Resizer & Compressor"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                }

                Rectangle {
                    radius: 12
                    color: "#2A3B5C"
                    border.color: "#3B82F6"
                    border.width: 1
                    implicitWidth: countText.implicitWidth + 18
                    implicitHeight: 24
                    Text {
                        id: countText
                        anchors.centerIn: parent
                        text: resizeEditor.batchCount + (resizeEditor.batchCount === 1 ? " image" : " images")
                        color: "#93C5FD"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "Original Total: " + (resizeEditor.totalOrigSizeBytes / (1024 * 1024)).toFixed(1) + " MB"
                    color: "#9CA3AF"
                    font.pixelSize: 12
                }

                Button {
                    text: "✕"
                    implicitWidth: 32
                    implicitHeight: 32
                    background: Rectangle { color: parent.hovered ? "#353842" : "transparent"; radius: 16 }
                    contentItem: Text { text: "✕"; color: "#9CA3AF"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: resizeEditor.reject()
                }
            }
        }

        // --- 2. MAIN BODY (SPLIT VIEW: PREVIEW & CONTROLS) ---
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 16
            spacing: 16

            // === LEFT: BEFORE & AFTER COMPARISON VIEW WITH SYNCHRONIZED PIXEL-LEVEL PAN & ZOOM ===
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 55
                color: "#131418"
                radius: 10
                border.color: "#282B34"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    // Zoom & Pan Toolbar for Scrutinizing Artifacts down to Individual Pixels
                    Rectangle {
                        Layout.fillWidth: true
                        height: 38
                        color: "#1A1D24"
                        radius: 6
                        border.color: "#2E333F"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 5

                            Text {
                                text: "🔍 Zoom: " + Math.round(resizeEditor.syncZoom * 100) + "%"
                                color: "#93C5FD"
                                font.pixelSize: 11
                                font.bold: true
                                Layout.preferredWidth: 80
                            }

                            // Zoom In/Out Steppers
                            Button {
                                text: "−"
                                implicitHeight: 24
                                implicitWidth: 26
                                background: Rectangle { color: parent.hovered ? "#374151" : "#22252E"; radius: 4; border.color: "#374151"; border.width: 1 }
                                contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                onClicked: {
                                    resizeEditor.syncZoom = Math.max(1.0, resizeEditor.syncZoom / 1.5);
                                    if (resizeEditor.syncZoom === 1.0) {
                                        resizeEditor.syncPanX = 0;
                                        resizeEditor.syncPanY = 0;
                                    }
                                }
                            }

                            Button {
                                text: "+"
                                implicitHeight: 24
                                implicitWidth: 26
                                background: Rectangle { color: parent.hovered ? "#374151" : "#22252E"; radius: 4; border.color: "#374151"; border.width: 1 }
                                contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                onClicked: resizeEditor.syncZoom = Math.min(64.0, resizeEditor.syncZoom * 1.5)
                            }

                            // Quick Zoom Chips (Fit, 100%, 200%, 400%, 800%, 1600%, 3200% Pixel Level)
                            Repeater {
                                model: [
                                    {label: "Fit", val: 1.0},
                                    {label: "100%", val: 2.0},
                                    {label: "400%", val: 5.0},
                                    {label: "800%", val: 9.0},
                                    {label: "1600%", val: 17.0},
                                    {label: "3200% (Pixel)", val: 33.0}
                                ]
                                delegate: Button {
                                    text: modelData.label
                                    implicitHeight: 24
                                    Layout.fillWidth: true
                                    font.pixelSize: 10
                                    font.bold: Math.abs(resizeEditor.syncZoom - modelData.val) < 0.2
                                    background: Rectangle {
                                        color: Math.abs(resizeEditor.syncZoom - modelData.val) < 0.2 ? "#2563EB" : (parent.hovered ? "#2D313D" : "#22252E")
                                        radius: 4
                                        border.color: Math.abs(resizeEditor.syncZoom - modelData.val) < 0.2 ? "#60A5FA" : "#374151"
                                        border.width: 1
                                    }
                                    contentItem: Text { text: parent.text; color: "white"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                    onClicked: {
                                        resizeEditor.syncZoom = modelData.val;
                                        if (modelData.val === 1.0) {
                                            resizeEditor.syncPanX = 0;
                                            resizeEditor.syncPanY = 0;
                                        }
                                    }
                                }
                            }

                            Button {
                                text: "⟲ Reset"
                                implicitHeight: 24
                                implicitWidth: 56
                                font.pixelSize: 10
                                background: Rectangle { color: parent.hovered ? "#374151" : "#22252E"; radius: 4; border.color: "#4B5563"; border.width: 1 }
                                contentItem: Text { text: parent.text; color: "#D1D5DB"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                onClicked: resizeEditor.resetPanZoom()
                            }
                        }
                    }

                    // Before / After Synchronized Dual-Card View
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 10

                        // 1. Before (Original Card)
                        Rectangle {
                            id: origCard
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#0D0E12"
                            radius: 8
                            border.color: "#22252E"
                            border.width: 1
                            clip: true

                            Item {
                                id: origViewport
                                anchors.fill: parent
                                anchors.bottomMargin: 32
                                clip: true

                                Item {
                                    id: origTransformContainer
                                    anchors.fill: parent
                                    transform: [
                                        Translate {
                                            x: resizeEditor.syncPanX
                                            y: resizeEditor.syncPanY
                                        },
                                        Scale {
                                            xScale: resizeEditor.syncZoom
                                            yScale: resizeEditor.syncZoom
                                            origin.x: origViewport.width / 2
                                            origin.y: origViewport.height / 2
                                        }
                                    ]

                                    Image {
                                        id: origImg
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        source: resizeEditor.currentImagePath ? ("image://async/" + resizeEditor.currentImagePath) : ""
                                        fillMode: Image.PreserveAspectFit
                                        asynchronous: true
                                        smooth: resizeEditor.syncZoom < 2.0
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: (resizeEditor.syncZoom > 1.0) ? (pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor) : Qt.ArrowCursor
                                    property real lastX: 0
                                    property real lastY: 0
                                    onPressed: (mouse) => {
                                        lastX = mouse.x;
                                        lastY = mouse.y;
                                    }
                                    onPositionChanged: (mouse) => {
                                        if (pressed) {
                                            var dx = mouse.x - lastX;
                                            var dy = mouse.y - lastY;
                                            resizeEditor.syncPanX += dx;
                                            resizeEditor.syncPanY += dy;
                                            lastX = mouse.x;
                                            lastY = mouse.y;
                                        }
                                    }
                                    onWheel: (wheel) => {
                                        if (wheel.angleDelta.y > 0) {
                                            resizeEditor.syncZoom = Math.min(64.0, resizeEditor.syncZoom * 1.25);
                                        } else {
                                            resizeEditor.syncZoom = Math.max(1.0, resizeEditor.syncZoom / 1.25);
                                            if (resizeEditor.syncZoom === 1.0) {
                                                resizeEditor.syncPanX = 0;
                                                resizeEditor.syncPanY = 0;
                                            }
                                        }
                                        wheel.accepted = true;
                                    }
                                }
                            }

                            // Original Badge Banner
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 32
                                color: "#D0111317"
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    Text { text: "ORIGINAL"; color: "#9CA3AF"; font.pixelSize: 10; font.bold: true }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: resizeEditor.origW + "×" + resizeEditor.origH + " (" + resizeEditor.origDpi + " DPI, " + (resizeEditor.singleOrigSizeBytes / (1024 * 1024)).toFixed(2) + " MB)"
                                        color: "#E5E7EB"
                                        font.pixelSize: 11
                                        font.bold: true
                                        elide: Text.ElideMiddle
                                    }
                                }
                            }
                        }

                        // 2. After (Resized Live Preview Card)
                        Rectangle {
                            id: previewCard
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#0D0E12"
                            radius: 8
                            border.color: "#1E3A8A"
                            border.width: 1.5
                            clip: true

                            Item {
                                id: previewViewport
                                anchors.fill: parent
                                anchors.bottomMargin: 32
                                clip: true

                                Item {
                                    id: previewTransformContainer
                                    anchors.fill: parent
                                    transform: [
                                        Translate {
                                            x: resizeEditor.syncPanX
                                            y: resizeEditor.syncPanY
                                        },
                                        Scale {
                                            xScale: resizeEditor.syncZoom
                                            yScale: resizeEditor.syncZoom
                                            origin.x: previewViewport.width / 2
                                            origin.y: previewViewport.height / 2
                                        }
                                    ]

                                    Image {
                                        id: previewImgItem
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        source: resizeEditor.livePreviewUrl
                                        fillMode: Image.PreserveAspectFit
                                        asynchronous: true
                                        smooth: resizeEditor.syncZoom < 2.0
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: (resizeEditor.syncZoom > 1.0) ? (pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor) : Qt.ArrowCursor
                                    property real lastX: 0
                                    property real lastY: 0
                                    onPressed: (mouse) => {
                                        lastX = mouse.x;
                                        lastY = mouse.y;
                                    }
                                    onPositionChanged: (mouse) => {
                                        if (pressed) {
                                            var dx = mouse.x - lastX;
                                            var dy = mouse.y - lastY;
                                            resizeEditor.syncPanX += dx;
                                            resizeEditor.syncPanY += dy;
                                            lastX = mouse.x;
                                            lastY = mouse.y;
                                        }
                                    }
                                    onWheel: (wheel) => {
                                        if (wheel.angleDelta.y > 0) {
                                            resizeEditor.syncZoom = Math.min(64.0, resizeEditor.syncZoom * 1.25);
                                        } else {
                                            resizeEditor.syncZoom = Math.max(1.0, resizeEditor.syncZoom / 1.25);
                                            if (resizeEditor.syncZoom === 1.0) {
                                                resizeEditor.syncPanX = 0;
                                                resizeEditor.syncPanY = 0;
                                            }
                                        }
                                        wheel.accepted = true;
                                    }
                                }
                            }

                            // Preview Badge Banner
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 32
                                color: "#D0111317"
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    Text { text: "RESIZED PREVIEW"; color: "#60A5FA"; font.pixelSize: 10; font.bold: true }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: resizeEditor.targetWidth + "×" + resizeEditor.targetHeight + " (" + resizeEditor.targetDpi + " DPI, " +
                                              (resizeEditor.singleNewSizeBytes > 1024 * 1024 ?
                                               (resizeEditor.singleNewSizeBytes / (1024 * 1024)).toFixed(2) + " MB" :
                                               Math.round(resizeEditor.singleNewSizeBytes / 1024) + " KB") + ")"
                                        color: "#34D399"
                                        font.pixelSize: 11
                                        font.bold: true
                                        elide: Text.ElideMiddle
                                    }
                                }
                            }
                        }
                    }

                    // Bottom Summary Banner
                    Rectangle {
                        Layout.fillWidth: true
                        height: 52
                        radius: 8
                        color: "#182030"
                        border.color: "#2563EB"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 16

                            ColumnLayout {
                                spacing: 1
                                Text { text: "ESTIMATED TOTAL"; color: "#93C5FD"; font.pixelSize: 10; font.bold: true }
                                Text {
                                    text: (resizeEditor.totalNewSizeBytes / (1024 * 1024)).toFixed(1) + " MB"
                                    color: "#60A5FA"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                            }

                            Rectangle { width: 1; height: 30; color: "#30ffffff" }

                            ColumnLayout {
                                spacing: 1
                                Text { text: "AVG PER IMAGE"; color: "#9CA3AF"; font.pixelSize: 10 }
                                Text {
                                    property real avgBytes: resizeEditor.batchCount > 0 ? (resizeEditor.totalNewSizeBytes / resizeEditor.batchCount) : 0
                                    text: avgBytes > 1024 * 1024 ? (avgBytes / (1024 * 1024)).toFixed(2) + " MB" : Math.round(avgBytes / 1024) + " KB"
                                    color: "#E5E7EB"
                                    font.pixelSize: 13
                                    font.bold: true
                                }
                            }

                            Rectangle { width: 1; height: 30; color: "#30ffffff" }

                            ColumnLayout {
                                spacing: 1
                                Text { text: "SPACE SAVED"; color: "#34D399"; font.pixelSize: 10; font.bold: true }
                                Text {
                                    property real pct: resizeEditor.totalOrigSizeBytes > 0 ?
                                                       (1.0 - (resizeEditor.totalNewSizeBytes / resizeEditor.totalOrigSizeBytes)) * 100.0 : 0
                                    text: (pct >= 0 ? "-" : "+") + Math.abs(Math.round(pct)) + "%"
                                    color: pct >= 0 ? "#34D399" : "#F87171"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }
                }
            }

            // === RIGHT: CONTROLS & CLEAN CONSTRAINTS ===
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 45
                color: "#181A20"

                ScrollView {
                    anchors.fill: parent
                    contentWidth: parent.width
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        width: parent.width
                        spacing: 12

                        // 1. Dimension & Resolution Section
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: dimCol.implicitHeight + 20
                            radius: 8
                            color: "#1F222A"
                            border.color: "#2C303B"
                            border.width: 1

                            ColumnLayout {
                                id: dimCol
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "📐 Resolution & Scaling"; color: "#E5E7EB"; font.pixelSize: 12; font.bold: true }
                                    Item { Layout.fillWidth: true }
                                    Text { text: resizeEditor.scalePercent + "% Scale"; color: "#60A5FA"; font.pixelSize: 11; font.bold: true }
                                }

                                // Quick Scale Chips
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    Repeater {
                                        model: [
                                            {label: "25%", val: 25},
                                            {label: "50%", val: 50},
                                            {label: "75%", val: 75},
                                            {label: "100% (Original)", val: 100}
                                        ]
                                        delegate: Button {
                                            text: modelData.label
                                            Layout.fillWidth: true
                                            font.pixelSize: 11
                                            font.bold: resizeEditor.scalePercent === modelData.val
                                            background: Rectangle {
                                                color: resizeEditor.scalePercent === modelData.val ? "#2563EB" : "#282B34"
                                                radius: 4
                                                border.color: resizeEditor.scalePercent === modelData.val ? "#60A5FA" : "#374151"
                                                border.width: 1
                                            }
                                            contentItem: Text { text: parent.text; color: "white"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                            onClicked: resizeEditor.setScalePercent(modelData.val)
                                        }
                                    }
                                }

                                // Preset Dimension Chips
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    Repeater {
                                        model: ["4K", "1080p", "720p", "Web", "Thumb"]
                                        delegate: Button {
                                            text: modelData
                                            Layout.fillWidth: true
                                            font.pixelSize: 11
                                            font.bold: resizeEditor.activePreset === modelData
                                            background: Rectangle {
                                                color: resizeEditor.activePreset === modelData ? "#1E3A8A" : "#282B34"
                                                radius: 4
                                                border.color: resizeEditor.activePreset === modelData ? "#3B82F6" : "#374151"
                                                border.width: 1
                                            }
                                            contentItem: Text { text: parent.text; color: "white"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                            onClicked: resizeEditor.setDimensionPreset(modelData)
                                        }
                                    }
                                }

                                // Custom Width & Height SpinBoxes
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Text { text: "Width (px)"; color: "#9CA3AF"; font.pixelSize: 10 }
                                        SpinBox {
                                            id: widthSpin
                                            Layout.fillWidth: true
                                            from: 50; to: 16000
                                            value: resizeEditor.targetWidth
                                            editable: true
                                            contentItem: TextInput {
                                                text: widthSpin.textFromValue(widthSpin.value, widthSpin.locale)
                                                font.pixelSize: 12
                                                color: "white"
                                                selectionColor: "#2563EB"
                                                selectedTextColor: "white"
                                                horizontalAlignment: Qt.AlignHCenter
                                                verticalAlignment: Qt.AlignVCenter
                                                readOnly: !widthSpin.editable
                                                validator: widthSpin.validator
                                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                            }
                                            background: Rectangle {
                                                color: "#22252E"
                                                border.color: widthSpin.activeFocus ? "#60A5FA" : "#374151"
                                                radius: 4
                                            }
                                            up.indicator: Rectangle {
                                                x: widthSpin.mirrored ? 0 : widthSpin.width - width
                                                height: widthSpin.height
                                                implicitWidth: 28
                                                color: widthSpin.up.pressed ? "#1E293B" : (widthSpin.up.hovered ? "#334155" : "#282B34")
                                                border.color: "#374151"
                                                radius: 4
                                                Text { text: "+"; font.pixelSize: 13; color: "white"; anchors.centerIn: parent }
                                            }
                                            down.indicator: Rectangle {
                                                x: widthSpin.mirrored ? widthSpin.width - width : 0
                                                height: widthSpin.height
                                                implicitWidth: 28
                                                color: widthSpin.down.pressed ? "#1E293B" : (widthSpin.down.hovered ? "#334155" : "#282B34")
                                                border.color: "#374151"
                                                radius: 4
                                                Text { text: "−"; font.pixelSize: 13; color: "white"; anchors.centerIn: parent }
                                            }
                                            onValueChanged: {
                                                if (value !== resizeEditor.targetWidth) {
                                                    resizeEditor.targetWidth = value;
                                                    if (resizeEditor.lockAspectRatio && resizeEditor.origAspect > 0) {
                                                        resizeEditor.targetHeight = Math.max(50, Math.round(value / resizeEditor.origAspect));
                                                    }
                                                    resizeEditor.scalePercent = Math.round((value / resizeEditor.origW) * 100);
                                                    resizeEditor.activePreset = "Custom";
                                                    resizeEditor.updateLivePreview();
                                                }
                                            }
                                        }
                                    }

                                    Button {
                                        text: resizeEditor.lockAspectRatio ? "🔗" : "🔓"
                                        Layout.alignment: Qt.AlignBottom
                                        Layout.preferredWidth: 32
                                        Layout.preferredHeight: 38
                                        ToolTip.visible: hovered
                                        ToolTip.text: resizeEditor.lockAspectRatio ? "Aspect Ratio Locked" : "Aspect Ratio Free"
                                        background: Rectangle { color: resizeEditor.lockAspectRatio ? "#1E3A8A" : "#282B34"; radius: 4 }
                                        contentItem: Text { text: parent.text; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        onClicked: resizeEditor.lockAspectRatio = !resizeEditor.lockAspectRatio
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Text { text: "Height (px)"; color: "#9CA3AF"; font.pixelSize: 10 }
                                        SpinBox {
                                            id: heightSpin
                                            Layout.fillWidth: true
                                            from: 50; to: 16000
                                            value: resizeEditor.targetHeight
                                            editable: true
                                            contentItem: TextInput {
                                                text: heightSpin.textFromValue(heightSpin.value, heightSpin.locale)
                                                font.pixelSize: 12
                                                color: "white"
                                                selectionColor: "#2563EB"
                                                selectedTextColor: "white"
                                                horizontalAlignment: Qt.AlignHCenter
                                                verticalAlignment: Qt.AlignVCenter
                                                readOnly: !heightSpin.editable
                                                validator: heightSpin.validator
                                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                            }
                                            background: Rectangle {
                                                color: "#22252E"
                                                border.color: heightSpin.activeFocus ? "#60A5FA" : "#374151"
                                                radius: 4
                                            }
                                            up.indicator: Rectangle {
                                                x: heightSpin.mirrored ? 0 : heightSpin.width - width
                                                height: heightSpin.height
                                                implicitWidth: 28
                                                color: heightSpin.up.pressed ? "#1E293B" : (heightSpin.up.hovered ? "#334155" : "#282B34")
                                                border.color: "#374151"
                                                radius: 4
                                                Text { text: "+"; font.pixelSize: 13; color: "white"; anchors.centerIn: parent }
                                            }
                                            down.indicator: Rectangle {
                                                x: heightSpin.mirrored ? heightSpin.width - width : 0
                                                height: heightSpin.height
                                                implicitWidth: 28
                                                color: heightSpin.down.pressed ? "#1E293B" : (heightSpin.down.hovered ? "#334155" : "#282B34")
                                                border.color: "#374151"
                                                radius: 4
                                                Text { text: "−"; font.pixelSize: 13; color: "white"; anchors.centerIn: parent }
                                            }
                                            onValueChanged: {
                                                if (value !== resizeEditor.targetHeight) {
                                                    resizeEditor.targetHeight = value;
                                                    if (resizeEditor.lockAspectRatio && resizeEditor.origAspect > 0) {
                                                        resizeEditor.targetWidth = Math.max(50, Math.round(value * resizeEditor.origAspect));
                                                    }
                                                    resizeEditor.scalePercent = Math.round((value / resizeEditor.origH) * 100);
                                                    resizeEditor.activePreset = "Custom";
                                                    resizeEditor.updateLivePreview();
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // 2. Target File Size Limit Section (e.g. Fit into 20MB for Email / Custom Limit)
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: targetCol.implicitHeight + 20
                            radius: 8
                            color: "#1F222A"
                            border.color: "#2C303B"
                            border.width: 1

                            ColumnLayout {
                                id: targetCol
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "🎯 Target Total Batch Limit (Email / Share)"; color: "#E5E7EB"; font.pixelSize: 12; font.bold: true }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: resizeEditor.targetTotalMB + " MB Limit"
                                        color: "#34D399"
                                        font.pixelSize: 11
                                        font.bold: true
                                    }
                                }

                                // Quick Limit Presets
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    Repeater {
                                        model: [
                                            {label: "10 MB (Email)", val: 10},
                                            {label: "20 MB (Gmail)", val: 20},
                                            {label: "25 MB (Discord)", val: 25},
                                            {label: "50 MB", val: 50},
                                            {label: "100 MB", val: 100}
                                        ]
                                        delegate: Button {
                                            text: modelData.label
                                            Layout.fillWidth: true
                                            font.pixelSize: 10
                                            font.bold: Math.abs(resizeEditor.targetTotalMB - modelData.val) < 0.1
                                            background: Rectangle {
                                                color: Math.abs(resizeEditor.targetTotalMB - modelData.val) < 0.1 ? "#059669" : "#282B34"
                                                radius: 4
                                                border.color: Math.abs(resizeEditor.targetTotalMB - modelData.val) < 0.1 ? "#34D399" : "#374151"
                                                border.width: 1
                                            }
                                            contentItem: Text { text: parent.text; color: "white"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                            onClicked: resizeEditor.applyTargetTotalMB(modelData.val)
                                        }
                                    }
                                }

                                // Custom Target MB Input
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Text { text: "Custom Limit (MB):"; color: "#9CA3AF"; font.pixelSize: 11 }
                                    SpinBox {
                                        id: customMbSpin
                                        Layout.fillWidth: true
                                        from: 1; to: 5000
                                        value: Math.round(resizeEditor.targetTotalMB)
                                        editable: true
                                        contentItem: TextInput {
                                            text: customMbSpin.textFromValue(customMbSpin.value, customMbSpin.locale)
                                            font.pixelSize: 12
                                            color: "white"
                                            selectionColor: "#2563EB"
                                            selectedTextColor: "white"
                                            horizontalAlignment: Qt.AlignHCenter
                                            verticalAlignment: Qt.AlignVCenter
                                            readOnly: !customMbSpin.editable
                                            validator: customMbSpin.validator
                                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                                        }
                                        background: Rectangle {
                                            color: "#22252E"
                                            border.color: customMbSpin.activeFocus ? "#34D399" : "#374151"
                                            radius: 4
                                        }
                                        up.indicator: Rectangle {
                                            x: customMbSpin.mirrored ? 0 : customMbSpin.width - width
                                            height: customMbSpin.height
                                            implicitWidth: 28
                                            color: customMbSpin.up.pressed ? "#1E293B" : (customMbSpin.up.hovered ? "#334155" : "#282B34")
                                            border.color: "#374151"
                                            radius: 4
                                            Text { text: "+"; font.pixelSize: 13; color: "white"; anchors.centerIn: parent }
                                        }
                                        down.indicator: Rectangle {
                                            x: customMbSpin.mirrored ? customMbSpin.width - width : 0
                                            height: customMbSpin.height
                                            implicitWidth: 28
                                            color: customMbSpin.down.pressed ? "#1E293B" : (customMbSpin.down.hovered ? "#334155" : "#282B34")
                                            border.color: "#374151"
                                            radius: 4
                                            Text { text: "−"; font.pixelSize: 13; color: "white"; anchors.centerIn: parent }
                                        }
                                        onValueChanged: {
                                            if (value !== resizeEditor.targetTotalMB) {
                                                resizeEditor.applyTargetTotalMB(value);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // 3. Clean Quality & Compression Slider
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: qualCol.implicitHeight + 20
                            radius: 8
                            color: "#1F222A"
                            border.color: "#2C303B"
                            border.width: 1

                            ColumnLayout {
                                id: qualCol
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "🎛 Image Quality & Compression"; color: "#E5E7EB"; font.pixelSize: 12; font.bold: true }
                                    Item { Layout.fillWidth: true }
                                    Rectangle {
                                        width: 54
                                        height: 22
                                        radius: 4
                                        color: "#2563EB"
                                        Text {
                                            anchors.centerIn: parent
                                            text: resizeEditor.quality === 101 ? "PNG" : (resizeEditor.quality + "%")
                                            color: "white"
                                            font.pixelSize: 11
                                            font.bold: true
                                        }
                                    }
                                }

                                Slider {
                                    id: qualSlider
                                    Layout.fillWidth: true
                                    from: 10; to: 100
                                    value: resizeEditor.quality <= 100 ? resizeEditor.quality : 90
                                    enabled: resizeEditor.quality !== 101
                                    onMoved: {
                                        resizeEditor.quality = Math.round(value);
                                        resizeEditor.updateLivePreview();
                                    }
                                }

                                // Clean Custom Styled Checkbox
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    CheckBox {
                                        id: losslessBox
                                        text: "Lossless Output (PNG)"
                                        checked: resizeEditor.quality === 101
                                        indicator: Rectangle {
                                            implicitWidth: 18
                                            implicitHeight: 18
                                            radius: 4
                                            color: losslessBox.checked ? "#2563EB" : "#282B34"
                                            border.color: losslessBox.checked ? "#60A5FA" : "#4B5563"
                                            border.width: 1
                                            Text {
                                                anchors.centerIn: parent
                                                text: "✓"
                                                color: "white"
                                                font.pixelSize: 11
                                                font.bold: true
                                                visible: losslessBox.checked
                                            }
                                        }
                                        contentItem: Text {
                                            text: losslessBox.text
                                            color: "#E5E7EB"
                                            leftPadding: losslessBox.indicator.width + 8
                                            verticalAlignment: Text.AlignVCenter
                                            font.pixelSize: 11
                                        }
                                        onCheckedChanged: {
                                            if (checked) resizeEditor.quality = 101;
                                            else resizeEditor.quality = Math.round(qualSlider.value);
                                            resizeEditor.updateLivePreview();
                                        }
                                    }
                                }
                            }
                        }

                        // 4. Target DPI & Print Resolution with Resampling Toggle
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: dpiCol.implicitHeight + 20
                            radius: 8
                            color: "#1F222A"
                            border.color: "#2C303B"
                            border.width: 1

                            ColumnLayout {
                                id: dpiCol
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "🖨 Target DPI & Print Resolution"; color: "#E5E7EB"; font.pixelSize: 12; font.bold: true }
                                    Item { Layout.fillWidth: true }
                                    Text { text: resizeEditor.targetDpi + " DPI"; color: "#60A5FA"; font.pixelSize: 11; font.bold: true }
                                }

                                // Resampling Option Checkbox
                                CheckBox {
                                    id: resampleCheck
                                    text: "Resample Pixels with DPI (Scales Dimensions & File Size)"
                                    checked: resizeEditor.resampleDpi
                                    indicator: Rectangle {
                                        implicitWidth: 18
                                        implicitHeight: 18
                                        radius: 4
                                        color: resampleCheck.checked ? "#059669" : "#282B34"
                                        border.color: resampleCheck.checked ? "#34D399" : "#4B5563"
                                        border.width: 1
                                        Text {
                                            anchors.centerIn: parent
                                            text: "✓"
                                            color: "white"
                                            font.pixelSize: 11
                                            font.bold: true
                                            visible: resampleCheck.checked
                                        }
                                    }
                                    contentItem: Text {
                                        text: resampleCheck.text
                                        color: resampleCheck.checked ? "#34D399" : "#9CA3AF"
                                        leftPadding: resampleCheck.indicator.width + 8
                                        verticalAlignment: Text.AlignVCenter
                                        font.pixelSize: 11
                                    }
                                    onCheckedChanged: {
                                        resizeEditor.resampleDpi = checked;
                                        resizeEditor.applyDpi(resizeEditor.targetDpi);
                                    }
                                }

                                // Quick DPI Presets
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    Repeater {
                                        model: [
                                            {label: "72 (Web)", val: 72},
                                            {label: "150 (Draft)", val: 150},
                                            {label: "300 (Print)", val: 300},
                                            {label: "600 (Scan)", val: 600}
                                        ]
                                        delegate: Button {
                                            text: modelData.label
                                            Layout.fillWidth: true
                                            font.pixelSize: 10
                                            font.bold: resizeEditor.targetDpi === modelData.val
                                            background: Rectangle {
                                                color: resizeEditor.targetDpi === modelData.val ? "#2563EB" : "#282B34"
                                                radius: 4
                                                border.color: resizeEditor.targetDpi === modelData.val ? "#60A5FA" : "#374151"
                                                border.width: 1
                                            }
                                            contentItem: Text { text: parent.text; color: "white"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                            onClicked: resizeEditor.applyDpi(modelData.val)
                                        }
                                    }
                                }

                                // Custom DPI SpinBox
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Text { text: "Custom DPI:"; color: "#9CA3AF"; font.pixelSize: 11 }
                                    SpinBox {
                                        id: dpiSpin
                                        Layout.fillWidth: true
                                        from: 10; to: 2400
                                        value: resizeEditor.targetDpi
                                        editable: true
                                        contentItem: TextInput {
                                            text: dpiSpin.textFromValue(dpiSpin.value, dpiSpin.locale)
                                            font.pixelSize: 12
                                            color: "white"
                                            selectionColor: "#2563EB"
                                            selectedTextColor: "white"
                                            horizontalAlignment: Qt.AlignHCenter
                                            verticalAlignment: Qt.AlignVCenter
                                            readOnly: !dpiSpin.editable
                                            validator: dpiSpin.validator
                                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                                        }
                                        background: Rectangle {
                                            color: "#22252E"
                                            border.color: dpiSpin.activeFocus ? "#60A5FA" : "#374151"
                                            radius: 4
                                        }
                                        up.indicator: Rectangle {
                                            x: dpiSpin.mirrored ? 0 : dpiSpin.width - width
                                            height: dpiSpin.height
                                            implicitWidth: 28
                                            color: dpiSpin.up.pressed ? "#1E293B" : (dpiSpin.up.hovered ? "#334155" : "#282B34")
                                            border.color: "#374151"
                                            radius: 4
                                            Text { text: "+"; font.pixelSize: 13; color: "white"; anchors.centerIn: parent }
                                        }
                                        down.indicator: Rectangle {
                                            x: dpiSpin.mirrored ? dpiSpin.width - width : 0
                                            height: dpiSpin.height
                                            implicitWidth: 28
                                            color: dpiSpin.down.pressed ? "#1E293B" : (dpiSpin.down.hovered ? "#334155" : "#282B34")
                                            border.color: "#374151"
                                            radius: 4
                                            Text { text: "−"; font.pixelSize: 13; color: "white"; anchors.centerIn: parent }
                                        }
                                        onValueChanged: {
                                            if (value !== resizeEditor.targetDpi) {
                                                resizeEditor.applyDpi(value);
                                            }
                                        }
                                    }
                                }

                                // Physical Output Print Dimensions Display
                                Text {
                                    property real printWInches: resizeEditor.targetDpi > 0 ? (resizeEditor.targetWidth / resizeEditor.targetDpi) : 0
                                    property real printHInches: resizeEditor.targetDpi > 0 ? (resizeEditor.targetHeight / resizeEditor.targetDpi) : 0
                                    text: "Physical Output: " + printWInches.toFixed(1) + "\" × " + printHInches.toFixed(1) + "\" (" +
                                          (printWInches * 2.54).toFixed(1) + " × " + (printHInches * 2.54).toFixed(1) + " cm)"
                                    color: "#9CA3AF"
                                    font.pixelSize: 10
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        // 5. Clean Destination Segmented Chips
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: destCol.implicitHeight + 20
                            radius: 8
                            color: "#1F222A"
                            border.color: "#2C303B"
                            border.width: 1

                            ColumnLayout {
                                id: destCol
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                Text { text: "📁 Save Destination"; color: "#E5E7EB"; font.pixelSize: 12; font.bold: true }

                                // Segmented Destination Selector Chips
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Button {
                                        text: "📁 Subfolder (/Resized/)"
                                        Layout.fillWidth: true
                                        font.pixelSize: 10
                                        font.bold: resizeEditor.destinationMode === 0
                                        background: Rectangle {
                                            color: resizeEditor.destinationMode === 0 ? "#1E3A8A" : "#282B34"
                                            radius: 4
                                            border.color: resizeEditor.destinationMode === 0 ? "#3B82F6" : "#374151"
                                            border.width: 1
                                        }
                                        contentItem: Text { text: parent.text; color: "white"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        onClicked: resizeEditor.destinationMode = 0
                                    }

                                    Button {
                                        text: "📂 Choose Folder..."
                                        Layout.fillWidth: true
                                        font.pixelSize: 10
                                        font.bold: resizeEditor.destinationMode === 1
                                        background: Rectangle {
                                            color: resizeEditor.destinationMode === 1 ? "#1E3A8A" : "#282B34"
                                            radius: 4
                                            border.color: resizeEditor.destinationMode === 1 ? "#3B82F6" : "#374151"
                                            border.width: 1
                                        }
                                        contentItem: Text { text: parent.text; color: "white"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        onClicked: {
                                            resizeEditor.destinationMode = 1;
                                            destinationFolderPicker.open();
                                        }
                                    }

                                    Button {
                                        text: "⚠️ Overwrite"
                                        Layout.preferredWidth: 85
                                        font.pixelSize: 10
                                        font.bold: resizeEditor.destinationMode === 2
                                        background: Rectangle {
                                            color: resizeEditor.destinationMode === 2 ? "#7F1D1D" : "#282B34"
                                            radius: 4
                                            border.color: resizeEditor.destinationMode === 2 ? "#EF4444" : "#374151"
                                            border.width: 1
                                        }
                                        contentItem: Text { text: parent.text; color: resizeEditor.destinationMode === 2 ? "#FCA5A5" : "white"; font: parent.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        onClicked: resizeEditor.destinationMode = 2
                                    }
                                }

                                Text {
                                    text: "Saving to: " + resizeEditor.getResolvedDestination()
                                    color: "#9CA3AF"
                                    font.pixelSize: 10
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- 3. MODAL FOOTER ACTIONS ---
        Rectangle {
            Layout.fillWidth: true
            height: 56
            color: "#1F222A"
            radius: 12
            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: "#30ffffff"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 12

                Text {
                    text: "Estimated Reduction: " + (resizeEditor.totalOrigSizeBytes > 0 ?
                          (1.0 - (resizeEditor.totalNewSizeBytes / resizeEditor.totalOrigSizeBytes)) * 100.0 : 0).toFixed(0) + "%"
                    color: "#9CA3AF"
                    font.pixelSize: 12
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Cancel"
                    implicitWidth: 90
                    implicitHeight: 36
                    background: Rectangle {
                        color: parent.hovered ? "#374151" : "#282B34"
                        radius: 6
                        border.color: "#4B5563"
                        border.width: 1
                    }
                    contentItem: Text { text: parent.text; color: "#E5E7EB"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: resizeEditor.reject()
                }

                Button {
                    text: "Resize & Save " + (resizeEditor.batchCount > 1 ? (resizeEditor.batchCount + " Images") : "Image")
                    implicitWidth: 180
                    implicitHeight: 36
                    background: Rectangle {
                        color: parent.hovered ? "#3B82F6" : "#2563EB"
                        radius: 6
                    }
                    contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: resizeEditor.performBatchResize()
                }
            }
        }
    }
}