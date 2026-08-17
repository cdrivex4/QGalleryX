pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 431
    readonly property string buildTimestamp: "2026-08-18 02:13:15"
    readonly property string version: "1.0.431"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
