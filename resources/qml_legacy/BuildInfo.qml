pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 199
    readonly property string buildTimestamp: "2026-08-03 22:58:25"
    readonly property string version: "1.0.199"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
