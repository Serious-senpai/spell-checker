from __future__ import annotations

import json
from pathlib import Path
from typing import Iterable

from colorama import Fore, Style, just_fix_windows_console

from config import Data, ROOT


def benchmark_token_generator(data: Data) -> Iterable[str]:
    for annotation in data["annotations"]:
        if annotation["current_syllable"] == "":
            continue

        yield annotation["current_syllable"] if len(annotation["alternative_syllables"]) == 0 else annotation["alternative_syllables"][0]


def benchmark(path: Path) -> float:
    matched = total = 0
    with open(ROOT / "extern" / "VSEC" / "Dataset" / "VSEC.jsonl", "r", encoding="utf-8") as benchmark:
        with path.open("r", encoding="utf-8") as reader:
            for benchmark_line, reader_line in zip(benchmark, reader, strict=True):
                benchmark_data: Data = json.loads(benchmark_line)

                for correct, token in zip(benchmark_token_generator(benchmark_data), reader_line.split(), strict=True):
                    total += 1
                    matched += correct == token

    match_percentage = 100 * matched / total
    print(f"[{path.resolve()}] Matched {matched}/{total} tokens ({match_percentage:.2f}%)")
    return match_percentage


before = benchmark(ROOT / "data" / "input.txt")
after = benchmark(ROOT / "data" / "output.txt")
delta = after - before

just_fix_windows_console()
if delta < 0:
    print(f"{Fore.RED}{delta:.2f}%{Style.RESET_ALL}")
else:
    print(f"{Fore.GREEN}+{delta:.2f}%{Style.RESET_ALL}")
