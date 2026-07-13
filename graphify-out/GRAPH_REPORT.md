# Graph Report - antigravity  (2026-07-13)

## Corpus Check
- 88 files · ~45,577 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1559 nodes · 2635 edges · 88 communities (73 shown, 15 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 150 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `32237270`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- [[_COMMUNITY_Component 0|Component 0]]
- [[_COMMUNITY_Component 1|Component 1]]
- [[_COMMUNITY_Component 2|Component 2]]
- [[_COMMUNITY_Component 3|Component 3]]
- [[_COMMUNITY_Component 4|Component 4]]
- [[_COMMUNITY_Component 5|Component 5]]
- [[_COMMUNITY_Component 6|Component 6]]
- [[_COMMUNITY_Component 7|Component 7]]
- [[_COMMUNITY_Component 8|Component 8]]
- [[_COMMUNITY_Component 9|Component 9]]
- [[_COMMUNITY_Component 10|Component 10]]
- [[_COMMUNITY_Component 11|Component 11]]
- [[_COMMUNITY_Component 12|Component 12]]
- [[_COMMUNITY_Community 13|Community 13]]
- [[_COMMUNITY_Community 14|Community 14]]
- [[_COMMUNITY_Community 15|Community 15]]
- [[_COMMUNITY_Community 16|Community 16]]
- [[_COMMUNITY_Community 17|Community 17]]
- [[_COMMUNITY_Community 18|Community 18]]
- [[_COMMUNITY_Community 19|Community 19]]
- [[_COMMUNITY_Community 20|Community 20]]
- [[_COMMUNITY_Community 21|Community 21]]
- [[_COMMUNITY_Community 22|Community 22]]
- [[_COMMUNITY_Community 23|Community 23]]
- [[_COMMUNITY_Community 24|Community 24]]
- [[_COMMUNITY_Community 25|Community 25]]
- [[_COMMUNITY_Community 26|Community 26]]
- [[_COMMUNITY_Community 27|Community 27]]
- [[_COMMUNITY_Community 28|Community 28]]
- [[_COMMUNITY_Community 29|Community 29]]
- [[_COMMUNITY_Community 30|Community 30]]
- [[_COMMUNITY_Community 31|Community 31]]
- [[_COMMUNITY_Community 32|Community 32]]
- [[_COMMUNITY_Community 33|Community 33]]
- [[_COMMUNITY_Community 34|Community 34]]
- [[_COMMUNITY_Community 35|Community 35]]
- [[_COMMUNITY_Community 36|Community 36]]
- [[_COMMUNITY_Community 37|Community 37]]
- [[_COMMUNITY_Community 38|Community 38]]
- [[_COMMUNITY_Community 39|Community 39]]
- [[_COMMUNITY_Community 41|Community 41]]
- [[_COMMUNITY_Community 43|Community 43]]
- [[_COMMUNITY_Community 45|Community 45]]
- [[_COMMUNITY_Community 46|Community 46]]
- [[_COMMUNITY_Community 47|Community 47]]
- [[_COMMUNITY_Community 48|Community 48]]
- [[_COMMUNITY_Community 49|Community 49]]
- [[_COMMUNITY_Community 50|Community 50]]
- [[_COMMUNITY_Community 51|Community 51]]
- [[_COMMUNITY_Community 52|Community 52]]
- [[_COMMUNITY_Community 53|Community 53]]
- [[_COMMUNITY_Community 54|Community 54]]
- [[_COMMUNITY_Community 55|Community 55]]
- [[_COMMUNITY_Community 56|Community 56]]
- [[_COMMUNITY_Community 57|Community 57]]
- [[_COMMUNITY_Community 58|Community 58]]
- [[_COMMUNITY_Community 59|Community 59]]
- [[_COMMUNITY_Community 60|Community 60]]
- [[_COMMUNITY_Community 61|Community 61]]
- [[_COMMUNITY_Community 62|Community 62]]
- [[_COMMUNITY_Community 63|Community 63]]
- [[_COMMUNITY_Community 64|Community 64]]
- [[_COMMUNITY_Community 65|Community 65]]
- [[_COMMUNITY_Community 66|Community 66]]
- [[_COMMUNITY_Community 67|Community 67]]
- [[_COMMUNITY_Community 68|Community 68]]
- [[_COMMUNITY_Community 69|Community 69]]
- [[_COMMUNITY_Community 70|Community 70]]
- [[_COMMUNITY_Community 71|Community 71]]
- [[_COMMUNITY_Community 72|Community 72]]
- [[_COMMUNITY_Community 73|Community 73]]
- [[_COMMUNITY_Community 74|Community 74]]
- [[_COMMUNITY_Community 75|Community 75]]
- [[_COMMUNITY_Community 76|Community 76]]
- [[_COMMUNITY_Community 77|Community 77]]
- [[_COMMUNITY_Community 78|Community 78]]

## God Nodes (most connected - your core abstractions)
1. `ScrollBenchImageModel` - 100 edges
2. `DiagnosticsMonitor` - 84 edges
3. `TelemetryMonitor` - 77 edges
4. `SettingsHelper` - 64 edges
5. `AsyncImageProvider` - 53 edges
6. `SettingsHelper` - 45 edges
7. `SystemMonitor` - 43 edges
8. `ImageModel` - 42 edges
9. `TaskScheduler` - 42 edges
10. `LogManager` - 36 edges

## Surprising Connections (you probably didn't know these)
- `AsyncImageProvider::AsyncImageProvider()` --references--> `QTimer`  [INFERRED]
  src/AsyncImageProvider.cpp → test_scrollbench/src/ScrollBenchImageModel.h
- `FrameBudgetScheduler::FrameBudgetScheduler()` --references--> `QTimer`  [INFERRED]
  src/FrameBudgetScheduler.cpp → test_scrollbench/src/ScrollBenchImageModel.h
- `messageHandler` --calls--> `instance`  [INFERRED]
  src_legacy/LogManager.h → src/SystemMonitor.h
- `LogManager` --references--> `mutex`  [EXTRACTED]
  src_legacy/LogManager.h → src/AsyncImageProvider.h
- `scheduleStagingProcessing` --references--> `QTimer`  [EXTRACTED]
  src/AsyncImageProvider.h → test_scrollbench/src/ScrollBenchImageModel.h

## Import Cycles
- None detected.

## Communities (88 total, 15 thin omitted)

### Community 0 - "Component 0"
Cohesion: 0.16
Nodes (24): QSemaphore, abortIfNotNeeded, checkStalls, clearCache, deliverToPending, AsyncImageProvider::DriveStats::update(), getDriveRoot, getTaskWeight (+16 more)

### Community 1 - "Component 1"
Cohesion: 0.03
Nodes (49): ImageItem, FrameBudgetScheduler, atomic, Q_INVOKABLE, QSet, QString, QVector, ImageProcessor (+41 more)

### Community 2 - "Component 2"
Cohesion: 0.07
Nodes (48): QObject, QString, QVariantMap, Q_INVOKABLE, QSettings, QString, SettingsHelper, cacheSizeMB (+40 more)

### Community 3 - "Component 3"
Cohesion: 0.06
Nodes (43): Priority, Task, TaskType, atomic, Priority, QList, QMap, QMutex (+35 more)

### Community 4 - "Component 4"
Cohesion: 0.12
Nodes (16): AsyncImageResponse, cancel, m_cancelled, m_id, m_image, m_requestedSize, m_tracker, m_workDuration (+8 more)

### Community 5 - "Component 5"
Cohesion: 0.06
Nodes (67): QObject, QString, QVariantMap, Q_ENUM, Q_INVOKABLE, QString, SettingsHelper, cacheSizeMB (+59 more)

### Community 6 - "Component 6"
Cohesion: 0.05
Nodes (55): QByteArray, QHash, qint64, QList, QModelIndex, QObject, QRectF, QString (+47 more)

### Community 7 - "Component 7"
Cohesion: 0.15
Nodes (23): QByteArray, qint64, QList, QSize, QString, clearCache, getCachedData, getCachedPath (+15 more)

### Community 8 - "Component 8"
Cohesion: 0.09
Nodes (29): qint64, QObject, QRectF, QStringList, cancelPendingRequests, cancelScan, clearData, clearSelection (+21 more)

### Community 9 - "Component 9"
Cohesion: 0.06
Nodes (46): AVBufferRef, AVPixelFormat, HWAccel, QObject, QString, detectCPUFeatures(), AVBufferRef, AVPixelFormat (+38 more)

### Community 10 - "Component 10"
Cohesion: 0.32
Nodes (14): QObject, QString, addCritical, addWarning, checkAdaptiveIO, checkLoadProgress, checkSettings, checkViewportCulling (+6 more)

### Community 11 - "Component 11"
Cohesion: 0.08
Nodes (23): Q_ENUM, Q_INVOKABLE, QList, ImageModel, clearSelection, filterQueryChanged, getProxyIndexForSourceIndex, isLoadingChanged (+15 more)

### Community 12 - "Component 12"
Cohesion: 0.13
Nodes (15): clearDiskCache, FileCacheManager, initialize, m_canWrite, m_db, m_dbPath, m_dirty, m_maintenanceTimer (+7 more)

### Community 13 - "Community 13"
Cohesion: 0.18
Nodes (10): qint64, QString, ImageInfo, date, dateModified, dateTaken, fileName, filePath (+2 more)

### Community 14 - "Community 14"
Cohesion: 0.16
Nodes (14): quint64, quint64, MmapCacheDatabase, advanceHead, clear, clearInternal, m_capacity, m_file (+6 more)

### Community 15 - "Community 15"
Cohesion: 0.18
Nodes (17): QModelIndex, QObject, QString, QVariant, QVariantMap, applyFilter, data, ImageModel::getMetadata() (+9 more)

### Community 16 - "Community 16"
Cohesion: 0.12
Nodes (20): CacheEntry, fileSizeBytes, lastAccessed, originalPath, sizeKey, thumbPath, QHash, qint64 (+12 more)

### Community 17 - "Community 17"
Cohesion: 0.24
Nodes (10): QQuickAsyncImageProvider, QRunnable, atomic, QCache, QMap, QMutex, QSize, QQueue (+2 more)

### Community 18 - "Community 18"
Cohesion: 0.18
Nodes (10): ICacheDatabase, clear, contains, get, getOldestKeys, insert, load, remove (+2 more)

### Community 19 - "Community 19"
Cohesion: 0.07
Nodes (29): QQuickItem, QSGNode, QSGTexture, QQuickItem, QSize, QString, FastImageItem, FastImageItem::FastImageItem() (+21 more)

### Community 20 - "Community 20"
Cohesion: 0.07
Nodes (28): DriveStats, AsyncImageProvider, getActiveTaskIds, getCacheStats, m_cache, m_driveStats, m_driveStatsMutex, m_mutex (+20 more)

### Community 21 - "Community 21"
Cohesion: 0.50
Nodes (4): applyFilter, resortItems, setFilterQuery, setSortMode

### Community 22 - "Community 22"
Cohesion: 0.18
Nodes (12): EXCEPTION_POINTERS, LONG, AsyncImageProvider::AsyncImageProvider(), QElapsedTimer, QMessageLogContext, QString, QtMsgType, crashHandler() (+4 more)

### Community 23 - "Community 23"
Cohesion: 0.04
Nodes (46): DiagnosticsMonitor, CRITICAL_STALL_THRESHOLD_MS, criticalIssueDetected, criticalsChanged, diskCacheChanged, EXPECTED_MIN_RANGE_SIZE, healthChanged, ioStatusChanged (+38 more)

### Community 24 - "Community 24"
Cohesion: 0.04
Nodes (40): QElapsedTimer, QString, QVector, TelemetryMonitor, averageFpsChanged, cacheHitRateChanged, completionsThisFrameChanged, delegateCountChanged (+32 more)

### Community 25 - "Community 25"
Cohesion: 0.22
Nodes (9): QModelIndex, QString, QVariant, QVariantMap, data, deleteSelected, generateTestData, ScrollBenchImageModel::getMetadata() (+1 more)

### Community 26 - "Community 26"
Cohesion: 0.06
Nodes (42): DWORD, FILETIME, QObject, QString, SIZE_T, fileTimeToInt64(), QString, _PROCESS_MEMORY_COUNTERS (+34 more)

### Community 27 - "Community 27"
Cohesion: 0.67
Nodes (3): QByteArray, QHash, roleNames

### Community 28 - "Community 28"
Cohesion: 0.67
Nodes (3): QByteArray, QHash, roleNames

### Community 29 - "Community 29"
Cohesion: 0.08
Nodes (40): QQuickImageResponse, AsyncImageProvider, clearCache, getCachedImage, getCacheStats, insertCachedImage, m_cache, m_mutex (+32 more)

### Community 30 - "Community 30"
Cohesion: 0.06
Nodes (39): QMessageLogContext, QString, QStringList, QtMsgType, atomic, condition_variable, LogEntry, Q_INVOKABLE (+31 more)

### Community 31 - "Community 31"
Cohesion: 0.08
Nodes (39): QAbstractListModel, QByteArray, QHash, QModelIndex, QObject, QString, QVariant, QVariantList (+31 more)

### Community 32 - "Community 32"
Cohesion: 0.07
Nodes (37): mutex, QMessageLogContext, QString, QStringList, QtMsgType, atomic, LogEntry, Q_INVOKABLE (+29 more)

### Community 33 - "Community 33"
Cohesion: 0.08
Nodes (38): QAbstractListModel, QByteArray, QHash, QModelIndex, QObject, QString, QVariant, QVariantList (+30 more)

### Community 34 - "Community 34"
Cohesion: 0.07
Nodes (33): AlbumInfo, count, coverPaths, name, path, AlbumModel, AlbumModel::AlbumModel(), applyFilter (+25 more)

### Community 35 - "Community 35"
Cohesion: 0.07
Nodes (32): AlbumInfo, count, coverPaths, name, path, AlbumModel, AlbumModel::AlbumModel(), applyFilterFromPaths (+24 more)

### Community 36 - "Community 36"
Cohesion: 0.07
Nodes (32): Priority, Task, TaskType, atomic, Priority, Q_OBJECT, QMap, QMutex (+24 more)

### Community 37 - "Community 37"
Cohesion: 0.11
Nodes (29): DWORDLONG, HANDLE, QObject, QString, QVector, FastVolumeScanner, buildPaths, enumerateFiles (+21 more)

### Community 38 - "Community 38"
Cohesion: 0.08
Nodes (25): function, QObject, FrameBudgetScheduler, checkFrameBoundary, completionsThisFrameChanged, enabledChanged, frameBudgetChanged, FrameBudgetScheduler::FrameBudgetScheduler() (+17 more)

### Community 39 - "Community 39"
Cohesion: 0.08
Nodes (23): function, QObject, FrameBudgetScheduler, checkFrameBoundary, completionsThisFrameChanged, enabledChanged, frameBudgetChanged, FrameBudgetScheduler::FrameBudgetScheduler() (+15 more)

### Community 41 - "Community 41"
Cohesion: 0.09
Nodes (15): QString, SystemMonitor, m_availableSystemMemoryMB, m_cpuUsage, m_gpuName, m_gpuUsage, m_gpuVramTotalMB, m_gpuVramUsedMB (+7 more)

### Community 43 - "Community 43"
Cohesion: 0.11
Nodes (18): atomic, AVCodecContext, QElapsedTimer, QImage, QSize, QString, QVariantMap, va_list (+10 more)

### Community 45 - "Community 45"
Cohesion: 0.18
Nodes (17): FileType, QObject, QString, QStringList, QVariantMap, DesktopHelper, copyFiles, DesktopHelper::DesktopHelper() (+9 more)

### Community 46 - "Community 46"
Cohesion: 0.14
Nodes (13): string, dumpCrashLog(), main(), function, Q_OBJECT, QString, MatrixRunner, m_targetFolder (+5 more)

### Community 47 - "Community 47"
Cohesion: 0.16
Nodes (17): QObject, averageFps, logStats, recordCacheHit, recordCacheMiss, recordFrame, reportLoadTime, reportWorkDuration (+9 more)

### Community 48 - "Community 48"
Cohesion: 0.22
Nodes (16): getCachedImage, insertCachedImage, processImageTask, processImageTaskInternal, queueRequest, requestImageResponse, handleDone, atomic (+8 more)

### Community 49 - "Community 49"
Cohesion: 0.23
Nodes (14): ProcessorType, QString, FileTypeRouter, getProcessorForExtension, isRaw, isStandardImage, isVideo, s_qtNativeFormats (+6 more)

### Community 50 - "Community 50"
Cohesion: 0.29
Nodes (6): QList, QFile, QDateTime, condition_variable, QTextStream, thread

### Community 51 - "Community 51"
Cohesion: 0.23
Nodes (13): FileType, QObject, QString, DesktopHelper, DesktopHelper::DesktopHelper(), getFileType, openInExplorer, pauseBackgroundTasks (+5 more)

### Community 52 - "Community 52"
Cohesion: 0.24
Nodes (3): QAbstractListModel, QVector, QPointer

### Community 53 - "Community 53"
Cohesion: 0.15
Nodes (13): DWORD, SIZE_T, _PROCESS_MEMORY_COUNTERS, cb, PageFaultCount, PagefileUsage, PeakPagefileUsage, PeakWorkingSetSize (+5 more)

### Community 54 - "Community 54"
Cohesion: 0.15
Nodes (13): DWORD, SIZE_T, _PROCESS_MEMORY_COUNTERS, cb, PageFaultCount, PagefileUsage, PeakPagefileUsage, PeakWorkingSetSize (+5 more)

### Community 55 - "Community 55"
Cohesion: 0.21
Nodes (10): QObject, QSize, QString, Q_OBJECT, ImageProcessor, imageProcessingError, ImageProcessor::ImageProcessor(), public (+2 more)

### Community 56 - "Community 56"
Cohesion: 0.27
Nodes (6): QMutex, QSet, QString, VisibleRangeManager, m_mutex, m_visiblePaths

### Community 57 - "Community 57"
Cohesion: 0.25
Nodes (10): FILETIME, QObject, fileTimeToInt64(), getCpuUsage, getMemoryUsageMB, getSystemCpuUsage, logEnvironmentSnapshot, startMonitoring (+2 more)

### Community 58 - "Community 58"
Cohesion: 0.31
Nodes (4): QObject, QSettings, Q_OBJECT, TestImageModel

### Community 59 - "Community 59"
Cohesion: 0.31
Nodes (3): QString, QImage, main()

### Community 60 - "Community 60"
Cohesion: 0.22
Nodes (9): cpuUsageChanged, getGpuUsage, gpuUsageChanged, gpuVramTotalMBChanged, gpuVramUsedMBChanged, memoryUsageChanged, systemCpuUsageChanged, systemMemoryChanged (+1 more)

### Community 62 - "Community 62"
Cohesion: 0.62
Nodes (6): Build-Architecture(), Test-QtStatic(), Write-Error-Custom(), Write-Header(), Write-Step(), Write-Success()

### Community 64 - "Community 64"
Cohesion: 0.70
Nodes (4): Assert-DiskSpace(), Log-Me(), Run-Exec(), Setup-Module()

### Community 65 - "Community 65"
Cohesion: 0.50
Nodes (3): AsyncImageProvider, ScrollBenchImageModel, SettingsHelper

## Knowledge Gaps
- **502 isolated node(s):** `name`, `path`, `coverPaths`, `count`, `public` (+497 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **15 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `QTimer` connect `Community 22` to `Component 0`, `Community 65`, `Component 1`, `Community 38`, `Community 39`, `Component 8`, `Community 41`, `Component 12`, `Community 46`, `Community 47`, `Community 17`, `Community 50`, `Community 52`, `Community 23`, `Community 24`, `Community 26`, `Community 29`?**
  _High betweenness centrality (0.137) - this node is a cross-community bridge._
- **Why does `ScrollBenchImageModel` connect `Component 1` to `Component 8`, `Community 75`, `Community 76`, `Community 49`, `Community 52`, `Community 21`, `Community 22`, `Community 25`, `Community 28`?**
  _High betweenness centrality (0.118) - this node is a cross-community bridge._
- **Why does `TelemetryMonitor` connect `Community 24` to `Community 66`, `Community 68`, `Community 77`, `Community 78`, `Community 47`, `Community 22`, `Community 58`?**
  _High betweenness centrality (0.087) - this node is a cross-community bridge._
- **What connects `name`, `path`, `coverPaths` to the rest of the system?**
  _502 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Component 1` be split into smaller, more focused modules?**
  _Cohesion score 0.031746031746031744 - nodes in this community are weakly interconnected._
- **Should `Component 2` be split into smaller, more focused modules?**
  _Cohesion score 0.06857142857142857 - nodes in this community are weakly interconnected._
- **Should `Component 3` be split into smaller, more focused modules?**
  _Cohesion score 0.06363636363636363 - nodes in this community are weakly interconnected._