import json
import os

path = r"C:\Users\curtis\.gemini\antigravity-ide\brain\21f6f814-c1e9-4b18-b36a-8e9a619f26ff\.system_generated\logs\transcript.jsonl"
if os.path.exists(path):
    with open(path, "r", encoding="utf-8") as f:
        for idx, line in enumerate(f):
            try:
                data = json.loads(line)
                step = data.get("step_index", idx)
                if 620 <= step <= 672:
                    print(f"Step {step}: source={data.get('source')}, type={data.get('type')}, status={data.get('status')}")
                    content = str(data.get("content", ""))
                    tool_calls = data.get("tool_calls", [])
                    if content:
                        print(f"  Content: {content[:300]}")
                    if tool_calls:
                        print(f"  Tool Calls: {str(tool_calls)[:300]}")
                    print("-" * 50)
            except Exception as e:
                pass
