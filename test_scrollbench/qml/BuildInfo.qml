pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 193
    readonly property string buildTimestamp: "2026-08-03 02:18:21"
    readonly property string version: "1.0.193"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
