import json
import os

path = r"C:\Users\curtis\.gemini\antigravity-ide\brain\21f6f814-c1e9-4b18-b36a-8e9a619f26ff\.system_generated\logs\transcript.jsonl"
out_path = r"C:\Users\curtis\.gemini\antigravity-ide\brain\21f6f814-c1e9-4b18-b36a-8e9a619f26ff\scratch\step_9_content.txt"

if not os.path.exists(path):
    print("Transcript not found at", path)
else:
    with open(path, "r", encoding="utf-8") as f:
        for idx, line in enumerate(f):
            try:
                data = json.loads(line)
                step = data.get("step_index", idx)
                if step == 9:
                    with open(out_path, "w", encoding="utf-8") as out_f:
                        out_f.write(data.get("content", ""))
                    print("Successfully wrote step 9 content to", out_path)
                    break
            except Exception as e:
                print("Error:", e)
