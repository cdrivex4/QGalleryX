import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: shareDialog
    
    property var model: null
    property var targetPaths: []
    
    title: {
        var count = targetPaths.length > 0 ? targetPaths.length : (model ? model.selectedCount : 0)
        return "Share " + (count === 1 ? "Image" : count + " Images")
    }
    modal: true
    anchors.centerIn: parent
    width: 320
    
    background: Rectangle {
        color: "#1e1e1e"
        radius: 8
        border.color: "#333"
    }

    // Custom header to override default light title bar
    header: Rectangle {
        color: "#2a2a2a"
        height: 50
        radius: 8
        
        // Hide bottom rounded corners to merge with content
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 8
            color: "#2a2a2a"
        }
        
        Text {
            anchors.centerIn: parent
            text: shareDialog.title
            color: "white"
            font.pixelSize: 16
            font.bold: true
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Select Destination Folder"
        onAccepted: {
            var destinationUrl = folderDialog.selectedFolder
            // Convert file:/// path to native path
            var destPath = destinationUrl.toString().replace("file:///", "")
            
            var paths = shareDialog.targetPaths.length > 0 ? shareDialog.targetPaths : shareDialog.model.getSelectedPaths()
            if (paths.length === 0) return
            
            var cleanPaths = []
            for (var i = 0; i < paths.length; i++) {
                cleanPaths.push(paths[i].replace("file:///", "").replace("file:", ""))
            }
            
            desktopHelper.copyFiles(cleanPaths, destPath)
            
            // Clear selection after sharing
            if (shareDialog.model) {
                shareDialog.model.clearSelection()
            }
            shareDialog.close()
        }
    }
    
    contentItem: ColumnLayout {
        spacing: 15
        
        Text {
            text: "Choose an action:"
            font.pixelSize: 14
            color: "#aaa"
            Layout.fillWidth: true
            Layout.bottomMargin: 10
        }

        // Email Resize Preset
        Button {
            text: "Resize for Email (Small)"
            Layout.fillWidth: true
            Layout.preferredHeight: 45
            background: Rectangle { color: parent.hovered ? "#3d3d3d" : "#333"; radius: 6 }
            contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            
            onClicked: {
                var paths = shareDialog.targetPaths.length > 0 ? shareDialog.targetPaths : shareDialog.model.getSelectedPaths()
                if (paths.length === 0) return

                var desktopPath = Qt.platform.homePath + "/Desktop/"
                var cleanPaths = []
                for (var i = 0; i < paths.length; i++) {
                    cleanPaths.push(paths[i].replace("file:///", "").replace("file:", ""))
                }
                desktopHelper.exportImages(cleanPaths, desktopPath, 1024, 768, 80, -1)
                if (shareDialog.model) shareDialog.model.clearSelection()
                shareDialog.close()
            }
        }
        
        // Manual Resize
        Button {
            text: "Resize Manually..."
            Layout.fillWidth: true
            Layout.preferredHeight: 45
            background: Rectangle { color: parent.hovered ? "#3d3d3d" : "#333"; radius: 6 }
            contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            
            onClicked: {
                var paths = shareDialog.targetPaths.length > 0 ? shareDialog.targetPaths : shareDialog.model.getSelectedPaths()
                if (paths.length === 0) return

                shareDialog.close()
                resizeEditor.targetPaths = paths
                resizeEditor.currentImagePath = paths[0].startsWith("file:") ? paths[0] : "file:///" + paths[0]
                resizeEditor.originalSizeBytes = (shareDialog.targetPaths.length === 0) ? shareDialog.model.getSelectedTotalSizeBytes() : 0 
                resizeEditor.open()
            }
        }
        
        // Send to Folder
        Button {
            text: "Send to Folder / Device"
            Layout.fillWidth: true
            Layout.preferredHeight: 45
            background: Rectangle { color: parent.hovered ? "#2196F3" : "#1976D2"; radius: 6 }
            contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            
            onClicked: {
                folderDialog.open()
            }
        }
        
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#404040"
            Layout.topMargin: 5
            Layout.bottomMargin: 5
        }
        
        Button {
            text: "Cancel"
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            background: Rectangle {
                color: parent.hovered ? "#383838" : "#282828"
                border.color: parent.hovered ? "#555555" : "#3d3d3d"
                border.width: 1
                radius: 6
            }
            contentItem: Text {
                text: parent.text
                color: "#e0e0e0"
                font.pixelSize: 13
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: shareDialog.close()
        }
    }
}

