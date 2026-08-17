import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: resizeEditor
    // List of images to resize
    property var targetPaths: []
    
    parent: Overlay.overlay
    
    title: "Resize " + (targetPaths.length === 1 ? "Image" : targetPaths.length + " Images")
    modal: true
    width: parent ? parent.width * 0.9 : 1000
    height: parent ? parent.height * 0.9 : 700
    
    // Properties for resize settings
    property int targetWidth: 1024
    property int targetHeight: 768
    property int quality: 80
    property int compression: 5
    
    // Real data properties
    property string currentImagePath: ""
    property int originalSizeBytes: 0
    property real originalSizeMB: originalSizeBytes / (1024 * 1024)
    property real newSizeMB: 0.0
    
    onOpened: {
        if (targetPaths.length > 0) {
            let path = targetPaths[0]
            currentImagePath = path.replace("file:///", "").replace("file:", "")
            originalSizeBytes = desktopHelper.getFileSize ? desktopHelper.getFileSize(currentImagePath) : 5000000
            updateSizeEstimate()
        }
    }

    // Helper function to estimate new file size
    function updateSizeEstimate() {
        if (!currentImagePath || currentImagePath === "") return;
        var res = desktopHelper.generateResizePreview(currentImagePath, targetWidth, targetHeight, quality, compression);
        if (res && res.path) {
            previewImage.source = res.path + "?cache_buster=" + Math.random()
            newSizeMB = res.size / (1024 * 1024)
        }
    }
    
    contentItem: Rectangle {
        color: "#2b2b2b"
        implicitWidth: 900
        implicitHeight: 600
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20
            
            // Left: Preview
            RowLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                spacing: 10
                
                // Original View
                Rectangle {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    color: "#1a1a1a"
                    border.color: "#3d3d3d"
                    radius: 4
                    
                    Image {
                        anchors.fill: parent
                        anchors.margins: 2
                        fillMode: Image.PreserveAspectFit
                        source: "image://async/" + resizeEditor.currentImagePath
                        asynchronous: true
                    }
                    
                    Text {
                        anchors.bottom: parent.bottom
                        anchors.margins: 10
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Before"
                        color: "#888"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }
                
                // Preview View
                Rectangle {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    color: "#1a1a1a"
                    border.color: "#3d3d3d"
                    radius: 4
                    
                    Image {
                        id: previewImage
                        anchors.fill: parent
                        anchors.margins: 2
                        fillMode: Image.PreserveAspectFit
                        source: resizeEditor.currentImagePath
                        asynchronous: true
                    }
                    
                    Text {
                        anchors.bottom: parent.bottom
                        anchors.margins: 10
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "After Preview"
                        color: "#888"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }
            }
            
            // Right: Controls
            ColumnLayout {
                Layout.fillHeight: true
                Layout.preferredWidth: 320
                spacing: 15
                
                Text {
                    text: "Resize Settings"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.bottomMargin: 10
                }
                
                // Dimensions
                GroupBox {
                    title: "Dimensions"
                    Layout.fillWidth: true
                    label: Text { text: parent.title; color: "#ccc"; font.bold: true }
                    background: Rectangle { color: "#333"; radius: 4; border.color: "#555" }
                    
                    ColumnLayout {
                        width: parent.width
                        RowLayout {
                            Text { text: "Width:"; color: "white"; Layout.preferredWidth: 70 }
                            SpinBox {
                                id: widthSpin
                                from: 100; to: 8000
                                value: resizeEditor.targetWidth
                                editable: true
                                onValueChanged: {
                                    if (value !== resizeEditor.targetWidth) {
                                        resizeEditor.targetWidth = value
                                        updateSizeEstimate()
                                    }
                                }
                                Layout.fillWidth: true
                            }
                        }
                        RowLayout {
                            Text { text: "Height:"; color: "white"; Layout.preferredWidth: 70 }
                            SpinBox {
                                id: heightSpin
                                from: 100; to: 8000
                                value: resizeEditor.targetHeight
                                editable: true
                                onValueChanged: {
                                    if (value !== resizeEditor.targetHeight) {
                                        resizeEditor.targetHeight = value
                                        updateSizeEstimate()
                                    }
                                }
                                Layout.fillWidth: true
                            }
                        }
                        
                        CheckBox {
                            text: "Preserve Aspect Ratio"
                            checked: true
                            enabled: false // Logic implementation needed for true lock
                            contentItem: Text { text: parent.text; color: "#aaa"; leftPadding: 26; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                }
                
                // Quality
                GroupBox {
                    title: "Quality & Compression"
                    Layout.fillWidth: true
                    label: Text { text: parent.title; color: "#ccc"; font.bold: true }
                    background: Rectangle { color: "#333"; radius: 4; border.color: "#555" }
                    
                    ColumnLayout {
                        width: parent.width
                        
                        Text { text: resizeEditor.quality === 101 ? "Quality: Lossless (PNG)" : "Quality: " + resizeEditor.quality + "%"; color: "white" }
                        Slider {
                            id: qualitySlider
                            from: 10; to: 100
                            value: resizeEditor.quality
                            stepSize: 5
                            Layout.fillWidth: true
                            enabled: !losslessCheck.checked
                            onMoved: {
                                resizeEditor.quality = value
                                updateSizeEstimate()
                            }
                        }
                        CheckBox {
                            id: losslessCheck
                            text: "Lossless Output (No Degradation)"
                            checked: resizeEditor.quality === 101
                            onCheckedChanged: {
                                if (checked) {
                                    resizeEditor.quality = 101
                                } else {
                                    resizeEditor.quality = qualitySlider.value
                                }
                                updateSizeEstimate()
                            }
                            contentItem: Text { text: parent.text; color: "#aaa"; leftPadding: 26; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                }
                
                // File Info
                GroupBox {
                    title: "Output Estimate"
                    Layout.fillWidth: true
                    label: Text { text: parent.title; color: "#ccc"; font.bold: true }
                    background: Rectangle { color: "#333"; radius: 4; border.color: "#555" }
                    
                    GridLayout {
                        columns: 2
                        width: parent.width
                        columnSpacing: 20
                        
                        Text { text: "Original:"; color: "#aaa" }
                        Text { text: resizeEditor.originalSizeMB.toFixed(2) + " MB"; color: "white" }
                        
                        Text { text: "Estimated:"; color: "#aaa" }
                        Text { text: resizeEditor.newSizeMB.toFixed(2) + " MB"; color: "#4CAF50"; font.bold: true }
                        
                        Text { text: "Reduction:"; color: "#aaa" }
                        Text { 
                            text: Math.round((1 - resizeEditor.newSizeMB/resizeEditor.originalSizeMB)*100) + "%"
                            color: "#2196F3" 
                        }
                    }
                }
                
                Item { Layout.fillHeight: true } // Spacer
            }
        }
    }

    footer: DialogButtonBox {
        id: dialogButtonBox
        standardButtons: DialogButtonBox.Save | DialogButtonBox.Cancel

        onAccepted: {
            // "Save" button clicked - Batch or Single
            if (resizeEditor.targetPaths.length > 0) {
                var desktopPath = Qt.platform.homePath + "/Desktop/"
                var cleanPaths = []
                for (let i = 0; i < resizeEditor.targetPaths.length; i++) {
                    cleanPaths.push(resizeEditor.targetPaths[i].replace("file:///", "").replace("file:", ""))
                }
                desktopHelper.exportImages(cleanPaths, desktopPath, resizeEditor.targetWidth, resizeEditor.targetHeight, resizeEditor.quality, resizeEditor.compression)
                resizeEditor.close();
            } else {
                console.warn("No images in targetPaths for resizing.");
            }
        }

        onRejected: {
            // "Cancel" button clicked
            resizeEditor.close();
        }
    }
}