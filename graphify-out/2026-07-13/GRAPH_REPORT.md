# Graph Report - .  (2026-07-04)

## Corpus Check
- 10 files · ~42,175 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 234 nodes · 406 edges · 13 communities (10 shown, 3 thin omitted)
- Extraction: 93% EXTRACTED · 7% INFERRED · 0% AMBIGUOUS · INFERRED: 30 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

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

## God Nodes (most connected - your core abstractions)
1. `SettingsHelper` - 64 edges
2. `SettingsHelper` - 42 edges
3. `ImageModel` - 29 edges
4. `AsyncImageProvider::processImageTaskInternal()` - 12 edges
5. `ImageInfo` - 12 edges
6. `QueueGuardState` - 7 edges
7. `getCoalesceKey()` - 6 edges
8. `normalizeId()` - 6 edges
9. `getDiskCachePath()` - 6 edges
10. `AsyncImageProvider::requestImageResponse()` - 6 edges

## Surprising Connections (you probably didn't know these)
- `SettingsHelper::SettingsHelper()` --calls--> `logLevel`  [INFERRED]
  src/SettingsHelper.cpp → src/SettingsHelper.h
- `SettingsHelper::SettingsHelper()` --calls--> `rawAcceleration`  [INFERRED]
  src/SettingsHelper.cpp → src/SettingsHelper.h
- `SettingsHelper::SettingsHelper()` --calls--> `useDiskCache`  [INFERRED]
  src/SettingsHelper.cpp → src/SettingsHelper.h
- `SettingsHelper::SettingsHelper()` --calls--> `videoAcceleration`  [INFERRED]
  src/SettingsHelper.cpp → src/SettingsHelper.h
- `SettingsHelper::SettingsHelper()` --calls--> `logLevel`  [INFERRED]
  src_legacy/SettingsHelper.cpp → src_legacy/SettingsHelper.h

## Import Cycles
- None detected.

## Communities (13 total, 3 thin omitted)

### Community 0 - "Component 0"
Cohesion: 0.08
Nodes (40): atomic, FrameBudgetScheduler, QImage, QQuickImageResponse, QQuickTextureFactory, QSemaphore, QStringList, ResponseTracker (+32 more)

### Community 1 - "Component 1"
Cohesion: 0.06
Nodes (46): QAbstractListModel, QByteArray, QDateTime, QHash, qint64, QList, QModelIndex, QRectF (+38 more)

### Community 2 - "Component 2"
Cohesion: 0.07
Nodes (45): QObject, QString, QVariantMap, Q_INVOKABLE, QSettings, QString, SettingsHelper, cacheSizeMB (+37 more)

### Community 3 - "Component 3"
Cohesion: 0.08
Nodes (26): Q_ENUM, Q_INVOKABLE, QString, SettingsHelper, cacheSizeMBChanged, concurrentThreadsChanged, diskCacheDatabaseTypeChanged, graphicsApiChanged (+18 more)

### Community 4 - "Component 4"
Cohesion: 0.14
Nodes (19): QVariantMap, cacheSizeMB, clearDiskCache, diskCacheDatabaseType, SettingsHelper::getCacheStats(), gridResolution, isApiSupported, restartApp (+11 more)

### Community 5 - "Component 5"
Cohesion: 0.18
Nodes (9): QQuickItem, QSGNode, QSize, QString, FastImageItem::FastImageItem(), FastImageItem::setSource(), FastImageItem::setSourceSize(), FastImageItem::updatePaintNode() (+1 more)

### Community 6 - "Component 6"
Cohesion: 0.22
Nodes (9): QObject, QString, diskCachePath, getDiskCacheLocation, getGpuName, graphicsApi, graphicsDriver, graphicsProfile (+1 more)

### Community 7 - "Component 7"
Cohesion: 0.22
Nodes (9): logLevel, rawAcceleration, setLogLevel, setRawAcceleration, SettingsHelper::SettingsHelper(), setUseDiskCache, setVideoAcceleration, useDiskCache (+1 more)

## Knowledge Gaps
- **58 isolated node(s):** `drive`, `taskId`, `executed`, `filePath`, `fileName` (+53 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **3 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `SettingsHelper` connect `Component 3` to `Component 4`, `Component 6`, `Component 7`, `Component 8`, `Component 9`, `Component 10`?**
  _High betweenness centrality (0.449) - this node is a cross-community bridge._
- **Why does `SettingsHelper` connect `Component 2` to `Component 8`?**
  _High betweenness centrality (0.294) - this node is a cross-community bridge._
- **What connects `drive`, `taskId`, `executed` to the rest of the system?**
  _58 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Component 0` be split into smaller, more focused modules?**
  _Cohesion score 0.0792156862745098 - nodes in this community are weakly interconnected._
- **Should `Component 1` be split into smaller, more focused modules?**
  _Cohesion score 0.05803921568627451 - nodes in this community are weakly interconnected._
- **Should `Component 2` be split into smaller, more focused modules?**
  _Cohesion score 0.07215541165587419 - nodes in this community are weakly interconnected._
- **Should `Component 3` be split into smaller, more focused modules?**
  _Cohesion score 0.07692307692307693 - nodes in this community are weakly interconnected._