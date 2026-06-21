pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 57
    readonly property string buildTimestamp: "2026-06-21 10:19:29"
    readonly property string version: "1.0.57"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
