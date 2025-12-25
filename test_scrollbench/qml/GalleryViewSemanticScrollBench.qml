import QtQuick
import QtQuick.Controls
import ScrollBenchBackend 1.0

Item {
    id: semanticRoot
    signal imageClicked(int index)
    
    property var model: imageModel
    
    // Grouping Mode: 1=Day, 2=Week, 3=Month, 4=Year
    property int groupingMode: 1
    property bool groupingAuto: true
    
    readonly property real gridSize: settings.gridSize
    readonly property alias proxyModel: proxyModel

    function findChildListView() { return list }
    
    // Auto-link grouping mode to grid size
    onGridSizeChanged: {
        if (groupingAuto) {
            if (gridSize < 65) groupingMode = 4 // Year
            else if (gridSize < 105) groupingMode = 3 // Month
            else if (gridSize < 165) groupingMode = 2 // Week
            else groupingMode = 1 // Day
        }
    }

    // Grouped Proxy Model
    GroupedProxyModel {
        id: proxyModel
        sourceModel: semanticRoot.model
        columns: Math.max(1, Math.floor((semanticRoot.width - 40) / (settings.gridSize + 10)))
        
        groupRole: {
            if (groupingMode === 1) return 260 // Day
            if (groupingMode === 2) return 263 // Week
            if (groupingMode === 3) return 261 // Month
            if (groupingMode === 4) return 262 // Year
            return 260
        }
    }

    // Robust Visibility Tracking using itemAt (Delegate peering)
    function updateVisibleRange() {
        if (!model || list.count === 0) return;
        
        // Peek at delegates at top and bottom of viewport
        // itemAt uses coordinates relative to the contentItem if we call it on contentItem,
        // but ListView.itemAt uses coordinates relative to the viewport.
        var topRow = list.itemAt(25, 20)
        var bottomRow = list.itemAt(25, list.height - 20)
        
        var sourceStart = -1
        var sourceEnd = -1
        
        if (topRow && topRow.hasOwnProperty("capturedStart")) {
            sourceStart = topRow.capturedStart
        } else {
            // Fallback if top is a header or empty space
            sourceStart = 0
        }
        
        if (bottomRow && bottomRow.hasOwnProperty("capturedStart")) {
            sourceEnd = bottomRow.capturedStart + bottomRow.capturedCount - 1
        } else {
            sourceEnd = model.totalItems - 1
        }
        
        if (sourceStart !== -1 && sourceEnd !== -1) {
            model.visibleStartIndex = Math.max(0, sourceStart)
            model.visibleEndIndex = Math.min(model.totalItems - 1, sourceEnd)
        }
    }

    ListView {
        id: list
        anchors.fill: parent
        model: proxyModel
        clip: true
        spacing: 12
        cacheBuffer: 1500
        
        onContentYChanged: rangeTimer.restart()
        onHeightChanged: rangeTimer.restart()
        
        Timer {
            id: rangeTimer
            interval: 100; repeat: false
            onTriggered: semanticRoot.updateVisibleRange()
        }

        delegate: Item {
            id: rowDelegate
            width: list.width
            height: type === 0 ? 40 : settings.gridSize + 12
            
            // These properties are scrutinized by updateVisibleRange via peering
            readonly property int capturedType: type
            readonly property int capturedStart: rowStartIndex
            readonly property int capturedCount: rowCount
            readonly property string capturedTitle: headerTitle

            // Header Delegate
            Rectangle {
                anchors.fill: parent
                color: "#1e1e1e"
                visible: rowDelegate.capturedType === 0
                
                Text {
                    anchors.left: parent.left; anchors.leftMargin: 20
                    anchors.verticalCenter: parent.verticalCenter
                    text: rowDelegate.capturedTitle || "No Date"
                    color: "#2196F3"; font.bold: true; font.pixelSize: 18
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#333" }
            }
            
            // Row Delegate
            Row {
                anchors.fill: parent
                anchors.leftMargin: 20; anchors.rightMargin: 20
                spacing: 10
                visible: rowDelegate.capturedType === 1
                
                Repeater {
                    model: rowDelegate.capturedType === 1 ? rowDelegate.capturedCount : 0
                    delegate: Item {
                        width: settings.gridSize
                        height: settings.gridSize
                        
                        property int sourceIdx: rowDelegate.capturedStart + index
                        
                        Image {
                            id: thumb
                            anchors.fill: parent
                            source: {
                                var path = semanticRoot.model.data(semanticRoot.model.index(sourceIdx, 0), 257)
                                return path ? "image://async/" + path : ""
                            }
                            sourceSize: Qt.size(settings.thumbnailSize, settings.thumbnailSize)
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: true
                            
                            Rectangle {
                                anchors.fill: parent; color: "#222"
                                visible: thumb.status !== Image.Ready
                                BusyIndicator { anchors.centerIn: parent; width: parent.width * 0.4; height: width; opacity: 0.5 }
                            }

                            // Selection States
                            Rectangle {
                                anchors.fill: parent; color: "#2196F3"
                                // IsSelectedRole is 409
                                property bool isSelected: semanticRoot.model.data(semanticRoot.model.index(sourceIdx, 0), 409)
                                opacity: isSelected ? 0.4 : 0
                                border.color: "#2196F3"; border.width: isSelected ? 2 : 0
                                
                                // Scaled Checkmark
                                Rectangle {
                                    width: Math.max(10, parent.width * 0.25); height: width; radius: width/2
                                    color: "#2196F3"; anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 2
                                    visible: parent.isSelected
                                    Text { anchors.centerIn: parent; text: "✓"; color: "white"; font.bold: true; font.pixelSize: parent.width * 0.7 }
                                }
                            }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: (mouse) => {
                                if (mouse.modifiers & Qt.ControlModifier) semanticRoot.model.toggleSelection(sourceIdx)
                                else semanticRoot.imageClicked(sourceIdx)
                            }
                        }
                    }
                }
            }
        }
        
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }

        WheelHandler {
            acceptedModifiers: Qt.ControlModifier
            onWheel: (wheel) => {
                var delta = wheel.angleDelta.y > 0 ? 20 : -20
                settings.gridSize = Math.max(30, Math.min(settings.gridSize + delta, 400))
            }
        }

        PinchHandler {
            property real baseSize
            onActiveChanged: if (active) baseSize = settings.gridSize
            onScaleChanged: if (active) settings.gridSize = Math.max(30, Math.min(baseSize * scale, 400))
        }
    }

    Component.onCompleted: updateVisibleRange()
}
