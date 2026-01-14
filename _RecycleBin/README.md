# Recycle Bin

This folder contains files that were removed from the active project but preserved for reference.

## Purpose
- **Backup**: Keep copies of deleted files in case they're needed later
- **History**: Preserve project evolution and decision-making context
- **Clean Repository**: Remove clutter from main project while maintaining traceability

## Structure
```
_RecycleBin/
├── logs/          - Build logs, runtime logs, debug output
├── docs/          - Redundant or outdated documentation
├── code/          - Deprecated source files
└── build_artifacts/ - Accidentally committed build outputs
```

## File Format
Each file includes a header comment with:
- `ORIGINAL LOCATION:` - Where the file was moved from
- `MOVED ON:` - Date of removal
- `REASON:` - Why it was removed

## Restoration
To restore a file:
1. Check the header for original location
2. Copy file back to original path
3. Remove from recycle bin

---

**Created:** 2025-12-30  
**Project:** Antigravity Media Viewer v2.2.0
