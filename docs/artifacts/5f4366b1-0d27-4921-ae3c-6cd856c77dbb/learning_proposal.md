# Learning Proposal: Always Use Build Script

## Classification
**Rule** — Strict workspace-specific behavioral constraint.

## Rationale
When building the `antigravity` project, I was using raw `cmake --build` commands directly. The project has a dedicated `.\build.ps1` script at `d:\Dev\antigravity\build.ps1` that:
- Kills running instances of **both** `appSamsungGallery.exe` and `appScrollBench.exe` safely
- Cleans autogen folders to prevent stale QML/signal caches
- Runs CMake configure + compile for the full project
- Runs `windeployqt` to deploy Qt dependencies for both apps
- Verifies binary freshness via SHA256 hash comparison
- Runs linkage verification tests

Using raw cmake bypasses all of this, risking stale builds, locked files, and accidentally building only one codebase while leaving the other out of sync.

## Proposed Rule Addition

**Target file:** `d:\Dev\antigravity\.agents\AGENTS.md`

**Text to append:**

```markdown
## Build System Rule

When building the `antigravity` project, you MUST ALWAYS use the build script:
- **Command:** `.\build.ps1`
- **Working directory:** `d:\Dev\antigravity`

NEVER use raw `cmake --build`, `ninja`, or `Start-Process` to build or launch the application directly.
The build script handles killing instances, cleaning autogen, compiling both `appSamsungGallery` and `appScrollBench`, deploying Qt dependencies, and verifying binary freshness.

Bypassing the build script risks stale builds, locked binaries, and codebase divergence.
```
