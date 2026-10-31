#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
# (0.9.1) GEN's calibration (docs/braindancer/gen-calibration-0.9.1.md): the FM-1's console `status`, the lines the
# calibration reads, appended to LOG as one line "HH:MM:SS TAG |key value|key value|..." (build/cal/snap.sh of 0.8 / 0.9
# with the gen_* lines). The controller's, on the device; reading the reply is bounded by `timeout 3`.
#   tools/cal_snap.sh TAG [LOG [PORT]]      (LOG build/cal/cal-091.log, PORT /dev/ttyACM0)
set -e
TAG=$1; LOG=${2:-build/cal/cal-091.log}; D=${3:-/dev/ttyACM0}
[ -n "$TAG" ] || { echo "usage: tools/cal_snap.sh TAG [LOG [PORT]]"; exit 2; }
mkdir -p "$(dirname "$LOG")"
F=$(mktemp)
stty -F "$D" 115200 raw -echo
(timeout 3 cat "$D" > "$F" &)
sleep 0.5; printf 'status\r' > "$D"; sleep 2.4
printf '%s %s |' "$(date +%T)" "$TAG" >> "$LOG"
tr -d '\r' < "$F" | grep -E '^(uptime_ms|cpu_pct|audio_max_us|audio_late|voices_shed|audio_lost|safety_tape|safety_tex|safety_arp|dly_safety|voices_given_up|playing|gen_piece|gen_cost|gen_budget|gen_age|gen_id) ' | tr '\n' '|' >> "$LOG"
echo >> "$LOG"
rm -f "$F"
tail -1 "$LOG"
