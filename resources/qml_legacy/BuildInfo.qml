pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 424
    readonly property string buildTimestamp: "2026-08-18 00:53:15"
    readonly property string version: "1.0.424"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
