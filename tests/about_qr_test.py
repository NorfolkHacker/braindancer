#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""The ABOUT page's QR table (firmware/src/ui_menu.c ABOUT_QR) is the one tools/gen_about_qr.py makes for ABOUT_URL."""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ABOUT_URL = "https://github.com/NorfolkHacker/acid-fm"

src = (ROOT / "firmware/src/ui_menu.c").read_text()
m = re.search(r"static const uint32_t ABOUT_QR\[29\] = \{(.*?)\};", src, re.S)
if not m:
    sys.exit("FAIL: no ABOUT_QR[29] in ui_menu.c")
have = [int(w, 16) for w in re.findall(r"0x([0-9A-Fa-f]{8})u", m[1])]
out = subprocess.run([sys.executable, str(ROOT / "tools/gen_about_qr.py"), ABOUT_URL],
                     check=True, capture_output=True, text=True).stdout
want = [int(w, 16) for w in re.findall(r"0x([0-9A-Fa-f]{8})u", out)]
if len(want) != 29:
    sys.exit(f"FAIL: gen_about_qr.py made {len(want)} rows, not 29")
if have != want:
    sys.exit("FAIL: ABOUT_QR does not encode " + ABOUT_URL)
if ABOUT_URL not in src:
    sys.exit("FAIL: ui_menu.c does not name " + ABOUT_URL)
print("ok   ABOUT QR encodes", ABOUT_URL)
