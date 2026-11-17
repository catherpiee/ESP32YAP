# Packs web/index.html (with the font inlined) into src/web_page.h as gzip. Runs automatically before each build.
import base64
import gzip
import os

try:
    Import("env")  # noqa: F821  (defined when PlatformIO runs this)
    ROOT = env["PROJECT_DIR"]  # noqa: F821
except NameError:
    ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

html = open(os.path.join(ROOT, "web", "index.html"), encoding="utf-8").read()
font = base64.b64encode(open(os.path.join(ROOT, "fonts", "Archivo-Black-latin.ttf"), "rb").read()).decode()
html = html.replace("fonts/Archivo-Black-latin.ttf", "data:font/ttf;base64," + font)
data = gzip.compress(html.encode("utf-8"), 9, mtime=0)
lines = ["// Generated from web/index.html by tools/make_web.py. Edit that file instead.", "#pragma once", "#include <Arduino.h>",
         f"static const size_t WEB_PAGE_GZ_LEN = {len(data)};", "static const uint8_t WEB_PAGE_GZ[] PROGMEM = {"]
for i in range(0, len(data), 40):
    lines.append("  " + ",".join(map(str, data[i:i + 40])) + ",")
lines.append("};")
out = "\n".join(lines) + "\n"
path = os.path.join(ROOT, "src", "web_page.h")
if not os.path.exists(path) or open(path, encoding="utf-8").read() != out:
    open(path, "w", encoding="utf-8").write(out)
