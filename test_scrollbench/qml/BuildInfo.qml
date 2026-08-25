pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 576
    readonly property string buildTimestamp: "2026-08-26 03:01:34"
    readonly property string version: "1.0.576"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
