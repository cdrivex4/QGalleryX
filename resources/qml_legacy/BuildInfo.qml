pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 579
    readonly property string buildTimestamp: "2026-09-03 23:52:07"
    readonly property string version: "1.0.579"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
