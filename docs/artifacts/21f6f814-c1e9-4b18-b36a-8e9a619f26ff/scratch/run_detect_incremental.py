import sys
import json
from graphify.detect import detect_incremental
from pathlib import Path

path = Path(".")
result = detect_incremental(path)
new_total = result.get("new_total", 0)

print(f"New total files changed/added: {new_total}")
print("Incremental detection results:")
print(json.dumps(result, indent=2, ensure_ascii=False))

# Save the output
out_dir = Path("graphify-out")
out_dir.mkdir(exist_ok=True)
(out_dir / ".graphify_incremental.json").write_text(
    json.dumps(result, ensure_ascii=False), encoding="utf-8"
)
