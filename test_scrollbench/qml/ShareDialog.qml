import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: shareDialog
    
    // Support injected model
    property var model: imageModel // Default to global, can be overridden
    
    // The specific paths to share. If empty, uses model selection.
    property var targetPaths: []
    property bool isSingleMode: targetPaths.length === 1 || (targetPaths.length === 0 && model && model.selectedCount === 1)
    
    title: {
        var count = targetPaths.length > 0 ? targetPaths.length : (model ? model.selectedCount : 0)
        return "Share " + (count === 1 ? "Image" : count + " Images")
    }
    modal: true
    anchors.centerIn: parent
    
    ColumnLayout {
        spacing: 15
        width: 300
        
        Text {
            text: "Choose validation action:" // "share functionality ... takes you to another screen where you can decide"
            font.pixelSize: 14
            color: "#FFFFFF"
            Layout.fillWidth: true
        }

        // Resize for Email Preset
        Button {
            text: "Resize for Email (Small)"
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            onClicked: {
                var paths = shareDialog.targetPaths.length > 0 ? shareDialog.targetPaths : model.getSelectedPaths()
                if (paths.length === 0) return

                console.log("Email resize triggered for", paths.length, "images")
                shareDialog.close()
                // Directly trigger a batch resize with preset 1024x768
                for (var i = 0; i < paths.length; i++) {
                    var cleanPath = paths[i].replace("file:///", "").replace("file:", "")
                    var fileName = cleanPath.substring(cleanPath.lastIndexOf('/') + 1)
                    if (fileName === "") fileName = cleanPath.substring(cleanPath.lastIndexOf('\\') + 1)
                    var desktopPath = Qt.platform.homePath + "/Desktop/" + fileName.substring(0, fileName.lastIndexOf('.')) + "_email.jpg"
                    imageProcessor.resizeImage(cleanPath, desktopPath, Qt.size(1024, 768))
                }
            }
        }
        
        // Manual Resize
        Button {
            text: "Resize Manually..."
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            onClicked: {
                var paths = shareDialog.targetPaths.length > 0 ? shareDialog.targetPaths : model.getSelectedPaths()
                if (paths.length === 0) return

                shareDialog.close()
                resizeEditor.targetPaths = paths
                // For preview, use first image
                resizeEditor.currentImagePath = paths[0].startsWith("file:") ? paths[0] : "file:///" + paths[0]
                
                // If we have explicit targetPaths, we might need a way to get their size if not selected
                // But usually this comes from the viewer or gallery selection
                resizeEditor.originalSizeBytes = (shareDialog.targetPaths.length === 0) ? model.getSelectedTotalSizeBytes() : 0 
                resizeEditor.open()
            }
        }
        
        Button {
            text: "Share Original Size"
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            onClicked: {
                var paths = shareDialog.targetPaths.length > 0 ? shareDialog.targetPaths : model.getSelectedPaths()
                console.log("Sharing", paths.length, "original images via System API (Placeholder)")
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
