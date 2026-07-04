import json
import os

path = r"C:\Users\curtis\.gemini\antigravity-ide\brain\21f6f814-c1e9-4b18-b36a-8e9a619f26ff\.system_generated\logs\transcript.jsonl"
if os.path.exists(path):
    with open(path, "r", encoding="utf-8") as f:
        for idx, line in enumerate(f):
            try:
                data = json.loads(line)
                content = str(data.get("content", ""))
                tool_calls = str(data.get("tool_calls", ""))
                text_to_search = (content + " " + tool_calls).lower()
                if "chunk" in text_to_search or "timeout" in text_to_search:
                    print(f"Step {data.get('step_index', idx)}: source={data.get('source')}, type={data.get('type')}")
                    print(content[:500])
                    print("-" * 50)
            except Exception as e:
                pass
