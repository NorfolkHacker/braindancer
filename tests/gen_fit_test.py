#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1) tools/gen_fit.py: the model as gen.c computes it (0.8's and 0.9's figures in the calibration doc), a fit that
puts every row of a known model on its side, and tests/gen_cal_rows.h the rows' own."""
import random
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import gen_fit as F  # noqa: E402

bad = 0


def check(what, ok):
    global bad
    print(f"gen_fit: {what:<74} {'ok' if ok else 'FAIL'}")
    bad += not ok


rows = F.load(ROOT / "docs/braindancer/gen-cal-rows.csv")
by = {r["tag"]: r for r in rows}
th08 = dict(F.THETA0, e6=79, v6=26, k=0)
check("0.8's costs from the doc's breakdown (FM6 79 / 26, no component term)",
      [F.cost(by["0.8 " + t], th08) for t in ("0036", "2020", "1174", "8794", "1751", "7848")] ==
      [836, 845, 668, 707, 672, 583])
check("0.9's model: 0036 773 of 746, 1751 812, WILD 7848 583 of 569",
      F.cost(by["0.8 0036"], F.THETA0) == 773 and F.budget(by["0.8 0036"], F.THETA0) == 746 and
      F.cost(by["0.8 1751"], F.THETA0) == 812 and F.cost(by["0.8 7848"], F.THETA0) == 583 and
      F.budget(by["0.8 7848"], F.THETA0) == 569)
check("the late rows: 0.8's seven (late20 > 5)", sorted(r["tag"] for r in rows if r["late"] and r["tag"].startswith("0.8")) ==
      sorted("0.8 " + t for t in ("1751", "1174", "0036", "8794", "0965", "8790", "4493")))
rnd = random.Random(91)
true = {"sty": 100, "wild": 170, "e6": 50, "v6": 16, "k": 40}   # (21 of the 40 rows late)
syn = []
for i in range(40):
    parts = [(rnd.choice((0, 2, 3, 5, 10, 11, 12)), rnd.randrange(0, 401)) for _ in range(rnd.randrange(4, 7))]
    r = {"tag": f"s{i}", "wild": int(i % 10 == 0), "two": 1, "base": 117, "fixed": rnd.choice((295, 320, 342, 405)),
         "tape": rnd.randrange(2), "ccomp": rnd.randrange(4), "parts": parts, "late20": 0.0}
    r["late"] = F.cost(r, true) > F.budget(r, true)
    syn.append(r)
th, (score, margin, ml, mc) = F.fit(syn)
check(f"a known model's rows ({sum(r['late'] for r in syn)} late of 40): every row on its side", score == 0)
src = (ROOT / "firmware/src/gen.c").read_text()


def const(n):
    return int(re.search(r"#define %s (\d+)u" % n, src).group(1))


def arr(n):
    return [int(x) for x in re.search(r"%s\[NENGINES\] = \{([^}]*)\}" % n, src).group(1).split(",")]


A = F.ADOPTED
eng, vox = arr("GC_ENG"), arr("GC_VOX")
check("gen.c's constants are gen_fit.py's ADOPTED (factors, FM6, component, TAPE, DEV2)",
      (const("GC_GEN_X100"), const("GC_WILD_X100"), const("GC_CCOMP"), const("GC_TAPE1"), const("GC_TAPE2"),
       const("GC_DEV2_X100"), const("GC_DEV2W_X100"), eng[1], eng[12], vox[1], vox[12]) ==
      (A["sty"], A["wild"], A["k"], *A["tape"], *F.dev2(A), A["e6"], A["e6"], A["v6"], A["v6"]))
p = subprocess.run([sys.executable, str(ROOT / "tools/gen_fit.py"), str(ROOT / "docs/braindancer/gen-cal-rows.csv"),
                    "--check-h", str(ROOT / "tests/gen_cal_rows.h")])
check("tests/gen_cal_rows.h is the rows'", p.returncode == 0)
print("gen_fit: all ok" if not bad else f"gen_fit: {bad} FAILED")
sys.exit(bad != 0)
