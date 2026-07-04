# Task: Debugging UI Performance and Concurrency Leak

- [x] Research concurrency management in `AsyncImageProvider.cpp` <!-- id: 0 -->
- [x] Research UI thread blockage and `DiagnosticsMonitor` impact <!-- id: 1 -->
- [x] Create Implementation Plan <!-- id: 2 -->
- [x] Fix Adaptive I/O admission control leak and implement task weighting <!-- id: 3 -->
- [x] Optimize GUI thread interaction (throttling updates and counters) <!-- id: 4 -->
- [x] Verify fix with performance logs <!-- id: 5 -->
- [x] Improve build script robustness <!-- id: 6 -->
    - [x] Implement file lock detection and process cleanup in `build.ps1` <!-- id: 7 -->
    - [x] Implement post-build hash verification and reporting in `build.ps1` <!-- id: 8 -->
- [x] Resolve slow-network performance regression (2 FPS) <!-- id: 9 -->
    - [x] Investigate stall recovery (`checkStalls`) and weight leakage <!-- id: 10 -->
    - [x] Tighten network concurrency limits for high-latency shares <!-- id: 11 -->
    - [x] Optimize UI thread signaling frequency during heavy stalls <!-- id: 12 -->
