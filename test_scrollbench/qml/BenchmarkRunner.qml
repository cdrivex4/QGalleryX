import QtQuick
import QtQuick.Controls

Item {
    id: runner
    property bool active: false
    property string targetPath: "I:\\MY SDCArds"
    
    Timer {
        id: autoStartTimer
        interval: 5000 
        running: true
        repeat: false
        onTriggered: start()
    }
    
    signal finished()

    enum State { Idle, Scanning, Waiting, Scrolling, Done }
    property int state: BenchmarkRunner.State.Idle
    property int phase: 0 // 0: CPU Only, 1: HW Accelerated

    Timer {
        id: stateTimer
        interval: 1000
        repeat: false
        onTriggered: {
            if (state === BenchmarkRunner.State.Idle && phase === 1) {
                setupPhase();
            } else {
                processState();
            }
        }
    }

    function start() {
        if (active) return;
        active = true;
        phase = 0;
        setupPhase();
    }

    function setupPhase() {
        state = BenchmarkRunner.State.Scanning;
        if (phase === 0) {
            settings.useDiskCache = false;
            hwAccel.currentMode = 0; // None
            telemetry.startBenchmarking("Phase0_CPU_NoCache");
        } else {
            settings.useDiskCache = true;
            hwAccel.currentMode = 1; // DXVA2 or whatever corresponds to 1
            telemetry.startBenchmarking("Phase1_HW_DiskCache");
        }
        imageModel.clearData();
        imageModel.scanDirectory(targetPath);
        console.log("BENCHMARK Phase " + phase + ": Started scanning " + targetPath);
        stateTimer.restart();
    }

    function processState() {
        switch(state) {
            case BenchmarkRunner.State.Scanning:
                if (imageModel.rowCount() > 0 && imageModel.remainingItems < imageModel.rowCount() && imageModel.pendingDecodeCount === 0) {
                    console.log("BENCHMARK: Scanning phase " + phase + " complete. Viewport loaded. Waiting...");
                    state = BenchmarkRunner.State.Waiting;
                    stateTimer.interval = 5000;
                    stateTimer.restart();
                } else {
                    stateTimer.interval = 1000;
                    stateTimer.restart();
                }
                break;
            
            case BenchmarkRunner.State.Waiting:
                console.log("BENCHMARK Phase " + phase + ": Starting scroll test...");
                state = BenchmarkRunner.State.Scrolling;
                scrollAnimation.from = 0;
                scrollAnimation.to = gridView.contentHeight - gridView.height;
                scrollAnimation.duration = Math.max(5000, (gridView.contentHeight / 1500) * 1000); // 1500px/s
                scrollAnimation.start();
                break;
        }
    }

    property var gridView: standardView.findChildGridView()

    NumberAnimation {
        id: scrollAnimation
        target: runner.gridView
        property: "contentY"
        easing.type: Easing.Linear
        onFinished: {
            console.log("BENCHMARK Phase " + phase + " complete.");
            telemetry.stopBenchmarking();
            if (phase === 0) {
                console.log("BENCHMARK: Preparing Phase 1...");
                phase = 1;
                state = BenchmarkRunner.State.Idle; // Re-trigger via setupPhase check in timer
                stateTimer.interval = 2000;
                stateTimer.restart();
            } else {
                state = BenchmarkRunner.State.Done;
                runner.active = false;
                runner.finished();
            }
        }
    }
    
    // Safety timeout
    Timer {
        interval: 600000 // 10 minutes max
        running: active
        onTriggered: {
            if (active) {
                console.warn("BENCHMARK: Timeout reached. Stopping.");
                telemetry.stopBenchmarking();
                active = false;
            }
        }
    }
}
