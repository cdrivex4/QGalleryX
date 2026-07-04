pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 104
    readonly property string buildTimestamp: "2026-07-04 16:08:33"
    readonly property string version: "1.0.104"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
