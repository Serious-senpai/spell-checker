from __future__ import annotations

from pathlib import Path

from selenium import webdriver  # type: ignore
from selenium.webdriver.common.by import By  # type: ignore


ROOT = Path(__file__).parent.parent.resolve()
MEDIAFIRE_URL = "https://www.mediafire.com/file/015erqvfiomfhqh/corpus.zip/file"


options = webdriver.ChromeOptions()
options.binary_location = "/usr/lib/chromium-browser/chromedriver"

driver = webdriver.Chrome(options=options)
driver.get(MEDIAFIRE_URL)
button = driver.find_element(By.ID, "downloadButton")
url = button.get_attribute("href")
print(url)
