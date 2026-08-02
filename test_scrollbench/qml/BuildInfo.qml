pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 149
    readonly property string buildTimestamp: "2026-08-02 11:49:07"
    readonly property string version: "1.0.149"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
