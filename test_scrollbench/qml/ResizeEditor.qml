import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: resizeEditor
    // List of images to resize
    property var targetPaths: []
    
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
    
    // Helper function to estimate new file size
    function updateSizeEstimate() {
        // Rough estimation: size scales with resolution and quality
        // This will be replaced with actual backend calculation in Phase 3.4
        let baseWidth = 1920
        let baseHeight = 1080
        
        let qualityFactor = quality / 100.0
        let compressionFactor = 1.0 - (compression / 15.0) // 0-9 scale, mild effect
        
        // Mock math to show *change*:
        newSizeMB = originalSizeMB * qualityFactor * compressionFactor
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
            Rectangle {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 2
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
                    text: resizeEditor.currentImagePath ? resizeEditor.currentImagePath.substring(resizeEditor.currentImagePath.lastIndexOf('/') + 1) : "No Selection"
                    color: "#888"
                    font.pixelSize: 12
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
                        
                        Text { text: "Quality: " + resizeEditor.quality + "%"; color: "white" }
                        Slider {
                            id: qualitySlider
                            from: 10; to: 100
                            value: resizeEditor.quality
                            stepSize: 5
                            Layout.fillWidth: true
                            onMoved: {
                                resizeEditor.quality = value
                                updateSizeEstimate()
                            }
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
                let size = Qt.size(resizeEditor.targetWidth, resizeEditor.targetHeight);
                
                for (let i = 0; i < resizeEditor.targetPaths.length; i++) {
                    let sourcePath = resizeEditor.targetPaths[i];
                    // Strip file:/// if present for backend
                    let cleanPath = sourcePath.replace("file:///", "").replace("file:", "");
                    
                    let fileName = cleanPath.substring(cleanPath.lastIndexOf('/') + 1);
                    if (fileName === "") fileName = cleanPath.substring(cleanPath.lastIndexOf('\\') + 1);
                    
                    let lastDot = fileName.lastIndexOf('.');
                    let fileExtension = lastDot !== -1 ? fileName.substring(lastDot + 1) : "jpg";
                    let baseName = lastDot !== -1 ? fileName.substring(0, lastDot) : fileName;
                    
                    let desktopPath = Qt.platform.homePath + "/Desktop/" + baseName + "_resized." + fileExtension;
                    
                    console.log("[RESIZE] Processing (" + (i+1) + "/" + resizeEditor.targetPaths.length + "):", cleanPath);
                    imageProcessor.resizeImage(cleanPath, desktopPath, size);
                }
                
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

    Component.onCompleted: {
        imageProcessor.imageProcessingError.connect(function(message) {
            console.error("Image processing error:", message);
            // Optionally show a QMessageBox or similar to the user
        });
    }
}