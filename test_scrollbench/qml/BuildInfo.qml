pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 428
    readonly property string buildTimestamp: "2026-08-18 01:27:01"
    readonly property string version: "1.0.428"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
