<!-- SPDX-License-Identifier: GPL-3.0-only -->
<!-- Copyright (C) 2026 NorfolkHacker -->
# Baseline: JIANT main as forked

- Upstream commit: b66729d38a2e6aff21d24dda0e40ed53a63f5667
- Built: 2026-10-24, Linux x86-64, JieLi clang 4.0.1 (jieli-linux-toolchains-20260730.1), AC79 SDK d179b44
- App (`build/felucca.bin`): 568436 B of the 0x8EA90 (584336 B) XIP slot
- Package (`build/felucca.fwsc`): 610113 B, identity FM-1_900
- RAM code: 19464 B of 25600
- Host tests: stock GCC 14 aborts at the theme_test / editor_test builds (implicit declarations of `lcd_fill`, `live_return`); with `CC=tools/host-cc` all 92 groups pass, 0 failed. Skipped: web (no node), emulator (no emcc), logical image vs fm1pkg.js (needs node), official V15 restore (needs FM-1_v15.fwsc)

## Host-compiler note

GCC 14 makes `-Wimplicit-function-declaration` a hard error, and some test translation units include firmware sources
that call functions declared later. `tools/host-cc` (a one-word wrapper, as the sanitizer builds use `${CC%% *}`)
downgrades those errors. Run from the repo root: `CC=tools/host-cc sh tests/run_tests.sh`. No test or source was edited.
