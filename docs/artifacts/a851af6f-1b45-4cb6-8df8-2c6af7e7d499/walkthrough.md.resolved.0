# GPU Usage Investigation Walkthrough

I investigated the current GPU usage on your system using `nvidia-smi` and PowerShell performance counters. Here are the findings:

## Current GPU State
- **GPU**: NVIDIA GeForce GTX 1050 Ti
- **Utilization**: ~5% (Stable)
- **Memory**: ~1.1GB / 4.1GB used.

## Identified GPU Consumers
The following processes were identified as active GPU users. Note that in Windows WDDM mode, individual memory usage is often reported as `N/A`, but process types give us clear clues.

| Process Name | PID | Type | Likely Purpose |
| :--- | :--- | :--- | :--- |
| **LM Studio.exe** | 22368 | **C (Compute)** | Machine Learning / AI Model Execution |
| **Antigravity.exe** | 2136 | C+G | IDE Interface HW Acceleration |
| **chrome.exe** | 28416 | C+G | Browser HW Acceleration |
| **firefox.exe** | 5688 | C+G | Browser HW Acceleration |
| **SearchApp.exe** | 12324 | C+G | Windows Search UI |

### Primary Suspect for Spikes
**LM Studio** is the most likely culprit for any significant GPU spikes as it is running a dedicated **Compute** workload. If you were recently running a model or generating text, this would explain the usage.

## How to Check Yourself
You can run these commands in your terminal to see real-time status:

1.  **Quick Snapshot**: `nvidia-smi`
2.  **Detailed Process List**: `nvidia-smi -q`
3.  **Monitor in real-time**: `nvidia-smi -l 1` (Press Ctrl+C to stop)

No driver errors or resets were found in the recent system logs, suggesting the spikes are due to normal application workloads.
