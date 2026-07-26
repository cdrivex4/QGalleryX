pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 124
    readonly property string buildTimestamp: "2026-07-15 00:52:04"
    readonly property string version: "1.0.124"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
