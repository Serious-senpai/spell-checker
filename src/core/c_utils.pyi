def initialize(*, frequency_path: str, wordlist_path: str) -> None: ...
def inference(
    input: str,
    *,
    edit_distance_threshold: int = 2,
    max_candidates_per_token: int = 1000,
    edit_penalty_factor: float = 0.01,
) -> str: ...
