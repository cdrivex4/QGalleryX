pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 126
    readonly property string buildTimestamp: "2026-07-28 23:58:14"
    readonly property string version: "1.0.126"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
