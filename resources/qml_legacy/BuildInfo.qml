pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 483
    readonly property string buildTimestamp: "2026-08-22 01:28:43"
    readonly property string version: "1.0.483"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
