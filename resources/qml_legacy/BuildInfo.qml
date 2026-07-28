pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 127
    readonly property string buildTimestamp: "2026-07-29 00:27:50"
    readonly property string version: "1.0.127"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
