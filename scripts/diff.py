from __future__ import annotations

import asyncio
import json
import webbrowser

import aiohttp

from config import ROOT


input_path = ROOT / "data" / "input.txt"
output_path = ROOT / "data" / "output.txt"
diff_path = ROOT / "data" / "diff.html"


async def diffchecker() -> None:
    async with aiohttp.ClientSession() as session:
        data = {
            "left": input_path.read_text(encoding="utf-8"),
            "right": output_path.read_text(encoding="utf-8"),
        }
        async with session.post(
            "https://api.diffchecker.com/public/text",
            params={
                "output_type": "html",
                "diff_level": "word",
                "email": "invalid@example.com",
            },
            headers={
                "Content-Type": "application/json",
            },
            data=json.dumps(data),
            timeout=None
        ) as response:
            response.raise_for_status()
            html = await response.text(encoding="utf-8")
            with diff_path.open("w", encoding="utf-8") as writer:
                writer.write(html)


asyncio.run(diffchecker())
webbrowser.open_new_tab(diff_path.as_uri())
