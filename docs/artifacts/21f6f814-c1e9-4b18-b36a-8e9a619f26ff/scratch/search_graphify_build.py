import json
import os

path = r"C:\Users\curtis\.gemini\antigravity-ide\brain\21f6f814-c1e9-4b18-b36a-8e9a619f26ff\.system_generated\logs\transcript.jsonl"
if os.path.exists(path):
    with open(path, "r", encoding="utf-8") as f:
        for idx, line in enumerate(f):
            try:
                data = json.loads(line)
                step = data.get("step_index", idx)
                line_str = line.lower()
                if "build_from_json" in line_str or "build_merge" in line_str or "cluster" in line_str:
                    print(f"Step {step}: source={data.get('source')}, type={data.get('type')}")
                    content = str(data.get("content", ""))
                    if content:
                        print(f"  Content: {content[:300]}")
                    tool_calls = data.get("tool_calls", [])
                    if tool_calls:
                        for tc in tool_calls:
                            print(f"    Tool: {tc.get('name')} -> {str(tc.get('args'))[:150]}")
                    print("-" * 50)
            except Exception as e:
                pass
print("Done searching build/cluster steps.")
