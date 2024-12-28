def initialize(*, frequency_path: str, wordlist_path: str) -> None: ...


def inference(
    input: str,
    *,
    edit_distance_threshold: int,
    max_candidates_per_token: int,
    edit_penalty_factor: float,
) -> str: ...
