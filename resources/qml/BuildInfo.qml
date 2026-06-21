pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 62
    readonly property string buildTimestamp: "2026-06-21 10:48:48"
    readonly property string version: "1.0.62"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
