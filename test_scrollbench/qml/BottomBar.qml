import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    height: 60
    color: "#000000"
    
    signal tabSelected(int index)
    property int currentIndex: 0

    function focusTab(idx) {
        var targetIdx = (idx !== undefined && idx >= 0 && idx < 3) ? idx : root.currentIndex
        if (repeater.itemAt(targetIdx)) {
            repeater.itemAt(targetIdx).forceActiveFocus()
        }
    }

    RowLayout {
        id: tabRow
        anchors.fill: parent
        spacing: 0

        Repeater {
            id: repeater
            model: ["Pictures", "Albums", "Menu"]
            
            ItemDelegate {
                id: tabItem
                Layout.fillWidth: true
                Layout.fillHeight: true
                activeFocusOnTab: true
                focus: true

                background: Rectangle {
                    color: tabItem.pressed ? "#333333" : (tabItem.hovered || tabItem.activeFocus ? "#222222" : "transparent")
                    border.color: tabItem.activeFocus ? "#38BDF8" : "transparent"
                    border.width: tabItem.activeFocus ? 2 : 0
                    radius: 4

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -2
                        color: "transparent"
                        border.color: "#FFFFFF"
                        border.width: 1
                        radius: 6
                        opacity: 0.8
                        visible: tabItem.activeFocus
                    }
                }

                contentItem: ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 4

                    SvgIcon {
                        iconName: modelData === "Pictures" ? "pictures" : 
                                  modelData === "Albums" ? "albums" : "settings"
                        size: 22
                        color: root.currentIndex === index || tabItem.activeFocus ? "#38BDF8" : "#888888"
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: modelData
                        color: root.currentIndex === index || tabItem.activeFocus ? "white" : "#888888"
                        font.pixelSize: 12
                        font.bold: root.currentIndex === index || tabItem.activeFocus
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                onClicked: root.tabSelected(index)

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Left) {
                        if (index > 0 && repeater.itemAt(index - 1)) {
                            repeater.itemAt(index - 1).forceActiveFocus()
                        }
                        event.accepted = true
                    } else if (event.key === Qt.Key_Right) {
                        if (index < 2 && repeater.itemAt(index + 1)) {
                            repeater.itemAt(index + 1).forceActiveFocus()
                        }
                        event.accepted = true
                    } else if (event.key === Qt.Key_Up) {
                        if (window.activeGrid) {
                            window.activeGrid.forceActiveFocus()
                        } else {
                            searchField.forceActiveFocus()
                        }
                        event.accepted = true
                    } else if (event.key === Qt.Key_Tab) {
                        if (index < 2 && repeater.itemAt(index + 1)) {
                            repeater.itemAt(index + 1).forceActiveFocus()
                        } else {
                            searchField.forceActiveFocus()
                        }
                        event.accepted = true
                    } else if (event.key === Qt.Key_Backtab) {
                        if (index > 0 && repeater.itemAt(index - 1)) {
                            repeater.itemAt(index - 1).forceActiveFocus()
                        } else if (window.activeGrid) {
                            window.activeGrid.forceActiveFocus()
                        } else {
                            searchField.forceActiveFocus()
                        }
                        event.accepted = true
                    } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                        root.tabSelected(index)
                        event.accepted = true
                    }
                }
            }
        }
    }
}
