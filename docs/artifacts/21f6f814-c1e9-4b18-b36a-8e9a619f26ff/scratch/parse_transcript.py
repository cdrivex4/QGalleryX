import json
import os

path = r"C:\Users\curtis\.gemini\antigravity-ide\brain\21f6f814-c1e9-4b18-b36a-8e9a619f26ff\.system_generated\logs\transcript.jsonl"
if not os.path.exists(path):
    print("Transcript not found at", path)
else:
    with open(path, "r", encoding="utf-8") as f:
        for idx, line in enumerate(f):
            try:
                data = json.loads(line)
                step = data.get("step_index", idx)
                if step < 15:
                    is_user = (
                        data.get("source") == "USER_EXPLICIT"
                        or data.get("type") == "USER_INPUT"
                        or "user" in str(data.get("source")).lower()
                    )
                    if is_user:
                        print(f"--- STEP {step} ---")
                        print(data.get("content"))
                        print()
            except Exception as e:
                pass
