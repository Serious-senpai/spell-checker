from __future__ import annotations

import json
from typing import Iterable

from config import Data, ROOT


def benchmark_token_generator(data: Data) -> Iterable[str]:
    for annotation in data["annotations"]:
        if annotation["current_syllable"] == "":
            continue

        yield annotation["current_syllable"] if len(annotation["alternative_syllables"]) == 0 else annotation["alternative_syllables"][0]


matched = total = 0
with open(ROOT / "extern" / "VSEC" / "Dataset" / "VSEC.jsonl", "r", encoding="utf-8") as benchmark:
    with open(ROOT / "data" / "output.txt", "r", encoding="utf-8") as reader:
        for benchmark_line, reader_line in zip(benchmark, reader, strict=True):
            benchmark_data: Data = json.loads(benchmark_line)

            for correct, token in zip(benchmark_token_generator(benchmark_data), reader_line.split(), strict=True):
                total += 1
                matched += correct == token


print(f"Matched {matched}/{total} tokens ({100 * matched / total:.2f}%)")
