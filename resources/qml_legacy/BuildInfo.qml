pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 578
    readonly property string buildTimestamp: "2026-09-03 23:15:41"
    readonly property string version: "1.0.578"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
