pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 145
    readonly property string buildTimestamp: "2026-08-02 10:47:29"
    readonly property string version: "1.0.145"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
