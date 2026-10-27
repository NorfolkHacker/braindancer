# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""ACID FM's own name for its origins (3c-1 task 10, spec 5): ACID FM is "built on JIANT FM and Felucca", never "a fork of
JIANT" or "JIANT FM under a new name". Checked: the device's ABOUT (firmware/src/ui_menu.c, its string literals) and the
visible text of the web pages (the installer, the 404, the editor, the emulator, the manual's page and scripts, the
installer's scripts' messages: comments left out, they may keep JIANT's version notes). The licence record stays as it is:
LICENSING.md and every page's copyright lines (checked present)."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BAD = re.compile(r"fork of (JIANT|Felucca)|under a new name|JIANT FM under", re.I)
WEB = ["web/index_pkg.html", "web/404.html", "web/editor.html", "web/emu/index.html", "web/manual/index.html",
       "web/manual/manual.js", "web/manual/data.js", "web/fm1backup.js", "web/fm1diag.js", "web/fm1ota.js"]
# the copyright lines that must stay where they are (spec 5: code comments and LICENSING.md unchanged)
KEEP = {
    "LICENSING.md": ["Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments",
                     "Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) — JIANT FM",
                     "GPL-3.0-only"],
    "web/index_pkg.html": ["Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)"],
    "web/editor.html": ["Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)"],
    "web/emu/index.html": ["Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)"],
    "web/fm1backup.js": ["Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)"],
    "firmware/src/ui_menu.c": ["Copyright (C) 2026 Juan Ignacio Antonucci"],
}


def visible(src: str) -> str:
    """the text with HTML, C and JS comments taken out (a // after a colon is a URL's, kept)"""
    keep = lambda m: "\n" * m.group(0).count("\n")   # (the lines kept: a finding's line is the file's)
    src = re.sub(r"<!--.*?-->", keep, src, flags=re.S)
    src = re.sub(r"/\*.*?\*/", keep, src, flags=re.S)
    return re.sub(r"(^|[\s;{}(),])//[^\n]*", r"\1", src)


def c_literals(src: str) -> str:
    return " ".join(re.findall(r'"((?:[^"\\\n]|\\.)*)"', visible(src)))


bad = []
about = c_literals((ROOT / "firmware/src/ui_menu.c").read_text(encoding="utf-8"))
if "Built on JIANT FM and Felucca" not in about:
    bad.append('ui_menu.c: ABOUT does not say "Built on JIANT FM and Felucca"')
for m in BAD.finditer(about):
    bad.append(f'ui_menu.c: ABOUT says "{m.group(0)}"')
for who in ("Juan Jiant", "Hugelton Instruments", "NorfolkHacker"):   # (the credits keep every name)
    if who not in about:
        bad.append(f"ui_menu.c: ABOUT no longer names {who}")
for f in WEB:
    text = visible((ROOT / f).read_text(encoding="utf-8"))
    for m in BAD.finditer(text):
        line = text[:m.start()].count("\n") + 1
        bad.append(f'{f}: "{m.group(0)}" in its visible text (line {line})')
for f, lines in KEEP.items():
    src = (ROOT / f).read_text(encoding="utf-8")
    for s in lines:
        if s not in src:
            bad.append(f"{f}: lost its line {s!r}")
if bad:
    print("\n".join("FAIL: " + b for b in bad))
    sys.exit(1)
print(f"ok   built on JIANT FM and Felucca: ABOUT and {len(WEB)} web files; the copyright lines kept")
