pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 434
    readonly property string buildTimestamp: "2026-08-18 22:47:24"
    readonly property string version: "1.0.434"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
