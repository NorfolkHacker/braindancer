# The voice budget (NVOICE)

`NVOICE` (firmware/src/core.h) is the voices per part and the budget the parts share. It is a build option:
`FELUCCA_NVOICE=N ./build.sh` (1..16, default 8; `tools/build.py` passes it as `-DFELUCCA_NVOICE=N`). The device step
(Task 5 of piece 2) measures 8, 10 and 12 with JIANT's CPU TEST song (SAVE > TOOLS > 6 CPU TEST) and sets the default.

History: `NVOICE 8` is in the first commit of the tree (5684e18, "Felucca 0.8-beta source") with no reason recorded in
the code, README or docs. JIANT 0.9.12 (329aff5) added `VOICES_2CORE 12` in voice.c: the shared budget is 12 while the
second core takes parts (MENU > SYSTEM > ABOUT says CORES 2), NVOICE otherwise, and ECO 6. So on a two-core device the
shared cap is already 12 whatever NVOICE is (see Caveat).

## Build checks per N (host build of the real image, `./build.sh`)

| N | app (B) | slot free (B) | RAM .data+.bss (of 98304) | pool (B) / spare | RAM code (of 25600) | farthest RAM-code call reach spare (B) |
|---|---------|---------------|---------------------------|------------------|---------------------|-----------------------------------------|
| 8  | 553708 | 30628 | 75156 | 310544 / 8928 | 19456 | 333004 |
| 10 | 553804 | 30532 | 76212 (+1056) | 310544 / 8928 | 19456 | 332956 |
| 12 | 553788 | 30548 | 77268 (+2112) | 310544 / 8928 | 19456 | 332956 |

All build checks passed at 8, 10 and 12 (no FAIL). RAM grows 528 B per voice of NVOICE (`track_t.v[NVOICE]` for 6 parts);
the pool does not depend on NVOICE (its users, eng_digital's `[NPART][NVOICE][4]` arrays, are not in the default build).
Packages for the device: `build/acid-fm-nv8.fwsc`, `build/acid-fm-nv10.fwsc`, `build/acid-fm-nv12.fwsc` (git-ignored).

## Host cost

`tests/target_budget.py` (static count of instructions in the render loops of the pi32v2 disassembly) is the only
host measurement that runs here: it is the same for every N except `drum_render` (347 -> 350 instructions, 33 in loops
both) and `mod_begin` (216 -> 218, 160 in loops both): the render loops do not get longer, because they run per active
voice and N only sets how many may be active. All lines pass the +10 % budget at 10 and 12 as at 8.

There is no host instructions-per-sample number for the CPU TEST song: `tests/regress.c` counts instructions with
macOS `proc_pid_rusage` (absent on Linux: 0 instr) and `tests/cpu_vg.py` needs valgrind, which is not installed (it
skips). `regress` built with `-DFELUCCA_NVOICE=10/12` reports 4 golden changes and 4 voice / routing check failures,
expected: the goldens and those checks encode the budget of 8. The device measurement is the authority.

## Caveat for the device step

`VOICES_2CORE` is a constant 12 in voice.c, independent of NVOICE. With CORES 2 the shared cap is 12 at NVOICE 8, 10
and 12 alike (each part holds NVOICE voices, so 12 sounding voices need parts to share them); only with one core
(`mc_two()` false) does NVOICE set the cap. The governor `gov_cap` is bounded by NVOICE + 4 (audio.c). To compare 8 / 10 /
12 on a two-core device, VOICES_2CORE has to follow NVOICE, or the test has to be run with one core.

## NVOICE-dependent storage checked

Nothing stored or on the wire depends on NVOICE: `track_t.v[NVOICE]` is RAM only (project.c copies `p` and `step`,
never the track or voice structs); the editor protocol, project, song and preset formats do not mention it.
Other users are loops and caps (voice.c, audio.c, eng_drum.c bounds, ui_lzedit.c, eng_digital.c arrays not built).
`voice.c:834` has `i < 8u`: the 8 EDIT knobs (P_E0..), not voices.

## Device measurements (Task 5)

| N | CPU TEST mean load % | PEAK | LATE | SHED | clicks heard (yes/no) |
|---|----------------------|------|------|------|-----------------------|
| 8  |  |  |  |  |  |
| 10 |  |  |  |  |  |
| 12 |  |  |  |  |  |
