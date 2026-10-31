#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1 spec 8, 9) What the pages say now: BREAK (dropped 2026-10-28) is on neither the README's intro and roadmap nor
the landing page's tagline and roadmap (past entries keep it); the installer has no two-core warning (two cores run on
the FM-1 since 0.1)."""
import re
import sys
from pathlib import Path

R = Path(__file__).resolve().parent.parent
bad = 0


def check(what, ok):
    global bad
    print(f"pages: {what:<72} {'ok' if ok else 'FAIL'}")
    bad += not ok


def no_break(text):
    return not re.search(r"\bBREAK\b", text) and not re.search(r"breakbeat|chopped break", text, re.I)


readme = (R / "README.md").read_text()
intro = readme.split("**[Install]", 1)[0]
road = readme.split("The Braindancer features arrive in steps", 1)[1].split("\n\n")[1]
nums = [int(n) for n in re.findall(r"^(\d+)\. ", road, re.M)]
check("README: the intro without BREAK", no_break(intro))
check("README: the roadmap without BREAK, numbered 1..n", no_break(road) and nums == list(range(1, len(nums) + 1)))
land = (R / "web/landing/index.html").read_text()
tag = re.search(r'<p class="tag">(.*?)</p>', land, re.S).group(1)
steps = re.search(r"<h2>Where it is</h2>\s*<ol>(.*?)</ol>", land, re.S).group(1)
check("landing: the tagline and the roadmap without BREAK", no_break(tag) and no_break(steps))
pkg = (R / "web/index_pkg.html").read_text()
check("installer: no two-core warning (EN / JA / ES)", 'data-t="dual"' not in pkg and not re.search(r"\bdual:", pkg) and
      "0.6.71" not in pkg)
print("pages: all ok" if not bad else f"pages: {bad} FAILED")
sys.exit(bad != 0)
