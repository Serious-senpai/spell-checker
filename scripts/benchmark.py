from __future__ import annotations

import json

from config import Data, ROOT


matched = total = 0
with open(ROOT / "extern" / "VSEC" / "Dataset" / "VSEC.jsonl", "r", encoding="utf-8") as benchmark:
    with open(ROOT / "data" / "output.txt", "r", encoding="utf-8") as reader:
        for benchmark_line, reader_lines in zip(benchmark, reader, strict=True):
            benchmark_data: Data = json.loads(benchmark_line)

            for annotation, token in zip(benchmark_data["annotations"], reader_lines.split(), strict=True):
                correct = annotation["current_syllable"] if len(annotation["alternative_syllables"]) == 0 else annotation["alternative_syllables"][0]
                total += 1
                matched += correct == token


print(f"Matched {matched}/{total} tokens ({100 * matched / total:.2f}%)")
