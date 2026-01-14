import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: shareDialog
    
    // Support injected model
    property var model: imageModel // Default to global, can be overridden
    
    title: "Share " + (model ? model.selectedCount : 0) + " images"
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

        // Edit
        Button {
            text: "Edit"
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            onClicked: {
                console.log("Edit action triggered")
                // Logic to open editor (maybe rotation for batch?)
                shareDialog.close()
            }
        }
        
        // Export
        Button {
            text: "Export"
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            onClicked: {
                console.log("Export action triggered")
                // Logic to open export dialog
                shareDialog.close()
            }
        }
        
        // Resize (Existing)
        Button {
            text: "Resize"
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            onClicked: {
                 // Expand to show resize sub-options or open resize tool directly
                 // "decide between edit, export, resize" -> implies resize is a top level choice
                 shareDialog.close()
                 resizeEditor.currentImagePath = "file:///" + model.getSelectedPaths()[0]
                 resizeEditor.originalSizeBytes = model.getSelectedTotalSizeBytes()
                 resizeEditor.open()
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
