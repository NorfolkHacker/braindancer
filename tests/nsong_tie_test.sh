#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
# (0.9.1) the NSONG tie: tests/nsong_tie.c builds with NSONG 4 / NSONG_INDEX 8 and does not with NSONG 5 or NSONG_INDEX 9,
# each refused by storage.c's tie (its message: the sector layout's own assert also refuses NSONG 5, not NSONG_INDEX 9)
CC=${CC:-tools/host-cc}
F="-fsyntax-only -w -Ibuild/gen -Ifirmware/src -Itests"
ERR=$(mktemp)
trap 'rm -f "$ERR"' EXIT
$CC $F -DTIE_NSONG=4u tests/nsong_tie.c || { echo "nsong_tie: FAIL: does not build with NSONG 4 (the test's includes)"; exit 1; }
for d in "" "-DTIE_NSONG=4u -DTIE_NSONG_INDEX=9u"; do
    if $CC $F $d tests/nsong_tie.c 2>"$ERR"; then echo "nsong_tie: FAIL: builds with ${d:-NSONG 5}"; exit 1; fi
    grep -q "song_chain.c and storage.c differ" "$ERR" ||
        { echo "nsong_tie: FAIL: ${d:-NSONG 5} refused but not by the tie"; cat "$ERR"; exit 1; }
done
echo "nsong_tie: ok"
