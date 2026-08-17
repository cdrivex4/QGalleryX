pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 432
    readonly property string buildTimestamp: "2026-08-18 02:22:26"
    readonly property string version: "1.0.432"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
