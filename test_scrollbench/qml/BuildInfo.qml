pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 532
    readonly property string buildTimestamp: "2026-08-24 03:26:36"
    readonly property string version: "1.0.532"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
