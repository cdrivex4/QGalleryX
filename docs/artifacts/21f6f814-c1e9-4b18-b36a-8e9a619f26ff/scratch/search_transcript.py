import json
import os

path = r"C:\Users\curtis\.gemini\antigravity-ide\brain\21f6f814-c1e9-4b18-b36a-8e9a619f26ff\.system_generated\logs\transcript.jsonl"
keywords = ["redshift", "quark", "spin", "initial data", "boundary", "tov", "limit", "theory"]

if not os.path.exists(path):
    print("Transcript not found at", path)
else:
    with open(path, "r", encoding="utf-8") as f:
        for idx, line in enumerate(f):
            try:
                data = json.loads(line)
                is_user = (
                    data.get("source") == "USER_EXPLICIT"
                    or data.get("type") == "USER_INPUT"
                    or "user" in str(data.get("source")).lower()
                )
                if is_user:
                    content = str(data.get("content", "")).lower()
                    matched = [kw for kw in keywords if kw in content]
                    if matched:
                        print(f"--- STEP {data.get('step_index', idx)} (matched: {matched}) ---")
                        # Print first 200 chars and last 200 chars of user input to avoid huge text
                        text = data.get("content", "")
                        if len(text) > 400:
                            print(text[:200] + "\n... [TRUNCATED] ...\n" + text[-200:])
                        else:
                            print(text)
                        print()
            except Exception as e:
                pass
