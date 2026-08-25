pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 577
    readonly property string buildTimestamp: "2026-08-26 03:51:27"
    readonly property string version: "1.0.577"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
