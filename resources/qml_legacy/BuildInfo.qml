pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 133
    readonly property string buildTimestamp: "2026-07-29 23:42:51"
    readonly property string version: "1.0.133"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
