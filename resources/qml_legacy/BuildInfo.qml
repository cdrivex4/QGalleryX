pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 436
    readonly property string buildTimestamp: "2026-08-18 23:19:53"
    readonly property string version: "1.0.436"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
