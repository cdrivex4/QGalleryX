# Static Qt Build Information

## Is Qt Closed Source?
No. Qt is open-source (LGPLv3/GPLv3). The source code is freely available.

## Why Build from Source?
The standard Qt Installer provides **Dynamic Libraries** (DLLs). This mandates that your application be accompanied by a folder of DLLs.
To create a **Single Executable**, you need **Static Libraries** (`.lib` or `.a` files). The Qt Company generally expects developers to compile these themselves if needed.

## Trade-offs of Static Build

| Feature | Dynamic (Standard) | Static (Custom Build) |
|---------|-------------------|----------------------|
| **Executable Size** | Small (needs huge folder) | Large (One file) |
| **Startup Time** | Fast | Fast |
| **Upgrade path** | Easy (Update DLLs) | Hard (Recompile App) |
| **Licensing** | Easy (Standard LGPL) | Complex (LGPL details or Commercial) |

## How to Build Static Qt
1.  Download Qt Source Code (via Maintenance Tool or Git).
2.  Configure with `-static` flag.
3.  Run compilation (takes 1-3 hours).
4.  Point your project to the new Static Qt Kit.

## Current Project Status
-   **Current Output**: Portable Directory (EXE + DLLs).
-   **Goal**: Single Static EXE.
-   **Blocker**: Missing Static Qt Kit (`D:\Qt\6.9.3-static`).

## Recommendation
If you need a true single EXE natively, we must allocate time to build the Static Qt Kit on this machine.
