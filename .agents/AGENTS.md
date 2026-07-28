## Agy Graphify Memory System

You have a persistent, long-term knowledge graph of your past conversations located at `C:\Users\curtis\.gemini\agy_memory_summaries\`.

When the user asks you to recall something from a past session (e.g., "what did we do with Process Lasso?", "why did we change that config?", "what was the AnyDesk fix?"), you MUST query your memory graph before attempting to manually search log files.

**To query your memory:**
1. Use the `run_command` tool to execute: `graphify query "your question here"`
2. The working directory for the command MUST be: `C:\Users\curtis\.gemini\agy_memory_summaries\`
3. Use the output of the query to answer the user's question.

## Build System Rule

When building the 'QGalleryX' project, you MUST ALWAYS use the build script:
- **Command:** .\build.ps1
- **Working directory:** d:\Dev\QGalleryX

NEVER use raw cmake --build, 
inja, or Start-Process to build or launch the application directly.
The build script handles killing instances, cleaning autogen, compiling both  QGalleryX and  QGalleryXBench, deploying Qt dependencies, and verifying binary freshness.

Bypassing the build script risks stale builds, locked binaries, and codebase divergence.
