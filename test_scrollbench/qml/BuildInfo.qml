pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 185
    readonly property string buildTimestamp: "2026-08-03 00:42:39"
    readonly property string version: "1.0.185"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
