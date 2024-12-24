from __future__ import annotations

import json
from pathlib import Path
from typing import List, TypedDict


class Annotation(TypedDict):
    alternative_syllables: List[str]
    current_syllable: str
    id: int
    is_correct: bool


class Data(TypedDict):
    annotations: List[Annotation]
    text: str


ROOT = Path(__file__).parent.parent.resolve()
with open(ROOT / "extern" / "VSEC" / "Dataset" / "VSEC.jsonl", "r", encoding="utf-8") as reader:
    with open(ROOT / "data" / "input.txt", "w", encoding="utf-8") as writer:
        for line in reader:
            data: Data = json.loads(line)
            writer.write(data["text"] + "\n")
