---
name: graphify-folder
description: Extracts and builds a Graphify knowledge graph for any given project folder path.
---

# Graphify Folder Skill

Use this skill when the user asks you to "graphify" a folder, project, or path (e.g. "graphify D:\Dev\beamsolver").

## Instructions
1. Validate that the target path exists on disk.
2. Run the `graphify extract` command pointing at the target path.
3. Configure the local LLM environment variables for LM Studio.
4. Auto-fallback models if you encounter JSON errors in the output.

### Execution
Use the `run_command` tool to execute:
```powershell
$env:GRAPHIFY_OPENAI_BASE_URL = "http://127.0.0.1:8666/v1"
$env:OPENAI_BASE_URL = "http://127.0.0.1:8666/v1"
$env:OPENAI_API_BASE = "http://127.0.0.1:8666/v1"
$env:OPENAI_API_KEY = "lm-studio"
graphify extract "<TARGET_PATH>" --backend openai --model "ibm/granite-4-h-tiny" --max-concurrency 1 --api-timeout 600 --global
```
*Note: If `ibm/granite-4-h-tiny` fails with JSON errors or timeouts, step up to `qwen2.5-coder-14b-instruct`, or eventually `--no-cluster`.*
