from __future__ import annotations

import json

from config import Data, ROOT


with open(ROOT / "extern" / "VSEC" / "Dataset" / "VSEC.jsonl", "r", encoding="utf-8") as reader:
    with open(ROOT / "data" / "input.txt", "w", encoding="utf-8") as writer:
        for line in reader:
            data: Data = json.loads(line)
            writer.write(data["text"])
            writer.write("\n")
