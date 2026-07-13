pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 112
    readonly property string buildTimestamp: "2026-07-11 23:15:49"
    readonly property string version: "1.0.112"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
