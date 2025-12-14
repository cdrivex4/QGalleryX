import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: resizeEditor
    title: "Resize Images - " + (imageModel ? imageModel.selectedCount : 0) + " selected"
    modal: true
    width: parent ? parent.width * 0.9 : 1000
    height: parent ? parent.height * 0.9 : 700
    
    // Properties for resize settings
    property int targetWidth: 1024
    property int targetHeight: 768
    property int quality: 80
    property int compression: 5
    
    // Placeholder - will need image processing backend in Phase 3.4
    property string currentImagePath: ""
    property real originalSizeMB: 0.0
    property real newSizeMB: 0.0
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        
        // Split-screen preview
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10
            
            // Original image (left)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#2d2d2d"
                border.color: "#404040"
                border.width: 1
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 5
                    
                    Text {
                        Layout.fillWidth: true
                        text: "Original"
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        padding: 5
                    }
                    
                    // Original image preview
                    Flickable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: originalImage.width * originalImage.scale
                        contentHeight: originalImage.height * originalImage.scale
                        
                        Image {
                            id: originalImage
                            source: resizeEditor.currentImagePath
                            fillMode: Image.PreserveAspectFit
                            anchors.centerIn: parent
                            property real scale: 1.0
                            
                            transform: Scale {
                                origin.x: originalImage.width / 2
                                origin.y: originalImage.height / 2
                                xScale: originalImage.scale
                                yScale: originalImage.scale
                            }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onWheel: (wheel) => {
                                let delta = wheel.angleDelta.y > 0 ? 0.1 : -0.1
                                originalImage.scale = Math.max(0.1, Math.min(5.0, originalImage.scale + delta))
                            }
                        }
                    }
                    
                    Text {
                        Layout.fillWidth: true
                        text: "Size: " + resizeEditor.originalSizeMB.toFixed(2) + " MB"
                        color: "#AAAAAA"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        padding: 5
                    }
                }
            }
            
            // Processed image (right)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#2d2d2d"
                border.color: "#2196F3"
                border.width: 2
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 5
                    
                    Text {
                        Layout.fillWidth: true
                        text: "Resized (" + resizeEditor.targetWidth + "x" + resizeEditor.targetHeight + ")"
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        padding: 5
                    }
                    
                    // Processed image preview
                    Flickable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: processedImage.width * processedImage.scale
                        contentHeight: processedImage.height * processedImage.scale
                        
                        Rectangle {
                            anchors.centerIn: parent
                            width: resizeEditor.targetWidth
                            height: resizeEditor.targetHeight
                            color: "#1e1e1e"
                            border.color: "#404040"
                            border.width: 1
                            
                            Image {
                                id: processedImage
                                anchors.fill: parent
                                source: resizeEditor.currentImagePath
                                fillMode: Image.PreserveAspectFit
                                property real scale: 1.0
                                
                                transform: Scale {
                                    origin.x: processedImage.width / 2
                                    origin.y: processedImage.height / 2
                                    xScale: processedImage.scale
                                    yScale: processedImage.scale
                                }
                            }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onWheel: (wheel) => {
                                let delta = wheel.angleDelta.y > 0 ? 0.1 : -0.1
                                processedImage.scale = Math.max(0.1, Math.min(5.0, processedImage.scale + delta))
                            }
                        }
                    }
                    
                    Text {
                        Layout.fillWidth: true
                        text: "Est. Size: " + resizeEditor.newSizeMB.toFixed(2) + " MB"
                        color: "#2196F3"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        padding: 5
                    }
                }
            }
        }
        
        // Controls section
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            color: "#252525"
            border.color: "#404040"
            border.width: 1
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 10
                
                // Dimensions
                RowLayout {
                    Layout.fillWidth: true
                    
                    Text {
                        text: "Dimensions:"
                        color: "#FFFFFF"
                        font.pixelSize: 12
                        Layout.preferredWidth: 100
                    }
                    
                    SpinBox {
                        id: widthSpinBox
                        from: 100
                        to: 4000
                        value: resizeEditor.targetWidth
                        editable: true
                        onValueChanged: resizeEditor.targetWidth = value
                        Layout.preferredWidth: 120
                    }
                    
                    Text {
                        text: "×"
                        color: "#FFFFFF"
                        font.pixelSize: 14
                    }
                    
                    SpinBox {
                        id: heightSpinBox
                        from: 100
                        to: 4000
                        value: resizeEditor.targetHeight
                        editable: true
                        onValueChanged: resizeEditor.targetHeight = value
                        Layout.preferredWidth: 120
                    }
                    
                    Button {
                        text: "Preset: Email"
                        onClicked: {
                            resizeEditor.targetWidth = 1024
                            resizeEditor.targetHeight = 768
                        }
                    }
                    
                    Item { Layout.fillWidth: true }
                }
                
                // Quality slider
                RowLayout {
                    Layout.fillWidth: true
                    
                    Text {
                        text: "Quality:"
                        color: "#FFFFFF"
                        font.pixelSize: 12
                        Layout.preferredWidth: 100
                    }
                    
                    Slider {
                        id: qualitySlider
                        from: 1
                        to: 100
                        value: resizeEditor.quality
                        stepSize: 1
                        Layout.fillWidth: true
                        onMoved: {
                            resizeEditor.quality = Math.round(value)
                            updateSizeEstimate()
                        }
                    }
                    
                    Text {
                        text: resizeEditor.quality + "%"
                        color: "#2196F3"
                        font.pixelSize: 12
                        Layout.preferredWidth: 50
                    }
                }
                
                // Compression slider
                RowLayout {
                    Layout.fillWidth: true
                    
                    Text {
                        text: "Compression:"
                        color: "#FFFFFF"
                        font.pixelSize: 12
                        Layout.preferredWidth: 100
                    }
                    
                    Slider {
                        id: compressionSlider
                        from: 0
                        to: 9
                        value: resizeEditor.compression
                        stepSize: 1
                        Layout.fillWidth: true
                        onMoved: {
                            resizeEditor.compression = Math.round(value)
                            updateSizeEstimate()
                        }
                    }
                    
                    Text {
                        text: resizeEditor.compression + " (0=none, 9=max)"
                        color: "#2196F3"
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                    }
                }
                
                // Size summary
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: "#1e1e1e"
                    border.color: "#404040"
                    border.width: 1
                    radius: 3
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Total: " + (resizeEditor.originalSizeMB * imageModel.selectedCount).toFixed(1) + " MB → " + 
                              (resizeEditor.newSizeMB * imageModel.selectedCount).toFixed(1) + " MB (" + 
                              (imageModel.selectedCount) + " images)"
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.bold: true
                    }
                }
            }
        }
        
        // Action buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            
            Button {
                text: "Cancel"
                Layout.preferredWidth: 100
                onClicked: resizeEditor.close()
            }
            
            Item { Layout.fillWidth: true }
            
            Button {
                text: "Apply & Share"
                Layout.preferredWidth: 120
                highlighted: true
                onClicked: {
                    console.log("Apply resize:", resizeEditor.targetWidth + "x" + resizeEditor.targetHeight, 
                                "Q:" + resizeEditor.quality, "C:" + resizeEditor.compression)
                    // TODO: Phase 3.4 - Actually process images
                    resizeEditor.close()
                }
            }
        }
    }
    
    // Helper function to estimate new file size
    function updateSizeEstimate() {
        // Rough estimation: size scales with resolution and quality
        // This will be replaced with actual backend calculation in Phase 3.4
        let resolutionRatio = (targetWidth * targetHeight) / (1920 * 1080)  // Assume 1920x1080 original
        let qualityFactor = quality / 100.0
        let compressionFactor = 1.0 - (compression / 10.0)
        
        newSizeMB = originalSizeMB * resolutionRatio * qualityFactor * compressionFactor
    }
    
    Component.onCompleted: {
        // Placeholder values - will be populated from actual image data in Phase 3.4
        originalSizeMB = 2.5
        updateSizeEstimate()
    }
}
