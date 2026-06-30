pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 91
    readonly property string buildTimestamp: "2026-07-01 01:05:10"
    readonly property string version: "1.0.91"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
