#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1) tools/gen_cal.py: a calibration log (tools/cal_snap.sh) into the device CSV and the harness's identities."""
import csv
import io
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import gen_cal  # noqa: E402

LOG = """16:40:43 G01:s |uptime_ms 134488|cpu_pct 67|audio_max_us 3295|audio_late 45|voices_shed 112|audio_lost 0|dly_safety 0|safety_tex 1|safety_arp 1|safety_tape 0|voices_given_up 220|gen_piece RETROBOC+AMBIENT 7692|gen_cost 702|gen_budget 746|gen_age 70|gen_id 7692 1 117 1 0 37 36 50 50|playing 1|
16:41:03 G01:e |uptime_ms 154430|cpu_pct 58|audio_max_us 3295|audio_late 103|voices_shed 262|audio_lost 0|dly_safety 0|safety_tex 1|safety_arp 1|safety_tape 0|voices_given_up 515|gen_piece RETROBOC+AMBIENT 7692|gen_cost 655|gen_budget 746|gen_age 70|gen_id 7692 1 117 1 0 37 36 50 50|playing 1|
16:41:16 G02-AGE0:s |uptime_ms 167025|cpu_pct 73|audio_late 134|voices_shed 338|audio_lost 0|dly_safety 0|safety_tex 1|safety_arp 1|safety_tape 1|voices_given_up 663|gen_piece -|gen_cost -|gen_budget -|gen_age -|gen_id -|playing 1|
16:41:36 G02-AGE0:e |uptime_ms 186962|cpu_pct 69|audio_late 153|voices_shed 390|audio_lost 0|dly_safety 0|safety_tex 1|safety_arp 1|safety_tape 1|voices_given_up 791|gen_piece -|gen_cost -|gen_budget -|gen_age -|gen_id -|playing 1|
not a snapshot line
"""
bad = 0


def check(what, ok):
    global bad
    print(f"gen_cal: {what:<70} {'ok' if ok else 'FAIL'}")
    bad += not ok


rows, order = gen_cal.snaps(LOG.splitlines(True))
check("two rows, in the log's order", order == ["G01", "G02-AGE0"])
r = gen_cal.row("G01", *rows["G01"])
check("G01: the deltas over the window, per 20 s", r["late"] == 58 and r["shed"] == 150 and r["given_up"] == 295 and
      r["secs"] == "19.9" and r["late20"] == "58.2" and r["cpu_s"] == 67 and r["cpu_e"] == 58)
check("G01: the piece, its identity, its model figures, no note", r["piece"] == "RETROBOC+AMBIENT 7692" and
      r["seed"] == "7692" and r["two"] == "1" and r["dev"] == "117" and r["main"] == "37" and r["part"] == "36" and
      r["cost_s"] == "702" and r["cost_e"] == "655" and r["budget"] == "746" and r["age"] == "70" and
      r["age_live"] == "" and r["note"] == "")
r2 = gen_cal.row("G02-AGE0", *rows["G02-AGE0"])
check("G02-AGE0: AGE set by hand (its tag), no piece: noted", r2["age_live"] == "0" and r2["note"] == "no GEN piece")
with tempfile.TemporaryDirectory() as d:
    log, out = Path(d) / "cal.log", Path(d) / "out.csv"
    log.write_text(LOG)
    subprocess.run([sys.executable, str(ROOT / "tools/gen_cal.py"), "csv", str(log), str(out)], check=True)
    got = list(csv.DictReader(out.open()))
    check("csv: every column, one line a row", [g["tag"] for g in got] == ["G01", "G02-AGE0"] and
          list(got[0].keys()) == list(gen_cal.COLS))
    ids = subprocess.run([sys.executable, str(ROOT / "tools/gen_cal.py"), "ids", str(out)], check=True,
                         capture_output=True, text=True).stdout
    check("ids: the rows with a piece and no hand-set AGE, for tests/gen_terms.c",
          ids == "G01|7692 1 117 1 0 37 36 50 50|702|655|RETROBOC+AMBIENT 7692\n")
print("gen_cal: all ok" if not bad else f"gen_cal: {bad} FAILED")
sys.exit(bad != 0)
