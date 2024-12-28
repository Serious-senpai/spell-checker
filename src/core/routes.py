from __future__ import annotations

from pathlib import Path
from typing import Any, Callable, TypeVar

from aiohttp import web
from multidict import MultiDictProxy

from .c_utils import inference


__all__ = ()
T = TypeVar("T")


server = Path(__file__).parent.resolve()
router = web.RouteTableDef()
router.static("/static", server / "static")


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


@router.get("/")
async def root(request: web.Request) -> web.Response:
    raise web.HTTPTemporaryRedirect("/static/index.html")


@router.post("/api")
async def api(request: web.Request) -> web.Response:
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
