pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 97
    readonly property string buildTimestamp: "2026-07-01 23:02:58"
    readonly property string version: "1.0.97"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
