pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 66
    readonly property string buildTimestamp: "2026-06-30 13:58:50"
    readonly property string version: "1.0.66"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
