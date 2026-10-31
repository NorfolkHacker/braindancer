#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
# (0.9.1) one calibration row: a snapshot now (TAG:s) and one ~20 s later (TAG:e), then the row (tools/gen_cal.py delta)
#   tools/cal_piece.sh TAG [LOG [PORT]]
set -e
TAG=$1; LOG=${2:-build/cal/cal-091.log}; D=${3:-/dev/ttyACM0}
[ -n "$TAG" ] || { echo "usage: tools/cal_piece.sh TAG [LOG [PORT]]"; exit 2; }
H=$(dirname "$0")
"$H/cal_snap.sh" "$TAG:s" "$LOG" "$D" > /dev/null
sleep 17
"$H/cal_snap.sh" "$TAG:e" "$LOG" "$D" > /dev/null
python3 "$H/gen_cal.py" delta "$LOG" "$TAG"
