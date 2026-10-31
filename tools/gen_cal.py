#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1) GEN's CPU calibration data (docs/braindancer/gen-calibration-0.9.1.md).

  tools/gen_cal.py delta LOG TAG             the row of TAG (its :s and :e snapshots in LOG, tools/cal_snap.sh)
  tools/gen_cal.py csv LOG OUT.csv           every row of LOG -> OUT.csv (docs/braindancer/gen-calibration-0.9.1.csv)
  tools/gen_cal.py ids CSV                   the rows' identities for tests/gen_terms.c, in the session's order
  tools/gen_cal.py rows CSV TERMS ROWS.csv   the rows tests/gen_terms.c rebuilt exactly, appended to ROWS.csv

A LOG line: "HH:MM:SS TAG |key value|key value|..." (the console's status lines). A TAG ending -AGE<n> is a window
played with AGE set to n by hand (FX > MASTER): its age_live column; such rows are not fitted."""
import csv
import os
import re
import sys

DELTA = ("audio_late", "voices_shed", "audio_lost", "voices_given_up", "safety_tex", "safety_arp", "safety_tape",
         "dly_safety")
ID = ("seed", "two", "dev", "form", "tape", "main", "part", "energy", "complex")
COLS = ("tag", "piece", *ID, "age", "age_live", "secs", "cpu_s", "cpu_e", "late", "shed", "lost", "given_up",
        "safety_tex", "safety_arp", "safety_tape", "dly_safety", "cost_s", "cost_e", "budget", "late20", "shed20",
        "note")
ROWS = ("session", "tag", "name", "wild", "two", "base", "fixed", "tape", "ccomp", "parts", "late20", "timed")
GID_WILD = "254"


def parse(line):
    """(tag, {key: value text}) of a LOG line; None for any other line"""
    m = re.match(r"^\d\d:\d\d:\d\d (\S+) \|(.*)$", line.rstrip("\n"))
    if not m:
        return None
    f = {}
    for part in m.group(2).split("|"):
        if " " in part:
            k, v = part.split(" ", 1)
            f[k] = v.strip()
    return m.group(1), f


def snaps(lines):
    """({TAG: (s, e)}, the tags in order): each tag's last :s and :e snapshot"""
    s, e, order = {}, {}, []
    for line in lines:
        p = parse(line)
        if not p:
            continue
        tag, f = p
        if tag.endswith(":s"):
            if tag[:-2] not in s:
                order.append(tag[:-2])
            s[tag[:-2]] = f
        elif tag.endswith(":e"):
            e[tag[:-2]] = f
    order = [t for t in order if t in e]
    return {t: (s[t], e[t]) for t in order}, order


def num(f, k):
    v = f.get(k, "-")
    return int(v) if re.fullmatch(r"-?\d+", v) else None


def row(tag, s, e):
    secs = (num(e, "uptime_ms") - num(s, "uptime_ms")) / 1000.0
    d = {k: num(e, k) - num(s, k) for k in DELTA}
    ident = e.get("gen_id", "-").split()
    r = {"tag": tag, "piece": e.get("gen_piece", "-"), "secs": f"{secs:.1f}", "cpu_s": num(s, "cpu_pct"),
         "cpu_e": num(e, "cpu_pct"), "late": d["audio_late"], "shed": d["voices_shed"], "lost": d["audio_lost"],
         "given_up": d["voices_given_up"], "safety_tex": d["safety_tex"], "safety_arp": d["safety_arp"],
         "safety_tape": d["safety_tape"], "dly_safety": d["dly_safety"], "cost_s": s.get("gen_cost", "-"),
         "cost_e": e.get("gen_cost", "-"), "budget": e.get("gen_budget", "-"), "age": e.get("gen_age", "-"),
         "late20": f"{d['audio_late'] * 20.0 / secs:.1f}", "shed20": f"{d['voices_shed'] * 20.0 / secs:.1f}"}
    for k, v in zip(ID, ident if len(ident) == len(ID) else ["-"] * len(ID)):
        r[k] = v
    m = re.search(r"-AGE(\d+)$", tag)
    r["age_live"] = m.group(1) if m else ""
    notes = []
    if r["piece"] == "-":
        notes.append("no GEN piece")
    elif s.get("gen_piece") != e.get("gen_piece"):
        notes.append("piece changed in the window")
    r["note"] = "; ".join(notes)
    return r


def main(argv):
    if len(argv) >= 3 and argv[0] == "delta":
        rows, _ = snaps(open(argv[1]))
        r = row(argv[2], *rows[argv[2]])
        print(f"{r['tag']} {r['piece']}: late20 {r['late20']} shed20 {r['shed20']} lost {r['lost']} cpu {r['cpu_s']}->"
              f"{r['cpu_e']} cost {r['cost_s']}/{r['cost_e']} budget {r['budget']} age {r['age']} {r['note']}")
    elif len(argv) == 3 and argv[0] == "csv":
        rows, order = snaps(open(argv[1]))
        with open(argv[2], "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=COLS, lineterminator="\n")
            w.writeheader()
            for t in order:
                w.writerow(row(t, *rows[t]))
    elif len(argv) == 2 and argv[0] == "ids":
        for r in csv.DictReader(open(argv[1])):
            if not r["note"] and not r["age_live"]:
                print(f"{r['tag']}|{' '.join(r[k] for k in ID)}|{r['cost_s']}|{r['cost_e']}|{r['piece']}")
    elif len(argv) == 4 and argv[0] == "rows":
        dev = {r["tag"]: r for r in csv.DictReader(open(argv[1]))}
        new = not os.path.exists(argv[3]) or not open(argv[3]).read().strip()
        with open(argv[3], "a", newline="") as f:
            w = csv.DictWriter(f, fieldnames=ROWS, lineterminator="\n")
            if new:
                w.writeheader()
            for line in open(argv[2]):
                tag, match, fixed, tape, ccomp, parts, cost, name = line.rstrip("\n").split("|")
                if match != "1":
                    print(f"gen_cal: {tag} {name}: not rebuilt exactly (MATCH 0): left out", file=sys.stderr)
                    continue
                d = dev[tag]
                w.writerow({"session": "0.9.1", "tag": tag, "name": d["piece"], "wild": int(d["main"] == GID_WILD),
                            "two": d["two"], "base": d["dev"], "fixed": fixed, "tape": tape, "ccomp": ccomp,
                            "parts": parts, "late20": d["late20"], "timed": 1})
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main(sys.argv[1:])
