from __future__ import annotations

from aiohttp import web

from .routes import router


__all__ = ("app",)


app = web.Application()
app.add_routes(router)
