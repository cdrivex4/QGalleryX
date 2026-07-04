# MX Player Pro v1.68.2 — Nagware Patch Walkthrough

## Problem
MX Player Pro v1.68.2 (from liteapks) contains **nagware overlays** that display scam alerts and launcher redirects. The goal: remove all nagware while keeping the app fully functional.

## Key Findings

### ❌ What Didn't Work
1. **PendingIntent patching** — Caused `VerifyError`, `StackOverflowError`, and `IllegalArgumentException` crashes. This was a **red herring** — these patches were never needed.
2. **Injecting `loadLibrary` into `i51.o()`** — The nagware patches short-circuited the app's init flow, so `i51.o()` was never called. The native libraries were never loaded.

### ✅ What Worked: Static Class Initializers

> [!IMPORTANT]
> When patching nagware that modifies the app's initialization flow (e.g., short-circuiting `App.P()`, `App.p()`), you **MUST** ensure that native library loading is not bypassed. The original app loads libraries via its custom init chain — if you break that chain, JNI methods will crash with `UnsatisfiedLinkError`.

**The fix**: Inject `System.loadLibrary()` calls directly into the `<clinit>` (static class initializer) of every class that declares `native` methods. This way, the library is loaded when the class is first referenced, regardless of the app's init flow.

### Files Modified

| File | Change |
|------|--------|
| `Files.smali` | Added new `<clinit>` loading `c++_shared`, `mxutil`, `mxvp` |
| `ImmutableMediaDirectory.smali` | Prepended library loads before `nativeClassInit()` |
| `MediaDirectory.smali` | Prepended library loads before `nativeClassInit()` |
| `e.smali` | `return-void` in `y()` — Scam Alert bypass |
| `App.smali` | `return-void` in `P()` — Launcher neutralization |
| `App.smali` | `return true` in `p()` — Force licensed state |
| `i51.smali` | `return true` in `p()` — Force licensed state (parent) |
| `ActivityMessenger.smali` | `finish()` in `onCreate` — Overlay kill |

### Build Command
```
apktool b work_v5 -r -o output.apk
```
The `-r` flag preserves original resources/manifest for Android 12+ compatibility.

## Verification
- `filter_logs.py` script created at `tools/filter_logs.py` for time-windowed log analysis
- Logs confirmed crash was `UnsatisfiedLinkError` for `Files.canonicalize`, `ImmutableMediaDirectory.nativeClassInit`, `MediaDirectory.nativeClassInit`
- Root cause: `i51.o()` (which loads `c++_shared`) was never reached due to nagware patches
