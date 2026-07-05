# AsyncImageProvider Pipeline Audit

## Why Are We Still Finding These?

The honest answer: the pipeline was built incrementally under real-world testing conditions, and several "safety" mechanisms were added as one-off fixes without a full mental model of the entire coalescing + cancellation + caching chain. Each individual guard made local sense, but they created **cross-cutting assumptions** about what state the system is in when they fire.

---

## The Root Cause Pattern

Every dead thumbnail we have found follows the same shape:

```
A task is started for path X at size S
  → Something interrupts or aborts it mid-flight
     → The task returns NULL or an aborted result
        → The deliver() path at the end generates a PLACEHOLDER
           → The PLACEHOLDER is inserted into RAM/disk cache
              → Future requests for X@S get the cached placeholder forever
```

The placeholder caching is **intentional** for genuinely broken/missing files. The problem is when it fires on **transient** conditions (low memory, high load, delegate recycled, resolution change) that look identical to a broken file from the deliver() path's perspective.

---

## Bugs Found & Fixed This Session

| # | Bug | Root Cause | Fixed? |
|---|-----|-----------|--------|
| 1 | LIFO abort threshold | Hard limit of `activeTaskCount > 5000` pruned the newest/most visible tasks first | ✅ |
| 2 | OOM race with VRM | Low-memory path delivered `QImage()` to pending → placeholder cached permanently | ✅ |
| 3 | DriveConcurrencyGuard double-decrement | Stall timer evicted task, then destructor decremented weight again → cascading I/O flood | ✅ |
| 4 | VRM 50ms race (stalled pending) | VRM update lagged behind delegate creation; tasks parked in staging with no pump to wake them | ✅ |
| 5 | False positive stall detection | DiagnosticsMonitor counted staged+parked items as "stuck" when 0 active tasks | ✅ |
| 6 | Speculative FFmpeg/LibRaw abort → cached placeholder | Offscreen speculative abort returned NULL → placeholder cached at that size permanently | ✅ |
| 7 | FFmpeg cancellation token cross-delegate poisoning | Video task aborted via `c.get()` (the *first* requester's token) even if *other* delegates were still waiting for the result | ✅ |

---

## REMAINING RISKS (not yet fixed)

### 🔴 HIGH: Placeholder cache poisoning is still possible on genuine I/O failure mid-resize

**Scenario:** User sets grid to small size → system loads small thumbnails (cached at `200x200`). User then `ctrl+scroll` to large → system requests same file at `400x400`. If ANY decode attempt at `400x400` returns `null` for a transient reason (e.g. disk hiccup, timeout), a placeholder is cached at `400x400`. Small thumbs still work, large are dead forever until app restart.

**Root fix needed:** The `deliver:` label path (line 987) should NOT cache placeholders on transient errors. It should only cache placeholders for files that are truly unreadable (i.e., `!QFile::exists(realPath)` or `readerError` is a permanent error class). For transient errors, it should deliver `QImage()` WITHOUT caching, so the next request retries.

---

### 🔴 HIGH: `setSourceSize` does NOT re-request when size changes

**Scenario:** `FastImageItem::setSourceSize` is called but currently only calls `update()`. If the grid cell size changes (slider/ctrl+scroll), the QML binding sets a new `sourceSize`. But:
1. The old image (wrong size) is already in `m_image`.
2. `setSource` is **not** called again because `m_source` didn't change.
3. `setSourceSize` doesn't cancel+re-request.
4. The delegate renders the old-size image stretched.

**Root fix needed:** `setSourceSize` must cancel any pending `m_response` and re-request via `setSource` if we already have a loaded image OR a pending request at a different size.

---

### 🟡 MEDIUM: `getCachedImage` key ignores `QUrl` vs raw path normalization

**Scenario:** RAM cache is keyed as `id + "_" + WxH`. But `normalizeId()` strips `file://` and lowercases the drive letter. If a request arrives from `FastImageItem` with a pre-normalized path but `requestImageResponse` arrives with a raw path, they hit different cache keys → duplicate decode.

This isn't causing dead thumbnails but wastes RAM and CPU, especially on ctrl+scroll where many new sizes are requested simultaneously.

---

### 🟡 MEDIUM: Disk cache is NOT invalidated on resolution change

The disk cache key includes `lastModified` but **not** `requestedSize` in its file path derivation correctly. Check `getDiskCachePath`: the hash includes size but the function was written before the `insertCachedImage` key format was standardized. Worth verifying they are consistent so a 200x200 disk cache entry never serves a 400x400 request.

---

### 🟡 MEDIUM: `isRequestStillNeeded` can return `false` during coalesce window

During `processStagedRequests`, we call `isRequestStillNeeded(cKey)` which checks if any tracker has a live `response` that isn't cancelled. But if the delegate was recycled by QML (fast scroll), the `AsyncImageResponse` is deleted, `m_tracker->response = nullptr` is set, and this returns `false` — even if a brand new delegate for the SAME file was just created but hasn't been added to `m_pendingResponses` yet (the 2ms `singleShot` delay in `scheduleStagingProcessing`). The task is then silently dropped.

---

### 🟢 LOW: No retry for permanently-staged items

If a staged item is re-queued due to `lowMemory + offscreen`, but VRM never updates to make it visible (e.g. the user closed the folder), the item sits in `m_stagedRequests` forever. It won't cause a dead thumbnail — it just leaks staging memory. On a 100k folder scroll, this could accumulate significant staging queue bloat.

---

## Recommended Fix Order

1. **[CRITICAL - fixes the reported bug]** `setSourceSize` must trigger re-request
2. **[CRITICAL - prevents placeholder poisoning]** `deliver:` should not cache placeholders for transient I/O errors
3. **[MEDIUM]** Verify disk cache size key consistency
4. **[MEDIUM]** Add staging queue expiry (age-out items older than N seconds)
5. **[LOW]** Normalize cache key before all RAM cache lookups

