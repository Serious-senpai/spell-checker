from __future__ import annotations

import asyncio
import time
from pathlib import Path

import aiohttp
from selenium import webdriver  # type: ignore
from selenium.webdriver.common.by import By  # type: ignore


ROOT = Path(__file__).parent.parent.resolve()
MEDIAFIRE_URL = "https://www.mediafire.com/file/015erqvfiomfhqh/corpus.zip/file"


async def download(url: str) -> None:
    async with aiohttp.ClientSession() as session:
        async with session.get(url, timeout=None) as response:
            response.raise_for_status()

            print(f"Downloading from {url!r}")
            with open(ROOT / "data" / "corpus.zip", "wb") as file:
                while data := await response.content.read(4096):
                    file.write(data)


options = webdriver.FirefoxOptions()
options.add_argument("--headless")

driver = webdriver.Firefox(options=options)
driver.get(MEDIAFIRE_URL)

# Possible cloudflare protection
time.sleep(5)

driver.get_full_page_screenshot_as_file(str(ROOT / "data" / "screenshot.png"))

button = driver.find_element(By.ID, "downloadButton")
url = button.get_attribute("href")
if url is None:
    raise ValueError("Could not find download URL")


asyncio.run(download(url))
