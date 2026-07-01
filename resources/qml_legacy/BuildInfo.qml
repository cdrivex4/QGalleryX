pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 96
    readonly property string buildTimestamp: "2026-07-01 16:57:41"
    readonly property string version: "1.0.96"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
