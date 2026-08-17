pragma Singleton
import QtQuick

QtObject {
    readonly property int major: 1
    readonly property int minor: 0
    readonly property int build: 422
    readonly property string buildTimestamp: "2026-08-17 23:41:21"
    readonly property string version: "1.0.422"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
