#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1) tools/build.py: FELUCCA_NVOICE / FELUCCA_VOICES_2CORE take an ASCII integer 1..16 only (str.isdigit also
says yes to other scripts' digits and to superscripts, which int() then refuses or reads)."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import build  # noqa: E402

CASES = {"8": 8, "1": 1, "16": 16, "0": None, "17": None, "": None, "x": None, " 8": None, "٣": None,
         "²": None, "１２": None}
bad = [repr(v) for v, want in CASES.items() if build.small_int(v) != want]
print("build: small_int " + ("ok" if not bad else "FAIL on " + ", ".join(bad)))
sys.exit(bool(bad))
