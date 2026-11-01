#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1) The printed cheat sheets (docs/cheatsheet-*.png and .pdf) and docs/navigation-map.png, rendered with the
system's Chromium from its command line, headless (no Playwright): the sizes tools/render_cheatsheets.mjs and
tools/gen_nav_map.py gave them.

  python3 tools/render_chromium.py            render all five files
  python3 tools/render_chromium.py --verify   exit 1 unless each file has the size its source gives now

A cheat sheet: 1600 px wide at 1.5x (the PNG 2400 px wide), the page's whole height (measured: a copy of the page with a
script that puts its height in the title, --dump-dom); the PDF one page of 1600 px x the height + 2, backgrounds
printed (a copy with an @page rule). The map: the SVG's own size at 2x, a transparent background, 256 colours.
CHROMIUM names another Chromium (default /usr/bin/chromium)."""
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image

R = Path(__file__).resolve().parent.parent
CHROME = os.environ.get("CHROMIUM", "/usr/bin/chromium")
SHEETS = ("controls", "sound")
W_SHEET, SCALE_SHEET = 1600, 1.5


def chrome(*args):
    prof = tempfile.mkdtemp(prefix="render-chromium-")
    try:
        return subprocess.run([CHROME, "--headless", "--disable-gpu", "--hide-scrollbars", "--no-first-run",
                               "--allow-file-access-from-files", f"--user-data-dir={prof}",
                               "--virtual-time-budget=5000", *args],
                              check=True, capture_output=True, text=True, timeout=120).stdout
    finally:
        shutil.rmtree(prof, ignore_errors=True)


def copy_of(name, head_extra="", body_extra=""):
    """docs/cheatsheet/NAME.html as a temporary file that still finds its fonts (a <base>), with extras"""
    src = R / "docs" / "cheatsheet" / f"{name}.html"
    html = src.read_text()
    base = f'<base href="{src.parent.as_uri()}/">'
    html = re.sub(r"<head>", "<head>" + base + head_extra, html, count=1)
    html = re.sub(r"</body>", body_extra + "</body>", html, count=1)
    f = tempfile.NamedTemporaryFile("w", suffix=".html", delete=False)
    f.write(html)
    f.close()
    return Path(f.name)


def sheet_height(name):
    p = copy_of(name, body_extra="<script>document.title='H='+document.documentElement.scrollHeight</script>")
    try:
        dom = chrome(f"--window-size={W_SHEET},1000", "--dump-dom", p.as_uri())
    finally:
        p.unlink()
    return int(re.search(r"<title>H=(\d+)</title>", dom).group(1))


def svg_size(svg):
    head = svg.read_text()[:400]
    return int(re.search(r'width="(\d+)"', head).group(1)), int(re.search(r'height="(\d+)"', head).group(1))


def render_sheet(name):
    h = sheet_height(name)
    png, pdf = R / "docs" / f"cheatsheet-{name}.png", R / "docs" / f"cheatsheet-{name}.pdf"
    p = copy_of(name)
    try:
        chrome(f"--window-size={W_SHEET},{h}", f"--force-device-scale-factor={SCALE_SHEET}", f"--screenshot={png}",
               p.as_uri())
    finally:
        p.unlink()
    p = copy_of(name, head_extra=f"<style>@page{{size:{W_SHEET}px {h + 2}px;margin:0}}html{{-webkit-print-color-adjust:"
                                 f"exact;print-color-adjust:exact}}</style>")
    try:
        chrome("--no-pdf-header-footer", f"--print-to-pdf={pdf}", p.as_uri())
    finally:
        p.unlink()
    print(f"{png.relative_to(R)}, {pdf.relative_to(R)}: {W_SHEET} x {h} px")


def render_map(svg, png):
    w, h = svg_size(svg)
    chrome(f"--window-size={w},{h}", "--force-device-scale-factor=2", "--default-background-color=00000000",
           f"--screenshot={png}", svg.as_uri())
    Image.open(png).quantize(256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE).save(png, optimize=True)
    print(f"{Path(png).relative_to(R)}: 2x, {Path(png).stat().st_size // 1024} KB")


def pdf_pages(pdf):
    b = pdf.read_bytes()
    boxes = re.findall(rb"/MediaBox\s*\[\s*0\s+0\s+([\d.]+)\s+([\d.]+)\s*\]", b)
    return len(re.findall(rb"/Type\s*/Page\b", b)), [(float(x), float(y)) for x, y in boxes]


def verify():
    bad = []
    for name in SHEETS:
        h = sheet_height(name)
        png, pdf = R / "docs" / f"cheatsheet-{name}.png", R / "docs" / f"cheatsheet-{name}.pdf"
        w0, h0 = Image.open(png).size
        if w0 != round(W_SHEET * SCALE_SHEET) or abs(h0 - round(h * SCALE_SHEET)) > 2:
            bad.append(f"{png.name}: {w0} x {h0}, the page gives {round(W_SHEET * SCALE_SHEET)} x {round(h * SCALE_SHEET)}")
        n, boxes = pdf_pages(pdf)
        if n != 1 or not boxes or abs(boxes[0][0] - W_SHEET * 0.75) > 1 or abs(boxes[0][1] - (h + 2) * 0.75) > 1:
            bad.append(f"{pdf.name}: {n} pages, {boxes[:1]}, the page gives 1 of {W_SHEET * 0.75} x {(h + 2) * 0.75} pt")
    svg, png = R / "docs" / "navigation-map.svg", R / "docs" / "navigation-map.png"
    w, h = svg_size(svg)
    im = Image.open(png)
    if im.size != (2 * w, 2 * h) or im.mode != "P":
        bad.append(f"{png.name}: {im.size[0]} x {im.size[1]} {im.mode}, the SVG gives {2 * w} x {2 * h} P")
    for b in bad:
        print("render_chromium: FAIL", b)
    print("render_chromium: the renders are the size their sources give" if not bad else "")
    return not bad


def main():
    if not Path(CHROME).exists():
        sys.exit(f"render_chromium: no Chromium at {CHROME} (nothing rendered; nothing may be installed)")
    if "--verify" in sys.argv:
        sys.exit(0 if verify() else 1)
    for name in SHEETS:
        render_sheet(name)
    render_map(R / "docs" / "navigation-map.svg", R / "docs" / "navigation-map.png")


if __name__ == "__main__":
    main()
