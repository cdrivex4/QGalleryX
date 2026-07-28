# QGalleryX Rename — Migration Plan (v2)

> **Baseline commit:** `7c108e0` — safe rollback point on `origin/main`
> **Rollback command:** `git reset --hard 7c108e0` then `git push --force`

---

## Rename Map

| Old Name | New Name |
|---|---|
| `appSamsungGallery` | `QGalleryX` |
| `appSamsungGallery.exe` | `QGalleryX.exe` |
| `appSamsungGalleryTest` | `QGalleryXTest` |
| `appScrollBench` | `QGalleryXBench` |
| `appScrollBench.exe` | `QGalleryXBench.exe` |
| `appScrollBenchNet` | `QGalleryXBenchNet` |
| `appScrollBenchNet.exe` | `QGalleryXBenchNet.exe` |
| `SamsungGallery` (QML URI) | `QGalleryX` |
| `SamsungGalleryTest` (QML URI) | `QGalleryXTest` |
| `Samsung Gallery Clone` (display name) | `QGalleryX` |
| `Dist/SamsungGallery/` (output folder) | `Dist/QGalleryX/` |

---

## Step 1 — Pre-flight Cleanup

Delete these old compiled binaries **before** any code changes so there is zero
ambiguity about which binary is new after the build:

| Delete | Why |
|---|---|
| `Dist/SamsungGallery/` (whole folder) | `deploy.ps1` will create `Dist/QGalleryX/` instead |
| `test_scrollbench/deploy/appScrollBench.exe` | New build outputs `QGalleryXBench.exe` |
| `test_scrollbench/deploy/appScrollBenchNet.exe` | New build outputs `QGalleryXBenchNet.exe` |

> The `build/` directory does NOT need manual cleanup — Step 3 uses `-Clean` which wipes it.

---

## Step 2 — Build-Critical Code Changes

> These files directly affect compilation or QML runtime binding.
> If any of these are wrong, the build will fail or the app will crash on launch.

### `CMakeLists.txt` (root)
- `qt_add_executable(appSamsungGallery` → `qt_add_executable(QGalleryX`
- `qt_add_qml_module(appSamsungGallery` → `qt_add_qml_module(QGalleryX`
- `target_link_libraries(appSamsungGallery` → `target_link_libraries(QGalleryX`
- `target_include_directories(appSamsungGallery` → `target_include_directories(QGalleryX`
- `add_custom_command(TARGET appSamsungGallery` → `add_custom_command(TARGET QGalleryX`
- `$<TARGET_FILE_DIR:appSamsungGallery>` → `$<TARGET_FILE_DIR:QGalleryX>`

### `test_scrollbench/CMakeLists.txt`
- `qt_add_executable(appScrollBench` → `qt_add_executable(QGalleryXBench`
- `set_target_properties(appScrollBench` → `set_target_properties(QGalleryXBench`
- `add_executable(appScrollBenchNet` → `add_executable(QGalleryXBenchNet`
- `set_target_properties(appScrollBenchNet` → `set_target_properties(QGalleryXBenchNet`
- `qt_add_qml_module(appScrollBench` → `qt_add_qml_module(QGalleryXBench`
- `target_link_libraries(appScrollBench` → `target_link_libraries(QGalleryXBench`
- `target_include_directories(appScrollBench` → `target_include_directories(QGalleryXBench`
- `add_custom_command(TARGET appScrollBench` → `add_custom_command(TARGET QGalleryXBench`
- `$<TARGET_FILE_DIR:appScrollBench>` → `$<TARGET_FILE_DIR:QGalleryXBench>`

### `build.ps1`
- `$ExeName = "appSamsungGallery.exe"` → `"QGalleryX.exe"`
- `@("appSamsungGallery", "appSamsungGalleryTest", "appScrollBench")` → `@("QGalleryX", "QGalleryXTest", "QGalleryXBench")`
- `"$BuildDir/appSamsungGallery_autogen"` → `"$BuildDir/QGalleryX_autogen"`
- `$BuildDir/appSamsungGallery.exe` → `$BuildDir/QGalleryX.exe`
- `appScrollBench.exe` (windeployqt lines) → `QGalleryXBench.exe`

### `deploy.ps1` (root)
- `$DIST_DIR = "Dist/SamsungGallery"` → `"Dist/QGalleryX"`
- `"$BUILD_DIR/appSamsungGallery.exe"` → `"$BUILD_DIR/QGalleryX.exe"`
- `"$DIST_DIR/appSamsungGallery.exe"` → `"$DIST_DIR/QGalleryX.exe"`

### `test_scrollbench/build.ps1`
- All `appScrollBench` → `QGalleryXBench`

### `test_scrollbench/scrollbench_build.ps1`
- All `appScrollBench` → `QGalleryXBench`

### `test_scrollbench/deploy.ps1`
- All `appScrollBench` → `QGalleryXBench`

### `src/main.cpp` and `src_legacy/main.cpp`
- QML URI strings: `"SamsungGallery"` → `"QGalleryX"`
- QRC path: `"qrc:/SamsungGallery/..."` → `"qrc:/QGalleryX/..."`

### `src/main_test.cpp` and `src_legacy/main_test.cpp`
- QML URI strings: `"SamsungGalleryTest"` → `"QGalleryXTest"`
- QRC path: `"qrc:/SamsungGalleryTest/..."` → `"qrc:/QGalleryXTest/..."`

### `resources/qml/*.qml` and `resources/qml_legacy/*.qml`
All QML import statements:
- `import SamsungGallery 1.0` → `import QGalleryX 1.0`
- `import SamsungGalleryTest 1.0` → `import QGalleryXTest 1.0`

Files affected: `Main.qml`, `MainSemantic.qml`, `GalleryView.qml`, `GalleryViewTiles.qml`, `GalleryViewSemantic.qml`, `AlbumsView.qml` (×2 for qml and qml_legacy)

### `test_scrollbench/src/main_net_watchdog.cpp`
- All `"appScrollBench.exe"` string literals → `"QGalleryXBench.exe"`

---

## Step 3 — Verification Build

```powershell
.\build.ps1 -Clean
```

**Expected result:**
- `build/QGalleryX.exe` → **FRESH** ✅
- `test_scrollbench/deploy/QGalleryXBench.exe` → **FRESH** ✅
- Module linkage → **VERIFICATION SUCCESSFUL** ✅

If this fails → `git reset --hard 7c108e0` and start over.

---

## Step 4 — Docs Update

| File | What changes |
|---|---|
| `README.md` | Title, exe names, quick start |
| `docs/README.md` | Same |
| `docs/BUILD.md` | Exe name references |
| `docs/AI_RESUME_TICKET.md` | Tidy up |
| `docs/SYSTEM_ARCHITECTURE.md` | Exe name references |
| `docs/SINGLE_EXE_INTEGRATION.md` | Exe name references |
| `docs/THREAD_HIERARCHY.md` | Display name, exe ref |
| `single_exe/README.md` | Exe name references |
| `single_exe/CMakeLists.txt` | Comment lines |
| `test_scrollbench/README.md` | Exe name references |
| `RELEASE_NOTES.md` | Exe references |

---

## Step 5 — Commit & Push

```
feat: rename project to QGalleryX

- appSamsungGallery.exe → QGalleryX.exe
- appScrollBench.exe → QGalleryXBench.exe
- Update all CMakeLists, build/deploy scripts, QML URIs, imports, and docs
```

---

## Step 6 — GitHub Repo Rename (YOU do this in browser)

1. Go to `https://github.com/cdrivex4/antigravity`
2. **Settings** tab → scroll to **Danger Zone**
3. Click **"Rename this repository"**
4. Type `QGalleryX` → confirm

GitHub auto-redirects old URL. Takes 2 minutes.

---

## Step 7 — Update Git Remote (Agy does this)

```powershell
git remote set-url origin https://github.com/cdrivex4/QGalleryX.git
git remote -v  # verify
```

---

## Step 8 — Regenerate Graphify

```powershell
graphify update .
```

---

## Reversion Strategy

At any point before Step 5:
```powershell
git reset --hard 7c108e0
git clean -fd   # removes untracked deletions
```
After Step 5 (already pushed):
```powershell
git revert HEAD
git push
```
