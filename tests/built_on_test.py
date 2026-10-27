# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""ACID FM's own name for its origins (3c-1 task 10, spec 5): ACID FM is "built on JIANT FM and Felucca", never "a fork of
JIANT" or "JIANT FM under a new name". Checked: the device's ABOUT (firmware/src/ui_menu.c, its string literals) and the
visible text of the web pages (the installer, the 404, the editor, the emulator, the manual's page and scripts, the
installer's scripts' messages, the landing page: comments left out, they may keep JIANT's version notes; whitespace, line
breaks, &nbsp; and HTML tags between the words do not hide a match). (fix 1) The installer's scripts' messages and report
name no JIANT at all ("JIANT" in capitals in a string; the lower-case "jiant" is the firmware kind they detect), the report
says "ACID FM installer report". The licence record stays as it is: LICENSING.md and every page's copyright lines (checked
present)."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BAD = re.compile(r"fork\s+of\s+(JIANT|Felucca)|under\s+a\s+new\s+name|JIANT\s+FM\s+under", re.I)
WEB = ["web/index_pkg.html", "web/404.html", "web/editor.html", "web/emu/index.html", "web/manual/index.html",
       "web/manual/manual.js", "web/manual/data.js", "web/fm1backup.js", "web/fm1diag.js", "web/fm1ota.js", "web/fm1pkg.js",
       "web/landing/index.html"]
SCRIPTS = ["web/fm1diag.js", "web/fm1ota.js", "web/fm1backup.js", "web/fm1pkg.js"]   # (the installer's messages and report)
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


def plain(src: str, html: bool) -> str:
    """visible() with the entities for a space as spaces and (HTML) the tags taken out, the lines kept"""
    src = re.sub(r"&nbsp;|&#160;|&#x0*a0;", " ", visible(src), flags=re.I).replace("\u00a0", " ")
    if html:
        src = re.sub(r"</?[A-Za-z][^<>]*>", lambda m: "\n" * m.group(0).count("\n"), src)
    return src


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
    text = plain((ROOT / f).read_text(encoding="utf-8"), f.endswith(".html"))
    for m in BAD.finditer(text):
        line = text[:m.start()].count("\n") + 1
        bad.append(f'{f}: "{" ".join(m.group(0).split())}" in its visible text (line {line})')
for f in SCRIPTS:
    text = visible((ROOT / f).read_text(encoding="utf-8"))
    for m in re.finditer(r"([\"'`])((?:(?!\1)[^\\\n]|\\.)*)\1", text):
        if "JIANT" in m.group(2):
            line = text[:m.start()].count("\n") + 1
            bad.append(f"{f}: a message names JIANT (line {line}): {m.group(2)[:60]!r}")
if "ACID FM installer report" not in visible((ROOT / "web/fm1diag.js").read_text(encoding="utf-8")):
    bad.append('web/fm1diag.js: the report does not open with "ACID FM installer report"')
for f, lines in KEEP.items():
    src = (ROOT / f).read_text(encoding="utf-8")
    for s in lines:
        if s not in src:
            bad.append(f"{f}: lost its line {s!r}")
if bad:
    print("\n".join("FAIL: " + b for b in bad))
    sys.exit(1)
print(f"ok   built on JIANT FM and Felucca: ABOUT and {len(WEB)} web files; the copyright lines kept")
