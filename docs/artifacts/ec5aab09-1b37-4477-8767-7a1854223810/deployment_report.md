# Hermes Deployment & Configuration Report

The Hermes Agent CLI and Hermes Desktop Companion have been successfully deployed and configured on this Windows host to use the networked LM Studio inference endpoint.

## 1. Installed Components

*   **Hermes Agent CLI:**
    *   **Installation Path:** `C:\Users\curtis\AppData\Local\hermes\hermes-agent`
    *   **Virtual Environment:** `C:\Users\curtis\AppData\Local\hermes\hermes-agent\venv`
    *   **Executable:** `C:\Users\curtis\AppData\Local\hermes\hermes-agent\venv\Scripts\hermes.exe`
    *   **User PATH:** Permanent environment variable updated to include the executable path.

*   **Hermes Desktop GUI Companion:**
    *   **Installation Path:** `C:\Users\curtis\AppData\Local\Programs\hermes-desktop`
    *   **Executable:** `C:\Users\curtis\AppData\Local\Programs\hermes-desktop\hermes-agent.exe`
    *   **Version:** v0.5.0 (Latest Release)

## 2. Configuration Details

Both applications share the same configuration directory and files:
*   **Config File Path:** `C:\Users\curtis\AppData\Local\hermes\config.yaml`
*   **Active Provider:** `lmstudio`
*   **Active Base URL:** `http://192.168.1.165:8666/v1`

### Verified Active Configuration Values:
```yaml
provider: lmstudio
base_url: http://192.168.1.165:8666/v1
model:
  provider: lmstudio
  base_url: http://192.168.1.165:8666/v1
  default: z-ai/glm-4.5-air:free
```

## 3. Verification

You can run `hermes` in any terminal to interact with the agent or start the companion desktop application from `Start Menu -> hermes-agent` (or `C:\Users\curtis\AppData\Local\Programs\hermes-desktop\hermes-agent.exe`).
