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
    
    onOpened: {
        updateSizeEstimate()
    }

    footer: DialogButtonBox {
        id: dialogButtonBox
        standardButtons: DialogButtonBox.Save | DialogButtonBox.Cancel

        onAccepted: {
            // "Save" button clicked
            if (imageModel && imageModel.selectedItem && imageModel.selectedItem.path) {
                let sourcePath = imageModel.selectedItem.path;
                let fileName = sourcePath.substring(sourcePath.lastIndexOf('/') + 1);
                let fileExtension = fileName.substring(fileName.lastIndexOf('.') + 1);
                let baseName = fileName.substring(0, fileName.lastIndexOf('.'));
                let destinationPath = "file:///" + Qt.platform.homePath + "/Desktop/" + baseName + "_resized." + fileExtension; // Save to desktop for now

                let size = Qt.size(resizeEditor.targetWidth, resizeEditor.targetHeight);
                console.log("Resizing image:", sourcePath, "to", size, "saving to", destinationPath);
                
                imageProcessor.resizeImage(sourcePath, destinationPath, size);
                resizeEditor.close();
            } else {
                console.warn("No image selected for resizing.");
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