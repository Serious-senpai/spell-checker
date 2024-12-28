from __future__ import annotations

import argparse
import asyncio
import json
import os
from pathlib import Path
from typing import Iterable, Literal, TYPE_CHECKING

import uvloop
from aiohttp import web
from colorama import Fore, Style, just_fix_windows_console
from tqdm import tqdm

from models import Data
from core import app, inference, initialize


class Namespace(argparse.Namespace):
    if TYPE_CHECKING:
        option: Literal["server", "input", "benchmark"]
        frequency_path: Path
        wordlist_path: Path
        verbose: bool


def benchmark_token_generator(data: Data) -> Iterable[str]:
    for annotation in data["annotations"]:
        if annotation["current_syllable"] == "":
            continue

        yield annotation["current_syllable"] if len(annotation["alternative_syllables"]) == 0 else annotation["alternative_syllables"][0]


def run_server(namespace: Namespace) -> None:
    asyncio.set_event_loop_policy(uvloop.EventLoopPolicy())
    web.run_app(app)


def run_input(namespace: Namespace) -> None:
    while True:
        try:
            text = input("Enter text (Ctrl-C to exit)>")
            print(inference(text))

        except (EOFError, KeyboardInterrupt):
            break


def run_benchmark(namespace: Namespace) -> None:
    before_matched = after_matched = total = 0
    dataset = ROOT / "extern" / "VSEC" / "Dataset" / "VSEC.jsonl"
    with dataset.open("r", encoding="utf-8") as reader:
        iterable: Iterable[str] = reader
        if namespace.verbose:
            iterable = tqdm(iterable, desc=str(dataset), ascii="█ ")

        try:
            for line in iterable:
                data: Data = json.loads(line)
                for correct, token in zip(benchmark_token_generator(data), data["text"].split(), strict=True):
                    total += 1
                    before_matched += correct == token

                for correct, token in zip(benchmark_token_generator(data), inference(data["text"]).split(), strict=True):
                    after_matched += correct == token

        except KeyboardInterrupt:
            pass

    before = 100 * before_matched / total
    after = 100 * after_matched / total
    delta = after - before

    just_fix_windows_console()

    print(f"{before_matched}/{total} ({before:.3f}%) -> {after_matched}/{total} ({after:.3f}%)")
    if delta < 0:
        print(f"{Fore.RED}{delta:.3f}%{Style.RESET_ALL}")
    else:
        print(f"{Fore.GREEN}+{delta:.3f}%{Style.RESET_ALL}")

    if os.getenv("CI"):
        print(f"::notice::{after:.3f}% (improved {delta:.3f}%)")


ROOT = Path(__file__).parent.parent.resolve()
parser = argparse.ArgumentParser(
    description="Vietnamese spell checker",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)
parser.add_argument("-o", "--option", choices=["server", "input", "benchmark"], default="server", help="Start a spell-checking server, read from stdin, or run benchmarking")
parser.add_argument("-f", "--frequency-path", type=Path, default=ROOT / "data" / "frequency.txt", help="Path to the frequency file")
parser.add_argument("-w", "--wordlist-path", type=Path, default=ROOT / "data" / "wordlist.txt", help="Path to the wordlist file")
parser.add_argument("-v", "--verbose", action="store_true", help="Enable verbose mode")


if __name__ == "__main__":
    namespace = Namespace()
    parser.parse_args(namespace=namespace)

    print(namespace)
    initialize(
        frequency_path=str(namespace.frequency_path),
        wordlist_path=str(namespace.wordlist_path),
    )

    callbacks = {
        "server": run_server,
        "input": run_input,
        "benchmark": run_benchmark,
    }
    callback = callbacks[namespace.option]
    callback(namespace)
