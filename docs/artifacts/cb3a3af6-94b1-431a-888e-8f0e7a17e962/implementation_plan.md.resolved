# Performance Optimization for Weak Hardware

## User Review Required
None.

## Proposed Changes

### [C++] Video Task Throttling
#### [MODIFY] [VideoThumbnailer.cpp](file:///d:/Dev/antigravity/src/VideoThumbnailer.cpp)
- **Reduce FFmpeg Log Spam**: Change line 61 from `qWarning()` to only log once per session
- **Add semaphore limit**: Max 2 concurrent video decode tasks (currently unlimited)

#### [MODIFY] [AsyncImageProvider.cpp](file:///d:/Dev/antigravity/src/AsyncImageProvider.cpp)
- **Lower video task priority**: Change video thumbnails from `Normal` to `Low` priority
- **Add yield points**: Insert `QThread::msleep(5)` every 10 tasks to let UI breathe

### [C++] Thread Pool Tuning
#### [MODIFY] [TaskScheduler.cpp](file:///d:/Dev/antigravity/src/TaskScheduler.cpp)
- **Reduce default threads**: Change from 4 to `max(2, cores/2)` for weak hardware
- **Add task queue limit**: Drop `Low` priority tasks if queue > 500 items

### [QML] Settings Defaults
#### [MODIFY] [SettingsHelper.cpp](file:///d:/Dev/antigravity/src/SettingsHelper.cpp)
- **Lower default threads**: Change from 4 to 2 threads
- **Lower default cache**: Change from 512MB to 256MB

## Verification Plan
### Automated Tests
- Build success

### Manual Verification
- Run on weak hardware over network
- Verify UI remains responsive during folder scan
- Check log spam is reduced
