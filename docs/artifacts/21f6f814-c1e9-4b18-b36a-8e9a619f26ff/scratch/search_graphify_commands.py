import json
import os

path = r"C:\Users\curtis\.gemini\antigravity-ide\brain\21f6f814-c1e9-4b18-b36a-8e9a619f26ff\.system_generated\logs\transcript.jsonl"
if os.path.exists(path):
    with open(path, "r", encoding="utf-8") as f:
        for idx, line in enumerate(f):
            try:
                data = json.loads(line)
                step = data.get("step_index", idx)
                tool_calls = data.get("tool_calls", [])
                for tc in tool_calls:
                    cmd = tc.get("args", {}).get("CommandLine", "")
                    if "graphify" in cmd or "graphify" in str(tc):
                        print(f"Step {step}: tool={tc.get('name')}")
                        print(f"  Command: {cmd}")
            except Exception as e:
                pass
