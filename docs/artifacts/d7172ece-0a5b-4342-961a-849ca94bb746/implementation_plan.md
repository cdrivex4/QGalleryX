# MX Player Pro JNI Binding - Iteration 3: JNI Registration Deep Dive

## Problem Identification
Iteration 2 successfully identified an architecture mismatch (64-bit device vs 32-bit workspace). We sideloaded the correct `arm64-v8a` libraries and injected precision tracing.

### Current Findings:
- `i51.o()` (the initialization method) is correctly entered.
- `System.loadLibrary("mxutil")` completes successfully.
- **The Crash:** Immediately after `mxutil` loads, the app crashes with `UnsatisfiedLinkError: No implementation found for canonicalize`. It never reaches the `mxutil loaded` log or the subsequent `mxvp` load. This means the VM attempts to resolve a native method *during* or *immediately after* the library load, and fails.

## Iteration 3: Registration and Symbol Integrity

### Hypothesis:
1.  **Implicit Dependency:** `mxutil` might require `loader.mx` to be loaded first to provide core JNI services.
2.  **Symbol Mismatch:** The sideloaded `mxutil.so` (v1.99.8) might have a different JNI signature than what the v1.68.2 Smali code expects.
3.  **Dynamic Registration Timing:** The `BH.a()` method might be responsible for calling `RegisterNatives`. If the app tries to use a native method before `BH.a()` is finished, it crashes.

### Action Plan:
1.  **Binary Audit:** Run a tool to extract all exported symbols from the sideloaded `libmxutil.so` to verify `canonicalize` exists and matches the expected signature.
2.  **Order Correction:** Modify `i51.o()` to load `loader.mx` first, and potentially move the `BH.a()` call earlier.
3.  **Trace Method Entries:** Inject `Log.d` into the `BH.a()` Smali method itself to see if the native side even begins its work.
4.  **Static Initializer Hunt:** Find which class is calling `Files.canonicalize` so early. The stack trace points to `qq1.j`.

### Verification Steps:
1.  Verify symbol presence in `lib/arm64-v8a/libmxutil.so`.
2.  Deploy with "Safe Load" ordering.
3.  Monitor specialized JNI logs.
