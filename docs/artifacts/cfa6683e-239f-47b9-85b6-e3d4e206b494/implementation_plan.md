### [Build Script](file:///d:/Dev/antigravity/build.ps1)

#### [MODIFY] [build.ps1](file:///d:/Dev/antigravity/build.ps1)
- **Fix Syntax**: Wrap `Join-Path` in parentheses in the `$CheckFiles` array.
- **Process Killing**: Add `tst_linkage` to the `$pNames` list.
- **Reporting**: Ensure "Binary is FRESH" is clearly reported at the end.

### [AsyncImageProvider](file:///d:/Dev/antigravity/src/AsyncImageProvider.cpp)

#### [MODIFY] [AsyncImageProvider.cpp](file:///d:/Dev/antigravity/src/AsyncImageProvider.cpp)
- **Fix Double Increment**: Remove the `activeWeight += weight` call from `DriveConcurrencyGuard` constructor. Admission is already handled in `processStagedRequests`.
- **Stall Recovery**: 
    - Add a `QTimer` to `AsyncImageProvider` (or use an existing one) to call `checkStalls()` every 5 seconds.
    - Ensure `checkStalls` correctly clears `activeTasksMap`.
- **Throttling**: 
    - Reduce `burst` for network shares from 12 to 8 to be more conservative on slow connections.
    - Increase `stallThreshold` to 30 seconds for network drives to avoid premature recovery on slow but working connections? No, let's keep 15s for now but ensure it's actually running.

## Verification Plan

### Automated Verification
- Run `.\build.ps1` and verify it successfuly kills processes and reports freshness.
- Run `ScrollBench` on a simulated slow network (or check logs) and verify `ActiveWeight` stays within limits (Limit + Burst).
- Verify that `checkStalls` logs `[AsyncImageProvider] STALL DETECTED` if tasks exceed 15s.

### Manual Verification
- Verify UI continues to respond even when loading is slow on the `quake2` network share.

### Automated Verification
- Run `.\build.ps1` with the application open on another machine (if possible) or by manually locking the file (e.g., using `PowerShell`).
- Verify that the script correctly detects the lock and errors out.
- Run `.\build.ps1` normally and verify that it reports "Binary is FRESH" at the end.
