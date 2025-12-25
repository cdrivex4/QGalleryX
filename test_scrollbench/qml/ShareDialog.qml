import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: shareDialog
    title: "Share " + (imageModel ? imageModel.selectedCount : 0) + " images"
    modal: true
    anchors.centerIn: parent
    
    ColumnLayout {
        spacing: 15
        width: 300
        
        Text {
            text: "Choose resize option:"
            font.pixelSize: 14
            color: "#FFFFFF"
            Layout.fillWidth: true
        }
        
        Button {
            text: "Resize for Email (Small)"
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            onClicked: {
                // Apply preset: 1024x768, 80% quality
                console.log("Email small preset - 1024x768, 80% quality")
                // TODO: Phase 3.4 - Apply resize
                shareDialog.close()
            }
        }
        
        Button {
            text: "Resize for Email (Manual)"
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            onClicked: {
                // Get data from model
                let paths = imageModel.getSelectedPaths()
                let totalSize = imageModel.getSelectedTotalSizeBytes()
                
                if (paths.length > 0) {
                    resizeEditor.currentImagePath = "file:///" + paths[0] // Preview first image
                    resizeEditor.originalSizeBytes = totalSize
                    resizeEditor.open()
                    shareDialog.close()
                } else {
                    console.warn("No images selected for resize")
                }
            }
        }
        
        Button {
            text: "Share Original Size"
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            onClicked: {
                // Share without modifications
                console.log("Share", imageModel.selectedCount, "images at original size")
                // TODO: Phase 3.4 - Share original
                shareDialog.close()
            }
        }
        
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#404040"
        }
        
        Button {
            text: "Cancel"
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            onClicked: shareDialog.close()
        }
    }
    
    // Resize Editor
    ResizeEditor {
        id: resizeEditor
    }
}
