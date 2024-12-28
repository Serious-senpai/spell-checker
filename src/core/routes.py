from __future__ import annotations

from pathlib import Path
from aiohttp import web

from .c_utils import inference


__all__ = ()


server = Path(__file__).parent.resolve()
router = web.RouteTableDef()
router.static("/static", server / "static")


@router.get("/")
async def root(request: web.Request) -> web.Response:
    raise web.HTTPTemporaryRedirect("/static/index.html")


@router.post("/api")
async def api(request: web.Request) -> web.Response:
    form = await request.post()

    try:
        text = form["text"]
    except KeyError:
        raise web.HTTPBadRequest

    if isinstance(text, str):
        return web.Response(text=inference(text))

    raise web.HTTPBadRequest
