#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1) tools/render_chromium.py's failures, with stand-ins for Chromium (no Chromium run): a Chromium that fails
shows its stderr (no traceback); one that hangs past the timeout is killed with its children (its process group);
verify() prints no empty line when it fails."""
import contextlib
import io
import os
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import render_chromium as rc  # noqa: E402

bad = []
tmp = Path(tempfile.mkdtemp(prefix="render-chromium-test-"))
pids = tmp / "pids"
try:
    # 1. Chromium fails: its stderr in the message, no traceback
    fail = tmp / "fail.sh"
    fail.write_text("#!/bin/sh\necho 'chromium: cannot open display' >&2\nexit 3\n")
    fail.chmod(0o755)
    p = subprocess.run([sys.executable, str(ROOT / "tools/render_chromium.py"), "--verify"], capture_output=True,
                       text=True, env={**os.environ, "CHROMIUM": str(fail)}, timeout=60)
    if p.returncode == 0 or "cannot open display" not in p.stderr or "Traceback" in p.stderr:
        bad.append(f"a failing Chromium: rc {p.returncode}, stderr {p.stderr.strip()[-300:]!r}")

    # 2. Chromium hangs (with a child of its own): both killed at the timeout
    hang = tmp / "hang.sh"
    hang.write_text(f"#!/bin/sh\nsleep 60 &\necho $! >> {pids}\necho $$ >> {pids}\nwait\n")
    hang.chmod(0o755)
    rc.CHROME, rc.TIMEOUT = str(hang), 1
    try:
        rc.chrome("about:blank")
        bad.append("a hanging Chromium: no error")
    except SystemExit as e:
        if "1 s" not in str(e):
            bad.append(f"a hanging Chromium: {e}")
    alive = []
    for _ in range(20):                                 # (the children reaped by init: at most 2 s)
        alive = []
        for pid in map(int, pids.read_text().split() if pids.exists() else []):
            try:
                st = Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()[0]
            except OSError:
                continue
            if st != "Z":
                alive.append(pid)
        if not alive:
            break
        time.sleep(0.1)
    if alive or not pids.exists():
        bad.append(f"a hanging Chromium: left running {alive}")

    # 3. verify() failing: its FAIL lines, no empty line
    rc.sheet_height = lambda name: 7
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        ok = rc.verify()
    if ok or "" in out.getvalue().splitlines() or "FAIL" not in out.getvalue():
        bad.append(f"verify() failing: returned {ok}, printed {out.getvalue()!r}")
finally:
    for pid in map(int, pids.read_text().split() if pids.exists() else []):
        with contextlib.suppress(OSError):
            os.kill(pid, signal.SIGKILL)
    for f in tmp.iterdir():
        f.unlink()
    tmp.rmdir()

for b in bad:
    print("render_chromium_test: FAIL", b)
print("render_chromium_test: ok" if not bad else "render_chromium_test: FAILED")
sys.exit(bool(bad))
