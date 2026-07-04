# YOLO APK Patch Workflow

This document outlines the **YOLO (You Only Look Once) Mode** continuous iteration loop for the Gemini CLI. Since current approaches to fixing the `UnsatisfiedLinkError` and `VerifyError` bugs have hit roadblocks, this workflow grants the CLI permission to autonomously build, deploy, test, and analyze the APK in an uninterrupted loop on the mobile device. 

The CLI MUST NOT wait for user intervention during this loop unless a critical toolchain failure occurs. It will continuously hypothesize, apply patches, and redeploy until the milestones are met.

## Document References (Context)
To maintain context, the CLI should refer to the established project artifacts:
- **Task Tracker:** [task.md](file:///C:/Users/curtis/.gemini/antigravity/brain/d7172ece-0a5b-4342-961a-849ca94bb746/task.md)
- **Patch Plans:** [implementation_plan.md](file:///C:/Users/curtis/.gemini/antigravity/brain/d7172ece-0a5b-4342-961a-849ca94bb746/implementation_plan.md)
- **Previous Findings:** [walkthrough.md](file:///C:/Users/curtis/.gemini/antigravity/brain/d7172ece-0a5b-4342-961a-849ca94bb746/walkthrough.md)

---

## The YOLO Iteration Loop

For each iteration, execute the following steps automatically:

### 1. Hypothesis & Smali Patching
- Analyze the failure from the previous loop.
- Apply the hypothesized Smali fixes in `d:\Dev\apk\work_v5`. 
- Document the attempted fix internally.

### 2. Autonomous Build & Sign
Build with resource preservation (`-r`) to avoid Android 12+ strict resource checks:
```powershell
& "C:\Program Files\Microsoft\jdk-17.0.18.8-hotspot\bin\java.exe" -jar tools\apktool.jar b work_v5 -r -o task_1\v1.68.2_GOD_MODE_YOLO.apk
& "C:\Program Files\Microsoft\jdk-17.0.18.8-hotspot\bin\java.exe" -jar tools\uber-apk-signer.jar -a task_1\v1.68.2_GOD_MODE_YOLO.apk --allowResign --overwrite
```

### 3. Clean Deployment
Always clear the previous installation to prevent Dex/Oat cache poisoning:
```powershell
tools\adb.exe -s adb-RF8M81HFJBR-RXjGOp._adb-tls-connect._tcp uninstall com.mxtech.videoplayer.pro
tools\adb.exe -s adb-RF8M81HFJBR-RXjGOp._adb-tls-connect._tcp install task_1\v1.68.2_GOD_MODE_YOLO.apk
```

### 4. Telemetry & Execution
Clear the log buffer, launch the app, and wait for initialization:
```powershell
tools\adb.exe -s adb-RF8M81HFJBR-RXjGOp._adb-tls-connect._tcp logcat -c
tools\adb.exe -s adb-RF8M81HFJBR-RXjGOp._adb-tls-connect._tcp shell am start -n com.mxtech.videoplayer.pro/.ActivityMediaList
# Wait 10 seconds asynchronously using PowerShell Start-Sleep or tool delays
tools\adb.exe -s adb-RF8M81HFJBR-RXjGOp._adb-tls-connect._tcp logcat -d > yolo_log.txt
```

### 5. Automated Triage
Run the filtering script to isolate faults:
```powershell
python tools\filter_logs.py yolo_log.txt "00:00:00" "23:59:59" --pkg mxtech --summary -o yolo_filtered.txt
```
*If a crash is detected inside the logs, immediately formulate a new hypothesis and trigger Step 1.*

---

## Progress Milestones
The CLI must periodically report progress according to these established milestones to ensure the path to resolution is correct.

*   **[ ] Milestone 1: Clean Installation & Initialization.** The APK builds, signs, and deploys without `INSTALL_FAILED_PACKAGE_CHANGED` or manifest parse errors.
*   **[ ] Milestone 2: JNI Binding Success.** Resolution of the `UnsatisfiedLinkError`. Native libraries (`libmxutil.so`, `libmxvp.so`) load successfully, either through the native initialization chain or via the injected `<clinit>` calls.
*   **[ ] Milestone 3: Stability & Nagware Death.** The application remains open for > 15 seconds. No `ActivityMessenger` overlays, scam alerts, or Dalvik `VerifyError` / `StackOverflowError` exceptions occur.
*   **[ ] Milestone 4: Code Verification.** Confirmed functionality.

## CLI Execution Directive
**You are cleared for YOLO Mode.** Begin the cycle starting from the log analysis of the most recent failure, hypothesize the next fix, and loop through build->deploy->test automatically. Do not pause for human acknowledgment unless you hit a terminal environmental issue (e.g., ADB disconnected).
