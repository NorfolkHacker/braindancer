# The voice budget (NVOICE)

Two build options, both passed by `tools/build.py` as `-D`:

- `FELUCCA_NVOICE=N` (core.h `NVOICE`, 1..16, default 8): the voices each part holds (`track_t.v[NVOICE]`, RAM).
- `FELUCCA_VOICES_2CORE=T` (voice.c `VOICES_2CORE`, NVOICE..min(NPART * NVOICE, 16), default 12): the TOTAL voices all
  parts share while the second core plays (the device's default, CORES 2). This is "as many voices as the hardware can
  handle", and what piece 2's device step (Task 5) measures: T = 12, 14, 16 with NVOICE 8, using JIANT's CPU TEST song
  (SAVE > TOOLS > 6 CPU TEST). Task 5 sets the default T.

The three caps on the shared total (`voice_cap()`): LOAD ECO 6; one core (REC held at power-on, or core 1 given up)
NVOICE; two cores VOICES_2CORE. The load governor (`gov_cap`, audio.c `gov_tick`) lowers it further under load; its
ceiling `GOV_MAX` = max(VOICES_2CORE, NVOICE + 4) now follows T (it was a fixed NVOICE + 4 = 12, which would have
silently held 14 and 16 to 12).

History: `NVOICE 8` is in the first commit of the tree (5684e18, "Felucca 0.8-beta source") with no reason recorded in
the code, README or docs. JIANT 0.9.12 (329aff5) added `VOICES_2CORE 12` in voice.c: the shared budget is 12 while the
second core takes parts (MENU > SYSTEM > ABOUT says CORES 2), NVOICE otherwise, and ECO 6. So on a two-core device the
shared cap is already 12 whatever NVOICE is (see Caveat).

## Build checks per T (NVOICE 8; packages `build/acid-fm-vc12.fwsc`, `-vc14`, `-vc16`, git-ignored)

| T | app (B) | slot free (B) | RAM .data+.bss (of 98304) | pool (B) / spare | RAM code (of 25600) |
|---|---------|---------------|---------------------------|------------------|---------------------|
| 12 (default) | 553708 | 30628 | 75156 | 310544 / 8928 | 19456 |
| 14 | 553708 | 30628 | 75156 | 310544 / 8928 | 19456 |
| 16 | 553708 | 30628 | 75156 | 310544 / 8928 | 19456 |

All build checks passed. T changes only constants (same sizes; the packages differ). target_budget passes the same at
each T. Per-part NVOICE (earlier measurement, T=12): 10 gives RAM 76212 (+1056), 12 gives 77268 (+2112), app +80 / +96 B,
all checks passing; nothing hit a ceiling. History: `NVOICE 8` is in the first commit (5684e18) with no reason recorded;
JIANT 0.9.12 (329aff5) added the 12 with the second core.

## What else scales with T (checked)

- `voice_cap()` readers (voice.c voice_room / voice_victim, the MENU > ABOUT cap display): all read the function.
- audio.c governor: `gov_tick` ceiling was NVOICE + 4; now `GOV_MAX`. `gov_cap` is a byte starting at 16, so T <= 16
  (static-asserted). The shed / floor numbers (4 least, -2 on a shed) are absolute and do not scale.
- fx.c `mc_two()` / the core-1 split (`mc_cost`, `mc_split`) work per part, not per total voice; core 1's buffers
  (`c1_buf`, `c1_mix`) are block-sized, not voice-sized; its stack is call depth, not voice count (the build's RAM and
  RAM-code checks pass). No fixed-size structure is sized by 12.
- Per-part arrays stay NVOICE. T <= NPART * NVOICE (48) is asserted.
- The audio ISR's 80 % overload guard and `load_tight` measure core 0's half, so they hold at any T; they cost the
  render more at 14 / 16, which is what the device step measures.

## Host cost

`tests/target_budget.py` (static loop counts) is unchanged by T. There is no host number for the CPU TEST song:
`tests/regress.c` counts with macOS `proc_pid_rusage` (0 instr here) and `tests/cpu_vg.py` needs valgrind (not installed;
skipped). The device measurement is the authority.

## Device measurements (Task 5), keyed by T (NVOICE 8)

| T | CPU TEST mean load % | PEAK | LATE | SHED | clicks heard (yes/no) |
|---|----------------------|------|------|------|-----------------------|
| 12 |  |  |  |  |  |
| 14 |  |  |  |  |  |
| 16 |  |  |  |  |  |
