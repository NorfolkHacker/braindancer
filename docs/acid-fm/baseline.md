# Baseline: JIANT main as forked

- Upstream commit: b66729d38a2e6aff21d24dda0e40ed53a63f5667
- Built: 2026-10-24, Linux x86-64, JieLi clang 4.0.1, AC79 SDK d179b44
- App (`build/felucca.bin`): 568436 B of the 0x8EA90 (584336 B) XIP slot
- Package (`build/felucca.fwsc`): 610113 B, identity FM-1_900
- RAM code: RAM code 19464 B of 25600
- Host tests: ALL HOST TESTS PASSED (92 test groups, about 4556 `ok` check lines), 0 failed; skipped: web (no node), emulator (no emcc), logical image vs fm1pkg.js (needs node), official V15 restore (needs FM-1_v15.fwsc)

## Host-compiler note

The stock host C compiler here is GCC 14, which makes `-Wimplicit-function-declaration` a hard error. Several test
translation units include firmware sources that call functions declared later (e.g. `lcd_fill`, `live_return`), so
`sh tests/run_tests.sh` fails to compile on untouched JIANT. The baseline run used a one-line wrapper as `CC`:

    cc -std=gnu17 -Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=incompatible-pointer-types "$@"

(`CC=/path/to/wrapper sh tests/run_tests.sh`; the wrapper must be a single word because the sanitizer builds use
`${CC%% *}`.) No test or source was edited.
