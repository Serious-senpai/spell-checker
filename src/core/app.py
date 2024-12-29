from __future__ import annotations

from pathlib import Path
from typing import Any, Callable, TypeVar, TYPE_CHECKING

from aiohttp import web
from multidict import MultiDictProxy

from .c_utils import inference


__all__ = ("Application",)
T = TypeVar("T")


core = Path(__file__).parent.resolve()


def _extract_form_key(
    form: MultiDictProxy[Any],
    *,
    key: str,
    cast: Callable[[Any], T],
) -> T:
    try:
        return cast(form[key])

    except Exception:
        raise web.HTTPBadRequest


class Application(web.Application):

    if TYPE_CHECKING:
        edit_distance_threshold: int
        max_candidates_per_token: int
        edit_penalty_factor: float

    def __init__(
        self,
        *,
        edit_distance_threshold: int,
        max_candidates_per_token: int,
        edit_penalty_factor: float,
    ) -> None:
        super().__init__()

        self.edit_distance_threshold = edit_distance_threshold
        self.max_candidates_per_token = max_candidates_per_token
        self.edit_penalty_factor = edit_penalty_factor

        self.add_routes(
            [
                web.get("/", self._root),
                web.post("/api", self._api),
            ],
        )

    async def _root(self, request: web.Request) -> web.Response:
        with open(core / "static" / "index.html", "r", encoding="utf-8") as file:
            html = file.read()

        html = html.replace(r"{{ edit_distance_threshold }}", str(self.edit_distance_threshold))
        html = html.replace(r"{{ max_candidates_per_token }}", str(self.max_candidates_per_token))
        html = html.replace(r"{{ edit_penalty_factor }}", str(self.edit_penalty_factor))
        return web.Response(
            text=html,
            content_type="text/html",
        )

    async def _api(self, request: web.Request) -> web.Response:
        form = await request.post()

        text = _extract_form_key(form, key="text", cast=str)
        edit_distance_threshold = _extract_form_key(form, key="edit_distance_threshold", cast=int)
        max_candidates_per_token = _extract_form_key(form, key="max_candidates_per_token", cast=int)
        edit_penalty_factor = _extract_form_key(form, key="edit_penalty_factor", cast=float)

        if edit_distance_threshold < 0 or max_candidates_per_token < 0 or edit_penalty_factor < 0.0 or edit_penalty_factor > 1.0:
            raise web.HTTPBadRequest

        return web.Response(
            text=inference(
                text,
                edit_distance_threshold=edit_distance_threshold,
                max_candidates_per_token=max_candidates_per_token,
                edit_penalty_factor=edit_penalty_factor,
            ),
        )
