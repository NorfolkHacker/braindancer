#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1) GEN's CPU model re-fitted on the device's rows (docs/braindancer/gen-cal-rows.csv: 0.8's 15 pieces and the
0.9.1 session's; docs/braindancer/gen-calibration-0.9.1.md). A row is late when the FM-1 played it with more than 5 late
halves per 20 s (spec 0.9.1 item 1). The fit: over a grid of five constants (a style's factor GC_GEN_X100, WILD's
GC_WILD_X100, FM6's GC_ENG / GC_VOX, GC_CCOMP), the fewest rows on the wrong side of the budget (a late row within it
counts 2, a clean row over it 1) among the points with at most a quarter of the clean rows over, then the widest margin, then the nearest to 0.9's constants.

  tools/gen_fit.py ROWS.csv                  the fit, and each row's cost / budget under 0.9's constants and the fit's
  tools/gen_fit.py ROWS.csv --emit-h OUT.h   and the rows as C for tests/gen_test.c (tests/gen_cal_rows.h)
  tools/gen_fit.py ROWS.csv --check-h OUT.h  exit 1 unless OUT.h is what --emit-h writes"""
import csv
import sys

ENG = [27, 40, 45, 27, 27, 64, 27, 27, 27, 27, 70, 60, 40, 27, 27]   # gen.c GC_ENG at 0.9 (FM6 12, DIGITAL 1: fitted)
VOX = [19, 13, 15, 17, 19, 23, 19, 19, 19, 19, 11, 16, 13, 19, 19]   # gen.c GC_VOX at 0.9
FM6 = (1, 12)
TAPE = (76, 92)                                                       # GC_TAPE1 / GC_TAPE2 (not fitted)
THETA0 = {"sty": 118, "wild": 155, "e6": 40, "v6": 13, "k": 70}
# the point adopted in gen.c (0.9.1): the best score of the fit's points that also pass tests/gen_test.c's fit-floor bounds
ADOPTED = {"sty": 122, "wild": 150, "e6": 30, "v6": 18, "k": 70}
DEV2_0 = (150, 196)                                                   # GC_DEV2_X100 / GC_DEV2W_X100 at THETA0
GRID = {"sty": range(100, 182, 2), "wild": range(120, 225, 5), "e6": range(20, 85, 5), "v6": range(6, 28, 2),
        "k": range(0, 150, 10)}
LATE = 5.0
NTRK = 6


def load(path):
    rows = []
    for r in csv.DictReader(open(path)):
        parts = [tuple(int(x) for x in p.split(":")) for p in r["parts"].split(";") if p]
        rows.append({"tag": r["tag"], "wild": int(r["wild"]), "two": int(r["two"]), "base": int(r["base"]),
                     "fixed": int(r["fixed"]), "tape": int(r["tape"]), "ccomp": int(r["ccomp"]), "parts": parts,
                     "late20": float(r["late20"]), "late": float(r["late20"]) > LATE})
    return rows


def dev2(th):
    return round(DEV2_0[0] * th["sty"] / THETA0["sty"]), round(DEV2_0[1] * th["wild"] / THETA0["wild"])


def dev(r, th):                                                       # gen.c gc_dev_of
    if not r["two"]:
        d = r["base"] or 160
    elif r["base"]:
        d = r["base"] * (th["wild"] if r["wild"] else th["sty"]) // 100
    else:
        d = dev2(th)[1 if r["wild"] else 0]
    return min(max(d, 100), 400)


def budget(r, th):                                                    # gen.c gc_budget
    d = dev(r, th)
    return 1030 * 100 // d if r["two"] else 714 * 100 // d - 120


def cost(r, th):                                                      # gen.c gc_sum
    c = r["fixed"] + r["ccomp"] * th["k"] + (TAPE[1 if r["two"] else 0] if r["tape"] else 0)
    for e, v in r["parts"]:
        en, vx = (th["e6"], th["v6"]) if e in FM6 else (ENG[e], VOX[e])
        c += en + vx * v // 100
    return c


def judge(rows, th):
    """(score, margin, late rows within, clean rows over)"""
    ml = mc = 0
    m = None
    for r in rows:
        c, b = cost(r, th), budget(r, th)
        x = c / b - 1.0 if r["late"] else 1.0 - c / b
        m = x if m is None else min(m, x)
        ml += r["late"] and c <= b
        mc += (not r["late"]) and c > b
    return 2 * ml + mc, m, ml, mc


def side(rows, costs, budgets):
    """(score, margin or None, clean rows over) of rows with these costs and budgets"""
    s, m, mc = 0, None, 0
    for r, c, b in zip(rows, costs, budgets):
        x = c / b - 1.0 if r["late"] else 1.0 - c / b
        m = x if m is None else min(m, x)
        s += (2 if c <= b else 0) if r["late"] else (1 if c > b else 0)
        mc += (not r["late"]) and c > b
    return s, m, mc


def fit(rows, limit=True):
    """(theta, judge(rows, theta)) of the best grid point; limit: only points with at most a quarter of the clean rows
    over the budget (the music is not thinned for lateness alone: the FM-1 lost no audio on any piece measured), the
    late rows within it a report. The unconstrained best: limit=False. A cost depends on e6 v6 k alone, a style row's budget on sty
    alone, a WILD row's on wild alone: the budgets once, the costs once a (e6, v6, k)"""
    sr, wr = [r for r in rows if not r["wild"]], [r for r in rows if r["wild"]]
    bs = {s: [budget(r, dict(THETA0, sty=s)) for r in sr] for s in GRID["sty"]}
    bw = {w: [budget(r, dict(THETA0, wild=w)) for r in wr] for w in GRID["wild"]}
    best = None
    nclean = sum(not r["late"] for r in rows)
    for e6 in GRID["e6"]:
        for v6 in GRID["v6"]:
            for k in GRID["k"]:
                th = dict(THETA0, e6=e6, v6=v6, k=k)
                cs, cw = [cost(r, th) for r in sr], [cost(r, th) for r in wr]
                d0 = abs(e6 / THETA0["e6"] - 1) + abs(v6 / THETA0["v6"] - 1) + abs(k / THETA0["k"] - 1)
                S = [(s, *side(sr, cs, bs[s])) for s in GRID["sty"]]
                W = [(w, *side(wr, cw, bw[w])) for w in GRID["wild"]]
                for s, ss, ms, cos in S:
                    for w, sw, mw, cow in W:
                        if limit and (cos + cow) * 4 > nclean:
                            continue
                        m = min([x for x in (ms, mw) if x is not None] or [0.0])
                        key = (ss + sw, -m, d0 + abs(s / THETA0["sty"] - 1) + abs(w / THETA0["wild"] - 1))
                        if best is None or key < best[0]:
                            best = (key, dict(th, sty=s, wild=w))
    return best[1], judge(rows, best[1])


def emit_h(rows):
    out = ["/* generated by tools/gen_fit.py from docs/braindancer/gen-cal-rows.csv: do not edit */",
           "typedef struct { const char *tag; uint8_t wild, two, late; uint16_t base; gc_terms_t t; } gcal_row_t;",
           "static const gcal_row_t GCAL[] = {"]
    for r in rows:
        eng = [e for e, _ in r["parts"]] + [0] * (NTRK - len(r["parts"]))
        v = [x for _, x in r["parts"]] + [0] * (NTRK - len(r["parts"]))
        out.append(f'    {{"{r["tag"]}", {r["wild"]}, {r["two"]}, {int(r["late"])}, {r["base"]}, {{{r["fixed"]}, '
                   f'{r["tape"]}, {r["ccomp"]}, {len(r["parts"])}, {{{", ".join(map(str, eng))}}}, '
                   f'{{{", ".join(map(str, v))}}}}}}},')
    out += ["};", "#define GCAL_N ((uint32_t)(sizeof GCAL / sizeof GCAL[0]))", ""]
    return "\n".join(out)


def main(argv):
    if not argv:
        sys.exit(__doc__)
    rows = load(argv[0])
    if "--check-h" in argv:
        h = argv[argv.index("--check-h") + 1]
        sys.exit(0 if open(h).read() == emit_h(rows) else f"gen_fit: {h} is not the rows' (run --emit-h)")
    th, (score, margin, ml, mc) = fit(rows)
    thu, ju = fit(rows, False)
    j0 = judge(rows, THETA0)
    print(f"gen_fit: {len(rows)} rows, {sum(r['late'] for r in rows)} late")
    print(f"  0.9's  {THETA0}: score {j0[0]}, late within {j0[2]}, clean over {j0[3]}, margin {j0[1]:+.3f}")
    print(f"  best, unconstrained {thu}: score {ju[0]}, late within {ju[2]}, clean over {ju[3]}, margin {ju[1]:+.3f}")
    print(f"  fitted (clean over at most a quarter of the clean rows) {th}: score {score}, late within {ml}, clean over {mc}, margin {margin:+.3f}; "
          f"GC_DEV2_X100 {dev2(th)[0]}, GC_DEV2W_X100 {dev2(th)[1]}")
    ja = judge(rows, ADOPTED)
    print(f"  adopted {ADOPTED}: score {ja[0]}, late within {ja[2]}, clean over {ja[3]}, margin {ja[1]:+.3f}")
    for r in rows:
        print(f"  {r['tag']:<14} {'LATE ' if r['late'] else 'clean'} late20 {r['late20']:6.1f}  0.9 {cost(r, THETA0)}/"
              f"{budget(r, THETA0)}  fit {cost(r, th)}/{budget(r, th)}  adopted {cost(r, ADOPTED)}/{budget(r, ADOPTED)}")
    if "--emit-h" in argv:
        open(argv[argv.index("--emit-h") + 1], "w").write(emit_h(rows))


if __name__ == "__main__":
    main(sys.argv[1:])
