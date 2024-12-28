from __future__ import annotations

from typing import List, TypedDict


__all__ = ("Annotation", "Data")


class Annotation(TypedDict):
    alternative_syllables: List[str]
    current_syllable: str
    id: int
    is_correct: bool


class Data(TypedDict):
    annotations: List[Annotation]
    text: str
