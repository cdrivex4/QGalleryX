pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 580
    readonly property string buildTimestamp: "2026-09-04 00:02:35"
    readonly property string version: "1.0.580"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
