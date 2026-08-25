import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QGalleryX 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: "#121316"
    visible: false
    z: 100

    property string sourcePath: ""
    property var model: null
    property int currentIndex: -1

    signal closed()
    signal saved(string newPath)

    // Editing State
    property int currentTab: 0 // 0: Crop/Rotate, 1: Filters, 2: Annotate, 3: Adjust
    property string activeFilter: "None"
    property real filterIntensity: 1.0
    property real exposureVal: 0.0
    property real contrastVal: 0.0
    property real saturationVal: 0.0
    property real temperatureVal: 0.0

    // Transform State
    property int rotationAngle: 0
    property bool isFlipH: false
    property bool isFlipV: false

    // Crop State (Normalized 0.0 - 1.0 relative to image wrapper)
    property real cropNormX: 0.02
    property real cropNormY: 0.02
    property real cropNormW: 0.96
    property real cropNormH: 0.96
    property string activeAspectRatio: "Free"

    // Annotation State
    property string activeAnnotateTool: "pen" // "pen", "arrow", "rect", "circle", "text", "eraser"
    property color activeColor: "#FF3B30"
    property real strokeWidth: 6.0
    property var annotationsList: []
    property int selectedTextIndex: -1

    // Live Preview State
    property string previewSourceUri: sourcePath ? ("image://async/" + sourcePath) : ""

    // Undo / Redo History Stack
    property var historyStack: []
    property int historyIndex: -1

    function closeWorkspace() {
        visible = false
        root.closed()
    }

    function openEditor(filePath, idx) {
        sourcePath = filePath || ""
        currentIndex = idx !== undefined ? idx : -1
        resetAllEdits()
        visible = true
        recordHistoryState()
    }

    function resetAllEdits() {
        activeFilter = "None"
        filterIntensity = 1.0
        exposureVal = 0.0
        contrastVal = 0.0
        saturationVal = 0.0
        temperatureVal = 0.0
        rotationAngle = 0
        isFlipH = false
        isFlipV = false
        cropNormX = 0.02
        cropNormY = 0.02
        cropNormW = 0.96
        cropNormH = 0.96
        activeAspectRatio = "Free"
        annotationsList = []
        selectedTextIndex = -1
        historyStack = []
        historyIndex = -1
        previewSourceUri = sourcePath ? ("image://async/" + sourcePath) : ""
        if (canvas) canvas.requestPaint()
    }

    function updateLivePreview() {
        if (!sourcePath) return;
        var hasAdjustments = (activeFilter !== "None" && filterIntensity > 0.01) ||
                             Math.abs(exposureVal) > 0.01 ||
                             Math.abs(contrastVal) > 0.01 ||
                             Math.abs(saturationVal) > 0.01 ||
                             Math.abs(temperatureVal) > 0.01;

        if (!hasAdjustments) {
            previewSourceUri = "image://async/" + sourcePath;
            return;
        }

        previewDebounceTimer.restart();
    }

    Timer {
        id: previewDebounceTimer
        interval: 50
        repeat: false
        onTriggered: {
            var url = imageProcessor.applyFilterPreview(
                root.sourcePath,
                root.activeFilter,
                root.filterIntensity,
                root.exposureVal,
                root.contrastVal,
                root.saturationVal,
                root.temperatureVal
            );
            if (url) {
                root.previewSourceUri = url + "?t=" + Date.now();
            }
        }
    }

    onActiveFilterChanged: updateLivePreview()
    onFilterIntensityChanged: updateLivePreview()
    onExposureValChanged: updateLivePreview()
    onContrastValChanged: updateLivePreview()
    onSaturationValChanged: updateLivePreview()
    onTemperatureValChanged: updateLivePreview()

    function recordHistoryState() {
        var state = {
            annotations: JSON.parse(JSON.stringify(annotationsList)),
            filter: activeFilter,
            filterIntensity: filterIntensity,
            exposure: exposureVal,
            contrast: contrastVal,
            saturation: saturationVal,
            temperature: temperatureVal,
            rotation: rotationAngle,
            flipH: isFlipH,
            flipV: isFlipV,
            cropX: cropNormX,
            cropY: cropNormY,
            cropW: cropNormW,
            cropH: cropNormH,
            aspect: activeAspectRatio
        }

        var newHistory = historyStack.slice(0, historyIndex + 1)
        newHistory.push(state)
        historyStack = newHistory
        historyIndex = historyStack.length - 1
    }

    function undo() {
        if (historyIndex > 0) {
            historyIndex--
            applyHistoryState(historyStack[historyIndex])
        }
    }

    function redo() {
        if (historyIndex < historyStack.length - 1) {
            historyIndex++
            applyHistoryState(historyStack[historyIndex])
        }
    }

    function applyHistoryState(state) {
        if (!state) return;
        annotationsList = JSON.parse(JSON.stringify(state.annotations || []))
        activeFilter = state.filter || "None"
        filterIntensity = state.filterIntensity !== undefined ? state.filterIntensity : 1.0
        exposureVal = state.exposure || 0.0
        contrastVal = state.contrast || 0.0
        saturationVal = state.saturation || 0.0
        temperatureVal = state.temperature || 0.0
        rotationAngle = state.rotation || 0
        isFlipH = !!state.flipH
        isFlipV = !!state.flipV
        cropNormX = state.cropX !== undefined ? state.cropX : 0.02
        cropNormY = state.cropY !== undefined ? state.cropY : 0.02
        cropNormW = state.cropW !== undefined ? state.cropW : 0.96
        cropNormH = state.cropH !== undefined ? state.cropH : 0.96
        activeAspectRatio = state.aspect || "Free"
        updateLivePreview()
        if (canvas) canvas.requestPaint()
    }

    function setAspectPreset(ratioStr) {
        activeAspectRatio = ratioStr
        var imgW = previewImg.sourceSize.width > 0 ? previewImg.sourceSize.width : imageWrapper.width
        var imgH = previewImg.sourceSize.height > 0 ? previewImg.sourceSize.height : imageWrapper.height
        if (imgW <= 0 || imgH <= 0) return;

        var isRot90 = (root.rotationAngle % 180 !== 0)
        var actualW = isRot90 ? imgH : imgW
        var actualH = isRot90 ? imgW : imgH
        var imgAspect = actualW / actualH

        if (ratioStr === "Free") {
            cropNormX = 0.02; cropNormY = 0.02; cropNormW = 0.96; cropNormH = 0.96;
            recordHistoryState()
            return;
        }

        var targetAspect = 1.0;
        if (ratioStr === "1:1") targetAspect = 1.0;
        else if (ratioStr === "4:3") targetAspect = 4.0 / 3.0;
        else if (ratioStr === "16:9") targetAspect = 16.0 / 9.0;
        else if (ratioStr === "9:16") targetAspect = 9.0 / 16.0;

        var targetNormW = 0.85;
        var targetNormH = targetNormW * (imgAspect / targetAspect);

        if (targetNormH > 0.90) {
            targetNormH = 0.85;
            targetNormW = targetNormH * (targetAspect / imgAspect);
        }

        targetNormW = Math.max(0.1, Math.min(targetNormW, 0.96));
        targetNormH = Math.max(0.1, Math.min(targetNormH, 0.96));

        cropNormW = targetNormW;
        cropNormH = targetNormH;
        cropNormX = (1.0 - targetNormW) / 2.0;
        cropNormY = (1.0 - targetNormH) / 2.0;
        recordHistoryState()
    }

    // Save Copy Folder Dialog
    FolderDialog {
        id: saveFolderDialog
        title: "Select Destination Folder for Saved Copy"
        currentFolder: {
            if (!sourcePath) return ""
            var baseDir = sourcePath.substring(0, Math.max(sourcePath.lastIndexOf("/"), sourcePath.lastIndexOf("\\")))
            return "file:///" + baseDir.replace(/\\/g, "/")
        }
        onAccepted: {
            var folder = selectedFolder.toString().replace("file:///", "").replace("file:", "")
            var ext = sourcePath.substring(sourcePath.lastIndexOf("."))
            var baseName = sourcePath.substring(Math.max(sourcePath.lastIndexOf("/"), sourcePath.lastIndexOf("\\")) + 1, sourcePath.lastIndexOf("."))
            var targetPath = folder + "/" + baseName + "_edited_" + Date.now() + ext
            performSave(targetPath, false)
        }
    }

    function requestSaveCopy() {
        saveFolderDialog.open()
    }

    function performSave(targetPath, isOverwrite) {
        var success = imageProcessor.saveEditedImage(
            sourcePath,
            targetPath,
            activeFilter,
            filterIntensity,
            exposureVal,
            contrastVal,
            saturationVal,
            temperatureVal,
            cropNormX,
            cropNormY,
            cropNormW,
            cropNormH,
            rotationAngle,
            isFlipH,
            isFlipV,
            annotationsList,
            95
        )

        if (success) {
            root.saved(targetPath)
            closeWorkspace()
        }
    }

    // Geometry Helpers for Strike-Through Eraser
    function distToSegment(x1, y1, x2, y2, px, py) {
        var l2 = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
        if (l2 === 0) return Math.hypot(px - x1, py - y1);
        var t = Math.max(0, Math.min(1, ((px - x1) * (x2 - x1) + (py - y1) * (y2 - y1)) / l2));
        var projX = x1 + t * (x2 - x1);
        var projY = y1 + t * (y2 - y1);
        return Math.hypot(px - projX, py - projY);
    }

    function eraseStrikeThrough(ex, ey) {
        var radius = 28;
        var hit = false;
        var list = annotationsList.slice();

        for (var i = list.length - 1; i >= 0; i--) {
            var item = list[i];
            if (item.type === "pen" && item.points) {
                for (var p = 0; p < item.points.length - 1; p++) {
                    if (distToSegment(item.points[p].x, item.points[p].y, item.points[p+1].x, item.points[p+1].y, ex, ey) < radius) {
                        list.splice(i, 1);
                        hit = true;
                        break;
                    }
                }
            } else if (item.type === "arrow") {
                if (distToSegment(item.x1, item.y1, item.x2, item.y2, ex, ey) < radius) {
                    list.splice(i, 1);
                    hit = true;
                }
            } else if (item.type === "rect") {
                var rHit = distToSegment(item.x, item.y, item.x + item.w, item.y, ex, ey) < radius ||
                           distToSegment(item.x + item.w, item.y, item.x + item.w, item.y + item.h, ex, ey) < radius ||
                           distToSegment(item.x + item.w, item.y + item.h, item.x, item.y + item.h, ex, ey) < radius ||
                           distToSegment(item.x, item.y + item.h, item.x, item.y, ex, ey) < radius;
                if (rHit) { list.splice(i, 1); hit = true; }
            } else if (item.type === "circle") {
                var cx = item.x + item.w / 2;
                var cy = item.y + item.h / 2;
                var cr = Math.hypot(item.w / 2, item.h / 2);
                if (Math.abs(Math.hypot(ex - cx, ey - cy) - cr) < radius || (Math.hypot(ex - cx, ey - cy) < cr)) {
                    list.splice(i, 1);
                    hit = true;
                }
            } else if (item.type === "text") {
                if (Math.hypot(ex - item.x, ey - item.y) < radius * 1.5) {
                    list.splice(i, 1);
                    hit = true;
                }
            }
        }

        if (hit) {
            annotationsList = list;
            canvas.requestPaint();
            recordHistoryState();
        }
    }

    // Keyboard Shortcuts
    focus: visible
    Keys.onPressed: (event) => {
        if (event.matches(StandardKey.Undo) || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Z)) {
            if (event.modifiers & Qt.ShiftModifier) redo()
            else undo()
            event.accepted = true
        } else if (event.matches(StandardKey.Redo) || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Y)) {
            redo()
            event.accepted = true
        } else if (event.key === Qt.Key_Escape) {
            closeWorkspace()
            event.accepted = true
        }
    }

    // --- TOP TOOLBAR ---
    Rectangle {
        id: topBar
        anchors.top: parent.top
        width: parent.width
        height: 56
        color: "#181A20"
        z: 30

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            StyledButton {
                text: "Cancel"
                iconText: "✕"
                fontSize: 13
                onClicked: root.closeWorkspace()
            }

            Rectangle { width: 1; height: 24; color: "#30ffffff" }

            StyledButton {
                iconText: "↶"
                ToolTip.visible: hovered
                ToolTip.text: "Undo (Ctrl+Z)"
                enabled: root.historyIndex > 0
                onClicked: root.undo()
            }

            StyledButton {
                iconText: "↷"
                ToolTip.visible: hovered
                ToolTip.text: "Redo (Ctrl+Y)"
                enabled: root.historyIndex < root.historyStack.length - 1
                onClicked: root.redo()
            }

            StyledButton {
                text: "Reset"
                fontSize: 13
                onClicked: root.resetAllEdits()
            }

            Item { Layout.fillWidth: true }

            Text {
                text: sourcePath.split(/[\/\\]/).pop()
                color: "#e0e0e0"
                font.pixelSize: 13
                font.bold: true
                elide: Text.ElideMiddle
                Layout.maximumWidth: 300
            }

            Item { Layout.fillWidth: true }

            StyledButton {
                text: "Overwrite"
                fontSize: 13
                hoverColor: "#50f87171"
                onClicked: root.performSave(root.sourcePath, true)
            }

            StyledButton {
                text: "Save Copy..."
                isAccent: true
                fontBold: true
                fontSize: 13
                onClicked: root.requestSaveCopy()
            }
        }
    }

    // --- CENTER CANVAS & IMAGE WORKSPACE ---
    Item {
        id: canvasArea
        anchors.top: topBar.bottom
        anchors.bottom: bottomTray.top
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true

        // Click on background deactivates active text typing
        MouseArea {
            anchors.fill: parent
            z: 1
            onClicked: {
                root.selectedTextIndex = -1
            }
        }

        Item {
            id: imageWrapper
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.88, 800)
            height: Math.min(parent.height * 0.88, 600)
            z: 2

            Binding {
                target: imageWrapper
                property: "width"
                value: {
                    if (previewImg.implicitWidth <= 0 || previewImg.implicitHeight <= 0) return canvasArea.width * 0.85;
                    var maxW = canvasArea.width * 0.88;
                    var maxH = canvasArea.height * 0.88;
                    var isRot90 = (root.rotationAngle % 180 !== 0);
                    var imgW = isRot90 ? previewImg.implicitHeight : previewImg.implicitWidth;
                    var imgH = isRot90 ? previewImg.implicitWidth : previewImg.implicitHeight;
                    var aspect = imgW / imgH;
                    return (maxW / maxH > aspect) ? (maxH * aspect) : maxW;
                }
            }
            Binding {
                target: imageWrapper
                property: "height"
                value: {
                    if (previewImg.implicitWidth <= 0 || previewImg.implicitHeight <= 0) return canvasArea.height * 0.85;
                    var maxW = canvasArea.width * 0.88;
                    var maxH = canvasArea.height * 0.88;
                    var isRot90 = (root.rotationAngle % 180 !== 0);
                    var imgW = isRot90 ? previewImg.implicitHeight : previewImg.implicitWidth;
                    var imgH = isRot90 ? previewImg.implicitWidth : previewImg.implicitHeight;
                    var aspect = imgW / imgH;
                    return (maxW / maxH > aspect) ? maxH : (maxW / aspect);
                }
            }

            // Base Image Display (Always Visible with Live Preview Source)
            Image {
                id: previewImg
                anchors.fill: parent
                source: root.previewSourceUri
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                autoTransform: true
                rotation: root.rotationAngle
                transform: Scale {
                    xScale: root.isFlipH ? -1 : 1
                    yScale: root.isFlipV ? -1 : 1
                    origin.x: previewImg.width / 2
                    origin.y: previewImg.height / 2
                }
            }

            // Vector Annotation Drawing Layer
            Canvas {
                id: canvas
                anchors.fill: parent
                z: 10
                renderTarget: Canvas.Image

                property var currentStroke: null

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)

                    for (var i = 0; i < root.annotationsList.length; i++) {
                        var itm = root.annotationsList[i]
                        if (itm.type !== "text") {
                            drawAnnotationItem(ctx, itm)
                        }
                    }

                    if (currentStroke) {
                        drawAnnotationItem(ctx, currentStroke)
                    }
                }

                function drawAnnotationItem(ctx, item) {
                    if (!item) return;
                    ctx.save()
                    ctx.lineWidth = item.width || 4
                    ctx.strokeStyle = item.color || "#FF3B30"
                    ctx.fillStyle = item.color || "#FF3B30"
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"

                    if (item.type === "pen") {
                        if (item.points && item.points.length >= 2) {
                            ctx.beginPath()
                            ctx.moveTo(item.points[0].x, item.points[0].y)
                            for (var p = 1; p < item.points.length; p++) {
                                ctx.lineTo(item.points[p].x, item.points[p].y)
                            }
                            ctx.stroke()
                        }
                    } else if (item.type === "arrow") {
                        ctx.beginPath()
                        ctx.moveTo(item.x1, item.y1)
                        ctx.lineTo(item.x2, item.y2)
                        ctx.stroke()

                        var angle = Math.atan2(item.y2 - item.y1, item.x2 - item.x1)
                        var headLen = Math.max(12, item.width * 3.2)
                        ctx.beginPath()
                        ctx.moveTo(item.x2, item.y2)
                        ctx.lineTo(item.x2 - headLen * Math.cos(angle - Math.PI / 6), item.y2 - headLen * Math.sin(angle - Math.PI / 6))
                        ctx.lineTo(item.x2 - headLen * Math.cos(angle + Math.PI / 6), item.y2 - headLen * Math.sin(angle + Math.PI / 6))
                        ctx.closePath()
                        ctx.fill()
                    } else if (item.type === "rect") {
                        ctx.beginPath()
                        ctx.strokeRect(item.x, item.y, item.w, item.h)
                    } else if (item.type === "circle") {
                        ctx.beginPath()
                        ctx.ellipse(item.x, item.y, Math.abs(item.w), Math.abs(item.h))
                        ctx.stroke()
                    }
                    ctx.restore()
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: root.currentTab === 2
                    cursorShape: {
                        if (root.activeAnnotateTool === "eraser") return Qt.ForbiddenCursor;
                        if (root.activeAnnotateTool === "text") return Qt.IBeamCursor;
                        return Qt.CrossCursor;
                    }

                    property real startX: 0
                    property real startY: 0

                    onPressed: (mouse) => {
                        startX = mouse.x
                        startY = mouse.y

                        if (root.activeAnnotateTool === "pen") {
                            canvas.currentStroke = {
                                type: "pen",
                                color: root.activeColor.toString(),
                                width: root.strokeWidth,
                                points: [{x: mouse.x, y: mouse.y}]
                            }
                        } else if (root.activeAnnotateTool === "arrow") {
                            canvas.currentStroke = {
                                type: "arrow",
                                color: root.activeColor.toString(),
                                width: root.strokeWidth,
                                x1: mouse.x, y1: mouse.y, x2: mouse.x, y2: mouse.y
                            }
                        } else if (root.activeAnnotateTool === "rect") {
                            canvas.currentStroke = {
                                type: "rect",
                                color: root.activeColor.toString(),
                                width: root.strokeWidth,
                                x: mouse.x, y: mouse.y, w: 0, h: 0
                            }
                        } else if (root.activeAnnotateTool === "circle") {
                            canvas.currentStroke = {
                                type: "circle",
                                color: root.activeColor.toString(),
                                width: root.strokeWidth,
                                x: mouse.x, y: mouse.y, w: 0, h: 0
                            }
                        } else if (root.activeAnnotateTool === "text") {
                            var newItem = {
                                type: "text",
                                text: "Type here",
                                x: mouse.x,
                                y: mouse.y,
                                boxWidth: 180,
                                color: root.activeColor.toString(),
                                fontSize: 20,
                                rotation: 0
                            }
                            var list = root.annotationsList.slice()
                            list.push(newItem)
                            root.annotationsList = list
                            root.selectedTextIndex = list.length - 1
                            root.recordHistoryState()
                            return;
                        } else if (root.activeAnnotateTool === "eraser") {
                            root.eraseStrikeThrough(mouse.x, mouse.y)
                        }
                        canvas.requestPaint()
                    }

                    onPositionChanged: (mouse) => {
                        if (root.activeAnnotateTool === "eraser") {
                            root.eraseStrikeThrough(mouse.x, mouse.y)
                            return;
                        }

                        if (!canvas.currentStroke) return;

                        if (root.activeAnnotateTool === "pen") {
                            canvas.currentStroke.points.push({x: mouse.x, y: mouse.y})
                        } else if (root.activeAnnotateTool === "arrow") {
                            canvas.currentStroke.x2 = mouse.x
                            canvas.currentStroke.y2 = mouse.y
                        } else if (root.activeAnnotateTool === "rect" || root.activeAnnotateTool === "circle") {
                            canvas.currentStroke.x = Math.min(startX, mouse.x)
                            canvas.currentStroke.y = Math.min(startY, mouse.y)
                            canvas.currentStroke.w = Math.abs(mouse.x - startX)
                            canvas.currentStroke.h = Math.abs(mouse.y - startY)
                        }
                        canvas.requestPaint()
                    }

                    onReleased: {
                        if (canvas.currentStroke) {
                            var list = root.annotationsList.slice()
                            list.push(canvas.currentStroke)
                            root.annotationsList = list
                            canvas.currentStroke = null
                            canvas.requestPaint()
                            root.recordHistoryState()
                        }
                    }
                }
            }

            // Interactive Text Elements with Word-Wrap & Object Manipulation
            Repeater {
                model: {
                    var items = []
                    for (var i = 0; i < root.annotationsList.length; i++) {
                        if (root.annotationsList[i].type === "text") {
                            items.push({ itemIndex: i, itemData: root.annotationsList[i] })
                        }
                    }
                    return items
                }

                delegate: Item {
                    id: textElement
                    z: 25
                    x: modelData.itemData.x
                    y: modelData.itemData.y
                    width: Math.max(120, modelData.itemData.boxWidth || 180)
                    height: Math.max(40, textEdit.contentHeight + 20)
                    rotation: modelData.itemData.rotation || 0

                    property bool isSelected: root.selectedTextIndex === modelData.itemIndex
                    property bool isTypingMode: isSelected && textEdit.activeFocus

                    Rectangle {
                        anchors.fill: parent
                        color: isSelected ? "#C0181A20" : "transparent"
                        radius: 6
                        border.color: isSelected ? "#2196F3" : "transparent"
                        border.width: 1.5
                    }

                    TextEdit {
                        id: textEdit
                        anchors.fill: parent
                        anchors.margins: 6
                        text: modelData.itemData.text
                        color: modelData.itemData.color || "white"
                        font.pixelSize: modelData.itemData.fontSize || 20
                        font.bold: true
                        font.family: "Segoe UI"
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                        cursorVisible: isTypingMode
                        focus: isSelected

                        onTextChanged: {
                            modelData.itemData.text = text
                        }
                        onEditingFinished: {
                            root.recordHistoryState()
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: textElement.isTypingMode ? -1 : 0
                        cursorShape: textElement.isTypingMode ? Qt.IBeamCursor : Qt.SizeAllCursor
                        drag.target: textElement.isTypingMode ? null : textElement
                        drag.minimumX: 0
                        drag.maximumX: imageWrapper.width - textElement.width
                        drag.minimumY: 0
                        drag.maximumY: imageWrapper.height - textElement.height

                        onPressed: {
                            root.selectedTextIndex = modelData.itemIndex
                            textEdit.forceActiveFocus()
                        }
                        onReleased: {
                            modelData.itemData.x = textElement.x
                            modelData.itemData.y = textElement.y
                            root.recordHistoryState()
                        }
                    }

                    Rectangle {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.topMargin: -10
                        anchors.rightMargin: -10
                        width: 20
                        height: 20
                        radius: 10
                        color: "#E53935"
                        visible: textElement.isSelected
                        z: 30

                        Text {
                            anchors.centerIn: parent
                            text: "✕"
                            color: "white"
                            font.pixelSize: 11
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                var list = root.annotationsList.slice()
                                list.splice(modelData.itemIndex, 1)
                                root.annotationsList = list
                                root.selectedTextIndex = -1
                                root.recordHistoryState()
                            }
                        }
                    }

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: -6
                        width: 12
                        height: 20
                        radius: 3
                        color: "#2196F3"
                        visible: textElement.isSelected
                        z: 30

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.SizeHorCursor
                            property real startX: 0
                            property real initW: 180

                            onPressed: (m) => { startX = m.x; initW = textElement.width }
                            onPositionChanged: (m) => {
                                var delta = m.x - startX
                                var newW = Math.max(100, Math.min(initW + delta, imageWrapper.width - textElement.x))
                                modelData.itemData.boxWidth = newW
                                textElement.width = newW
                            }
                            onReleased: root.recordHistoryState()
                        }
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        anchors.bottomMargin: -6
                        anchors.rightMargin: -6
                        width: 14
                        height: 14
                        radius: 3
                        color: "#64B5F6"
                        visible: textElement.isSelected
                        z: 30

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.SizeFDiagCursor
                            property real startY: 0
                            property int initFont: 20

                            onPressed: (m) => { startY = m.y; initFont = modelData.itemData.fontSize || 20 }
                            onPositionChanged: (m) => {
                                var delta = (m.y - startY) * 0.4
                                modelData.itemData.fontSize = Math.max(12, Math.min(initFont + delta, 72))
                            }
                            onReleased: root.recordHistoryState()
                        }
                    }
                }
            }

            // Darkened Dimmer Mask Outside Crop Box
            Item {
                id: cropDimmer
                anchors.fill: parent
                visible: root.currentTab === 0
                z: 14

                Rectangle {
                    x: 0; y: 0; width: parent.width; height: cropBox.y
                    color: "#A0000000"
                }
                Rectangle {
                    x: 0; y: cropBox.y + cropBox.height; width: parent.width; height: parent.height - (cropBox.y + cropBox.height)
                    color: "#A0000000"
                }
                Rectangle {
                    x: 0; y: cropBox.y; width: cropBox.x; height: cropBox.height
                    color: "#A0000000"
                }
                Rectangle {
                    x: cropBox.x + cropBox.width; y: cropBox.y; width: parent.width - (cropBox.x + cropBox.width); height: cropBox.height
                    color: "#A0000000"
                }
            }

            // Interactive 8-Handle Crop Box
            Item {
                id: cropBox
                visible: root.currentTab === 0
                z: 15
                x: root.cropNormX * imageWrapper.width
                y: root.cropNormY * imageWrapper.height
                width: root.cropNormW * imageWrapper.width
                height: root.cropNormH * imageWrapper.height

                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.color: "#FFFFFF"
                    border.width: 2

                    // Rule of Thirds Grid
                    Rectangle { x: parent.width / 3; y: 0; width: 1; height: parent.height; color: "#70ffffff" }
                    Rectangle { x: (2 * parent.width) / 3; y: 0; width: 1; height: parent.height; color: "#70ffffff" }
                    Rectangle { x: 0; y: parent.height / 3; width: parent.width; height: 1; color: "#70ffffff" }
                    Rectangle { x: 0; y: (2 * parent.height) / 3; width: parent.width; height: 1; color: "#70ffffff" }
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: 14
                    cursorShape: Qt.SizeAllCursor
                    property real dragStartX: 0
                    property real dragStartY: 0
                    property real initCropX: 0
                    property real initCropY: 0

                    onPressed: (mouse) => {
                        dragStartX = mouse.x
                        dragStartY = mouse.y
                        initCropX = root.cropNormX
                        initCropY = root.cropNormY
                    }
                    onPositionChanged: (mouse) => {
                        var dx = (mouse.x - dragStartX) / imageWrapper.width
                        var dy = (mouse.y - dragStartY) / imageWrapper.height
                        root.cropNormX = Math.max(0.0, Math.min(initCropX + dx, 1.0 - root.cropNormW))
                        root.cropNormY = Math.max(0.0, Math.min(initCropY + dy, 1.0 - root.cropNormH))
                    }
                    onReleased: root.recordHistoryState()
                }

                // 1. Top-Left (NW)
                Rectangle {
                    x: -8; y: -8; width: 18; height: 18; color: "#FFFFFF"; radius: 4
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.SizeFDiagCursor
                        property real startX: 0
                        property real startY: 0
                        onPressed: (m) => { startX = m.x; startY = m.y }
                        onPositionChanged: (m) => {
                            var deltaX = (m.x - startX) / imageWrapper.width
                            var deltaY = (m.y - startY) / imageWrapper.height
                            var newX = Math.max(0.0, Math.min(root.cropNormX + deltaX, root.cropNormX + root.cropNormW - 0.1))
                            var newY = Math.max(0.0, Math.min(root.cropNormY + deltaY, root.cropNormY + root.cropNormH - 0.1))
                            root.cropNormW += (root.cropNormX - newX)
                            root.cropNormH += (root.cropNormY - newY)
                            root.cropNormX = newX
                            root.cropNormY = newY
                        }
                        onReleased: root.recordHistoryState()
                    }
                }

                // 2. Top-Right (NE)
                Rectangle {
                    x: parent.width - 10; y: -8; width: 18; height: 18; color: "#FFFFFF"; radius: 4
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.SizeBDiagCursor
                        property real startX: 0
                        property real startY: 0
                        onPressed: (m) => { startX = m.x; startY = m.y }
                        onPositionChanged: (m) => {
                            var deltaX = (m.x - startX) / imageWrapper.width
                            var deltaY = (m.y - startY) / imageWrapper.height
                            var newY = Math.max(0.0, Math.min(root.cropNormY + deltaY, root.cropNormY + root.cropNormH - 0.1))
                            root.cropNormH += (root.cropNormY - newY)
                            root.cropNormY = newY
                            root.cropNormW = Math.max(0.1, Math.min(root.cropNormW + deltaX, 1.0 - root.cropNormX))
                        }
                        onReleased: root.recordHistoryState()
                    }
                }

                // 3. Bottom-Left (SW)
                Rectangle {
                    x: -8; y: parent.height - 10; width: 18; height: 18; color: "#FFFFFF"; radius: 4
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.SizeBDiagCursor
                        property real startX: 0
                        property real startY: 0
                        onPressed: (m) => { startX = m.x; startY = m.y }
                        onPositionChanged: (m) => {
                            var deltaX = (m.x - startX) / imageWrapper.width
                            var deltaY = (m.y - startY) / imageWrapper.height
                            var newX = Math.max(0.0, Math.min(root.cropNormX + deltaX, root.cropNormX + root.cropNormW - 0.1))
                            root.cropNormW += (root.cropNormX - newX)
                            root.cropNormX = newX
                            root.cropNormH = Math.max(0.1, Math.min(root.cropNormH + deltaY, 1.0 - root.cropNormY))
                        }
                        onReleased: root.recordHistoryState()
                    }
                }

                // 4. Bottom-Right (SE)
                Rectangle {
                    x: parent.width - 10; y: parent.height - 10; width: 18; height: 18; color: "#FFFFFF"; radius: 4
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.SizeFDiagCursor
                        property real startX: 0
                        property real startY: 0
                        onPressed: (m) => { startX = m.x; startY = m.y }
                        onPositionChanged: (m) => {
                            var deltaX = (m.x - startX) / imageWrapper.width
                            var deltaY = (m.y - startY) / imageWrapper.height
                            root.cropNormW = Math.max(0.1, Math.min(root.cropNormW + deltaX, 1.0 - root.cropNormX))
                            root.cropNormH = Math.max(0.1, Math.min(root.cropNormH + deltaY, 1.0 - root.cropNormY))
                        }
                        onReleased: root.recordHistoryState()
                    }
                }

                // 5. Top-Middle (N)
                Rectangle {
                    x: parent.width / 2 - 8; y: -6; width: 16; height: 12; color: "#FFFFFF"; radius: 3
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.SizeVerCursor
                        property real startY: 0
                        onPressed: (m) => { startY = m.y }
                        onPositionChanged: (m) => {
                            var deltaY = (m.y - startY) / imageWrapper.height
                            var newY = Math.max(0.0, Math.min(root.cropNormY + deltaY, root.cropNormY + root.cropNormH - 0.1))
                            root.cropNormH += (root.cropNormY - newY)
                            root.cropNormY = newY
                        }
                        onReleased: root.recordHistoryState()
                    }
                }

                // 6. Bottom-Middle (S)
                Rectangle {
                    x: parent.width / 2 - 8; y: parent.height - 6; width: 16; height: 12; color: "#FFFFFF"; radius: 3
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.SizeVerCursor
                        property real startY: 0
                        onPressed: (m) => { startY = m.y }
                        onPositionChanged: (m) => {
                            var deltaY = (m.y - startY) / imageWrapper.height
                            root.cropNormH = Math.max(0.1, Math.min(root.cropNormH + deltaY, 1.0 - root.cropNormY))
                        }
                        onReleased: root.recordHistoryState()
                    }
                }

                // 7. Left-Middle (W)
                Rectangle {
                    x: -6; y: parent.height / 2 - 8; width: 12; height: 16; color: "#FFFFFF"; radius: 3
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.SizeHorCursor
                        property real startX: 0
                        onPressed: (m) => { startX = m.x }
                        onPositionChanged: (m) => {
                            var deltaX = (m.x - startX) / imageWrapper.width
                            var newX = Math.max(0.0, Math.min(root.cropNormX + deltaX, root.cropNormX + root.cropNormW - 0.1))
                            root.cropNormW += (root.cropNormX - newX)
                            root.cropNormX = newX
                        }
                        onReleased: root.recordHistoryState()
                    }
                }

                // 8. Right-Middle (E)
                Rectangle {
                    x: parent.width - 6; y: parent.height / 2 - 8; width: 12; height: 16; color: "#FFFFFF"; radius: 3
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.SizeHorCursor
                        property real startX: 0
                        onPressed: (m) => { startX = m.x }
                        onPositionChanged: (m) => {
                            var deltaX = (m.x - startX) / imageWrapper.width
                            root.cropNormW = Math.max(0.1, Math.min(root.cropNormW + deltaX, 1.0 - root.cropNormX))
                        }
                        onReleased: root.recordHistoryState()
                    }
                }
            }
        }
    }

    // --- BOTTOM TOOL TRAY & CONTROLS ---
    Rectangle {
        id: bottomTray
        anchors.bottom: parent.bottom
        width: parent.width
        height: 220
        color: "#181A20"
        z: 30

        // 1. Tool Options Panel
        Item {
            id: toolContentPanel
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: bottomTabBar.top
            anchors.margins: 10

            StackLayout {
                anchors.fill: parent
                currentIndex: root.currentTab

                // Tab 0: Crop & Transform
                RowLayout {
                    spacing: 24
                    anchors.centerIn: parent

                    ColumnLayout {
                        spacing: 6
                        Text { text: "Aspect Ratio"; color: "#aaaaaa"; font.pixelSize: 11; font.bold: true }
                        RowLayout {
                            spacing: 6
                            Repeater {
                                model: ["Free", "1:1", "4:3", "16:9", "9:16"]
                                delegate: StyledButton {
                                    text: modelData
                                    fontSize: 12
                                    isAccent: root.activeAspectRatio === modelData
                                    onClicked: root.setAspectPreset(modelData)
                                }
                            }
                        }
                    }

                    Rectangle { width: 1; height: 50; color: "#30ffffff" }

                    ColumnLayout {
                        spacing: 6
                        Text { text: "Transform"; color: "#aaaaaa"; font.pixelSize: 11; font.bold: true }
                        RowLayout {
                            spacing: 6
                            StyledButton {
                                text: "Rotate 90°"
                                iconText: "↻"
                                onClicked: {
                                    root.rotationAngle = (root.rotationAngle + 90) % 360
                                    root.recordHistoryState()
                                }
                            }
                            StyledButton {
                                text: "Flip H"
                                iconText: "⇄"
                                isAccent: root.isFlipH
                                onClicked: {
                                    root.isFlipH = !root.isFlipH
                                    root.recordHistoryState()
                                }
                            }
                            StyledButton {
                                text: "Flip V"
                                iconText: "⇅"
                                isAccent: root.isFlipV
                                onClicked: {
                                    root.isFlipV = !root.isFlipV
                                    root.recordHistoryState()
                                }
                            }
                        }
                    }
                }

                // Tab 1: Filters (12 Presets)
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 65
                        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                        ScrollBar.horizontal.policy: ScrollBar.AsNeeded

                        RowLayout {
                            spacing: 8
                            Repeater {
                                model: imageProcessor.getAvailableFilters()
                                delegate: Rectangle {
                                    width: 90
                                    height: 56
                                    radius: 6
                                    color: root.activeFilter === modelData ? "#1976D2" : "#262933"
                                    border.color: root.activeFilter === modelData ? "#64B5F6" : "#35ffffff"
                                    border.width: root.activeFilter === modelData ? 2 : 1

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        Text {
                                            text: "🎨"
                                            font.pixelSize: 16
                                            Layout.alignment: Qt.AlignHCenter
                                        }
                                        Text {
                                            text: modelData
                                            color: "white"
                                            font.pixelSize: 10
                                            font.bold: true
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            root.activeFilter = modelData
                                            root.recordHistoryState()
                                        }
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Text { text: "Filter Strength"; color: "#cccccc"; font.pixelSize: 12; font.bold: true }

                        Slider {
                            id: filterSlider
                            Layout.fillWidth: true
                            from: 0.0
                            to: 1.0
                            value: root.filterIntensity
                            onMoved: root.filterIntensity = value
                            onPressedChanged: if (!pressed) root.recordHistoryState()

                            background: Rectangle {
                                x: filterSlider.leftPadding
                                y: filterSlider.topPadding + filterSlider.availableHeight / 2 - 3
                                implicitWidth: 160
                                implicitHeight: 6
                                width: filterSlider.availableWidth
                                height: 6
                                radius: 3
                                color: "#353842"

                                Rectangle {
                                    width: filterSlider.visualPosition * parent.width
                                    height: 6
                                    radius: 3
                                    color: "#2196F3"
                                }
                            }

                            handle: Rectangle {
                                x: filterSlider.leftPadding + filterSlider.visualPosition * (filterSlider.availableWidth - width)
                                y: filterSlider.topPadding + filterSlider.availableHeight / 2 - height / 2
                                implicitWidth: 18
                                implicitHeight: 18
                                radius: 9
                                color: filterSlider.pressed ? "#64B5F6" : "#FFFFFF"
                            }
                        }

                        Rectangle {
                            width: 50
                            height: 22
                            radius: 4
                            color: "#252830"
                            Text {
                                anchors.centerIn: parent
                                text: Math.round(root.filterIntensity * 100) + "%"
                                color: "white"
                                font.pixelSize: 11
                                font.bold: true
                            }
                        }
                    }
                }

                // Tab 2: Annotate & Draw
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 6

                        Repeater {
                            model: [
                                {id: "pen", label: "✏️ Pen"},
                                {id: "arrow", label: "➔ Arrow"},
                                {id: "rect", label: "▭ Box"},
                                {id: "circle", label: "○ Circle"},
                                {id: "text", label: "🔤 Text"},
                                {id: "eraser", label: "🧹 Eraser"}
                            ]
                            delegate: StyledButton {
                                text: modelData.label
                                isAccent: root.activeAnnotateTool === modelData.id
                                fontSize: 12
                                onClicked: root.activeAnnotateTool = modelData.id
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 10

                        RowLayout {
                            spacing: 6
                            Repeater {
                                model: ["#FF3B30", "#FF9500", "#FFCC00", "#34C759", "#007AFF", "#5856D6", "#AF52DE", "#FFFFFF", "#000000"]
                                delegate: Rectangle {
                                    width: 22
                                    height: 22
                                    radius: 11
                                    color: modelData
                                    border.color: root.activeColor.toString().toLowerCase() === modelData.toLowerCase() ? "#FFFFFF" : "#40000000"
                                    border.width: root.activeColor.toString().toLowerCase() === modelData.toLowerCase() ? 3 : 1

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.activeColor = modelData
                                    }
                                }
                            }
                        }

                        Rectangle { width: 1; height: 20; color: "#30ffffff" }

                        Text { text: "Width: " + Math.round(root.strokeWidth); color: "#aaa"; font.pixelSize: 11 }
                        Slider {
                            implicitWidth: 100
                            from: 2.0
                            to: 24.0
                            value: root.strokeWidth
                            onMoved: root.strokeWidth = value
                        }
                    }
                }

                // Tab 3: Adjust (Center-Zero Expansion Sliders with Numerical Range [-100 ... 0 ... +100])
                GridLayout {
                    columns: 2
                    columnSpacing: 24
                    rowSpacing: 8
                    anchors.centerIn: parent
                    Layout.fillWidth: true

                    // 1. Exposure
                    ColumnLayout {
                        spacing: 2
                        Layout.preferredWidth: 320
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "☀️ Exposure"; color: "#cccccc"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: 44
                                height: 20
                                radius: 4
                                color: Math.round(root.exposureVal * (100.0 / 1.5)) === 0 ? "#252830" : "#1976D2"
                                Text {
                                    anchors.centerIn: parent
                                    text: {
                                        var v = Math.round(root.exposureVal * (100.0 / 1.5))
                                        return (v > 0 ? "+" : "") + v
                                    }
                                    color: "white"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    ToolTip.visible: hovered
                                    ToolTip.text: "Reset to 0"
                                    onClicked: { root.exposureVal = 0; root.recordHistoryState() }
                                }
                            }
                        }
                        Slider {
                            id: expSlider
                            Layout.fillWidth: true
                            from: -100
                            to: 100
                            value: root.exposureVal * (100.0 / 1.5)
                            onMoved: root.exposureVal = value * (1.5 / 100.0)
                            onPressedChanged: if (!pressed) root.recordHistoryState()

                            background: Rectangle {
                                x: expSlider.leftPadding
                                y: expSlider.topPadding + expSlider.availableHeight / 2 - 3
                                implicitWidth: 160
                                implicitHeight: 6
                                width: expSlider.availableWidth
                                height: 6
                                radius: 3
                                color: "#353842"

                                Rectangle { x: parent.width / 2 - 1; y: -2; width: 2; height: 10; color: "#70ffffff"; radius: 1 }
                                Rectangle {
                                    property real centerPos: parent.width / 2
                                    property real thumbPos: expSlider.visualPosition * parent.width
                                    x: Math.min(centerPos, thumbPos)
                                    width: Math.abs(thumbPos - centerPos)
                                    height: 6
                                    radius: 3
                                    color: Math.round(expSlider.value) === 0 ? "transparent" : "#2196F3"
                                }
                            }
                            handle: Rectangle {
                                x: expSlider.leftPadding + expSlider.visualPosition * (expSlider.availableWidth - width)
                                y: expSlider.topPadding + expSlider.availableHeight / 2 - height / 2
                                implicitWidth: 18
                                implicitHeight: 18
                                radius: 9
                                color: expSlider.pressed ? "#64B5F6" : "#FFFFFF"
                            }
                        }
                    }

                    // 2. Contrast
                    ColumnLayout {
                        spacing: 2
                        Layout.preferredWidth: 320
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "◐ Contrast"; color: "#cccccc"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: 44
                                height: 20
                                radius: 4
                                color: Math.round(root.contrastVal * (100.0 / 0.8)) === 0 ? "#252830" : "#1976D2"
                                Text {
                                    anchors.centerIn: parent
                                    text: {
                                        var v = Math.round(root.contrastVal * (100.0 / 0.8))
                                        return (v > 0 ? "+" : "") + v
                                    }
                                    color: "white"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    ToolTip.visible: hovered
                                    ToolTip.text: "Reset to 0"
                                    onClicked: { root.contrastVal = 0; root.recordHistoryState() }
                                }
                            }
                        }
                        Slider {
                            id: contSlider
                            Layout.fillWidth: true
                            from: -100
                            to: 100
                            value: root.contrastVal * (100.0 / 0.8)
                            onMoved: root.contrastVal = value * (0.8 / 100.0)
                            onPressedChanged: if (!pressed) root.recordHistoryState()

                            background: Rectangle {
                                x: contSlider.leftPadding
                                y: contSlider.topPadding + contSlider.availableHeight / 2 - 3
                                implicitWidth: 160
                                implicitHeight: 6
                                width: contSlider.availableWidth
                                height: 6
                                radius: 3
                                color: "#353842"

                                Rectangle { x: parent.width / 2 - 1; y: -2; width: 2; height: 10; color: "#70ffffff"; radius: 1 }
                                Rectangle {
                                    property real centerPos: parent.width / 2
                                    property real thumbPos: contSlider.visualPosition * parent.width
                                    x: Math.min(centerPos, thumbPos)
                                    width: Math.abs(thumbPos - centerPos)
                                    height: 6
                                    radius: 3
                                    color: Math.round(contSlider.value) === 0 ? "transparent" : "#2196F3"
                                }
                            }
                            handle: Rectangle {
                                x: contSlider.leftPadding + contSlider.visualPosition * (contSlider.availableWidth - width)
                                y: contSlider.topPadding + contSlider.availableHeight / 2 - height / 2
                                implicitWidth: 18
                                implicitHeight: 18
                                radius: 9
                                color: contSlider.pressed ? "#64B5F6" : "#FFFFFF"
                            }
                        }
                    }

                    // 3. Saturation
                    ColumnLayout {
                        spacing: 2
                        Layout.preferredWidth: 320
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "🌈 Saturation"; color: "#cccccc"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: 44
                                height: 20
                                radius: 4
                                color: Math.round(root.saturationVal * 100.0) === 0 ? "#252830" : "#1976D2"
                                Text {
                                    anchors.centerIn: parent
                                    text: {
                                        var v = Math.round(root.saturationVal * 100.0)
                                        return (v > 0 ? "+" : "") + v
                                    }
                                    color: "white"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    ToolTip.visible: hovered
                                    ToolTip.text: "Reset to 0"
                                    onClicked: { root.saturationVal = 0; root.recordHistoryState() }
                                }
                            }
                        }
                        Slider {
                            id: satSlider
                            Layout.fillWidth: true
                            from: -100
                            to: 100
                            value: root.saturationVal * 100.0
                            onMoved: root.saturationVal = value * (1.0 / 100.0)
                            onPressedChanged: if (!pressed) root.recordHistoryState()

                            background: Rectangle {
                                x: satSlider.leftPadding
                                y: satSlider.topPadding + satSlider.availableHeight / 2 - 3
                                implicitWidth: 160
                                implicitHeight: 6
                                width: satSlider.availableWidth
                                height: 6
                                radius: 3
                                color: "#353842"

                                Rectangle { x: parent.width / 2 - 1; y: -2; width: 2; height: 10; color: "#70ffffff"; radius: 1 }
                                Rectangle {
                                    property real centerPos: parent.width / 2
                                    property real thumbPos: satSlider.visualPosition * parent.width
                                    x: Math.min(centerPos, thumbPos)
                                    width: Math.abs(thumbPos - centerPos)
                                    height: 6
                                    radius: 3
                                    color: Math.round(satSlider.value) === 0 ? "transparent" : "#2196F3"
                                }
                            }
                            handle: Rectangle {
                                x: satSlider.leftPadding + satSlider.visualPosition * (satSlider.availableWidth - width)
                                y: satSlider.topPadding + satSlider.availableHeight / 2 - height / 2
                                implicitWidth: 18
                                implicitHeight: 18
                                radius: 9
                                color: satSlider.pressed ? "#64B5F6" : "#FFFFFF"
                            }
                        }
                    }

                    // 4. Temperature (Cool Blue on left, Warm Amber on right)
                    ColumnLayout {
                        spacing: 2
                        Layout.preferredWidth: 320
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "🌡 Temperature"; color: "#cccccc"; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: 44
                                height: 20
                                radius: 4
                                color: Math.round(root.temperatureVal * 100.0) === 0 ? "#252830" : (root.temperatureVal > 0 ? "#E65100" : "#0277BD")
                                Text {
                                    anchors.centerIn: parent
                                    text: {
                                        var v = Math.round(root.temperatureVal * 100.0)
                                        return (v > 0 ? "+" : "") + v
                                    }
                                    color: "white"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    ToolTip.visible: hovered
                                    ToolTip.text: "Reset to 0"
                                    onClicked: { root.temperatureVal = 0; root.recordHistoryState() }
                                }
                            }
                        }
                        Slider {
                            id: tempSlider
                            Layout.fillWidth: true
                            from: -100
                            to: 100
                            value: root.temperatureVal * 100.0
                            onMoved: root.temperatureVal = value * (1.0 / 100.0)
                            onPressedChanged: if (!pressed) root.recordHistoryState()

                            background: Rectangle {
                                x: tempSlider.leftPadding
                                y: tempSlider.topPadding + tempSlider.availableHeight / 2 - 3
                                implicitWidth: 160
                                implicitHeight: 6
                                width: tempSlider.availableWidth
                                height: 6
                                radius: 3
                                color: "#353842"

                                Rectangle { x: parent.width / 2 - 1; y: -2; width: 2; height: 10; color: "#70ffffff"; radius: 1 }
                                Rectangle {
                                    property real centerPos: parent.width / 2
                                    property real thumbPos: tempSlider.visualPosition * parent.width
                                    x: Math.min(centerPos, thumbPos)
                                    width: Math.abs(thumbPos - centerPos)
                                    height: 6
                                    radius: 3
                                    color: Math.round(tempSlider.value) === 0 ? "transparent" : (tempSlider.value > 0 ? "#FF9800" : "#03A9F4")
                                }
                            }
                            handle: Rectangle {
                                x: tempSlider.leftPadding + tempSlider.visualPosition * (tempSlider.availableWidth - width)
                                y: tempSlider.topPadding + tempSlider.availableHeight / 2 - height / 2
                                implicitWidth: 18
                                implicitHeight: 18
                                radius: 9
                                color: tempSlider.pressed ? "#64B5F6" : "#FFFFFF"
                            }
                        }
                    }
                }
            }
        }

        // Horizontal Separator
        Rectangle {
            anchors.bottom: bottomTabBar.top
            width: parent.width
            height: 1
            color: "#30ffffff"
        }

        // 2. Persistent Tab Bar Navigation
        Rectangle {
            id: bottomTabBar
            anchors.bottom: parent.bottom
            width: parent.width
            height: 52
            color: "#121418"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 12
                Layout.alignment: Qt.AlignHCenter

                Item { Layout.fillWidth: true }

                StyledButton {
                    text: "Crop & Rotate"
                    iconText: "✂"
                    isAccent: root.currentTab === 0
                    onClicked: root.currentTab = 0
                }

                StyledButton {
                    text: "Filters (12)"
                    iconText: "🎨"
                    isAccent: root.currentTab === 1
                    onClicked: root.currentTab = 1
                }

                StyledButton {
                    text: "Draw & Annotate"
                    iconText: "✏"
                    isAccent: root.currentTab === 2
                    onClicked: root.currentTab = 2
                }

                StyledButton {
                    text: "Adjustments"
                    iconText: "🎛"
                    isAccent: root.currentTab === 3
                    onClicked: root.currentTab = 3
                }

                Item { Layout.fillWidth: true }
            }
        }
    }
}
