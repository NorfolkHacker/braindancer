# Braindancer 0.9.1 (the clean-up release) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Braindancer 0.9.1: the safety net follows a GEN piece through its sections and leaves the parts the user
rewrites alone, GEN's hint shows on every way in, SEQ dice's OCT− restores the level, KEYBOARD PLAYS SELECTED drops
the bend of the track it leaves, the console names the GEN piece playing, GEN's CPU model is re-fitted on a second
device session, and the docs, renders, tools, dead code and missing goldens are tidied.

**Architecture:** The safety net keeps, next to `safe_part` (what it may hush now), the piece's own parts
(`safe_gen`), the sections the piece wrote (`safe_secs`) and whether the music is the piece (`safe_live`); a section
load of one of those sections and SONG END re-arm from them (`safe_arm`), a per-track signature polled at the 250 ms
autosave tick takes a track the user edits out of them (`safe_edit_poll` → `safe_edit`), and `safe_clear` stamps each
lever it drops as released. The console's `status` prints the piece from `gen_status` (gen.c), whose identity a host
harness (`tests/gen_terms.c`) rebuilds into the CPU model's terms (`gc_terms` / `gc_sum`, gc_cost split in two); a
stdlib Python fit (`tools/gen_fit.py`) chooses five constants on the device rows and `tests/gen_test.c` pins the
firmware's constants against the same rows (`tests/gen_cal_rows.h`). Renders use the system Chromium's command line
(`tools/render_chromium.py`).

**Tech Stack:** C (JieLi clang 4 for pi32v2 / host gcc via `tools/host-cc`), Python 3 (stdlib; Pillow for the renders),
POSIX sh for the device capture scripts, Node 22 (`build/node/bin`) for the web tests, Chromium 156 (`/usr/bin/chromium`).

**Spec:** `docs/superpowers/specs/2026-10-31-braindancer-0.9.1-cleanup.md` (binding). Evidence for each item:
`docs/superpowers/plans/2026-10-31-braindancer-0.9.1-triage.md` (step 3's table). Calibration method and data:
`docs/braindancer/gen-calibration-2026-10-31.md`, `docs/braindancer/device-log.md` (0.8 and 0.9 sections), the raw log
`build/cal/cal.log` (0.8's rows p1–R1573, 0.9's G01–G10).

## Global Constraints

- C only (firmware). GPL-3.0-only; never remove JIANT / Felucca copyright lines; changed files gain `Portions Copyright
  (C) 2026 NorfolkHacker` if they lack it; new files carry `SPDX-License-Identifier: GPL-3.0-only` and `Copyright (C)
  2026 NorfolkHacker`.
- Spec §4: "No storage format change; budgets ISR 300 / TIMER5 2170 / slicer 821 / tape 780; image ≥ 16 KiB free; the
  pin guard." (`tests/target_budget.txt`: `fm1_alnk0_irq 300`, `fm1_timer5_irq 2170`, `slicer_bus 821`, `tape_block
  780`, every line unchanged; `tape_block` is at its limit: no task touches TAPE's code.) The pin guard: `./build.sh`'s
  `layout:` line prints and the build does not stop on "variables an interrupt reaches not pinned". Image: about
  21.2 KB free at b3f8a80 (0.9: 563,140 B, 21,196 B free); floor **16 KiB free**. Each task reports `./build.sh`'s
  image and RAM lines.
- Storage append-only; nothing in this release changes a stored format (projects, presets, songs, settings).
- Spec §4: "Sound unchanged except where an item says so (item 1 may thin some GEN pieces more; the goldens of existing
  renders unchanged unless item 1 moves a GEN one, named)." A changed line in `tests/golden.txt` is a bug unless Task 9
  names it.
- Spec §4: "Plain English, no band names; JIANT only in the credits line. No downloads or installs." Every
  user-visible word plain upper-case English; `tests/built_on_test.py` and `tests/ux_msg_scan.py` stay green.
- Environment (venv required): `cd ~/synth/mvavefirmware && . .venv/bin/activate && export PATH=$PWD/.venv/bin:$PWD/build/node/bin:$PATH JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK`;
  build `./build.sh`; suite `CC=tools/host-cc sh tests/run_tests.sh` (≈ 8 min; its log is binary-ish: read it with
  `grep -a`); host test builds need `-O1` (as `run_tests.sh` builds them; `regress` alone is `-O2`, as there). One
  test alone: `tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/gen_test tests/gen_test.c -lm && build/host/gen_test | grep -a -E "FAIL|passed"`
  (`ui_test`, `audio_test`, `midi_control_test` the same with their names). Node is `build/node/bin/node` (v22, on
  PATH above): `web/test_web.mjs`, `web/test_manual.mjs` run here now. Absent: Playwright, emcc, valgrind: those
  checks are skipped (say so). Chromium (`/usr/bin/chromium`), Pillow and segno (in `.venv`) are present.
- Golden renders: never commit a whole-file `GOLDEN_UPDATE=1` rewrite. Task 7 adds only the missing lines; every other
  task expects no golden line to change.
- STANDING RULE: no agent may download, unpack or install any tool, package or binary. A missing tool: skip that check
  and report it.
- Process: **foreground commands only** (Bash timeout ≤ 600000 ms; if the suite exceeds it run its parts by hand, never
  in the background); never start wait loops (no `until` / `while pgrep`); stop anything you start; never leave HEAD
  detached.
- Branch `cleanup-091`; commits end with a blank line + `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`;
  never on main, never push; implementers never touch the device or the serial port (Tasks 8 and 10's device steps are
  the controller's, with the user).

## Pre-flight notes (spec points the code makes hard or ambiguous; the plan's rulings)

Each with the plan's default, which the tasks implement unless the controller rules otherwise.

1. **Which section loads re-arm the net (item 2).** Only a section the GEN piece wrote: `safe_secs` (bit s), set by
   `gen_build` from what `gen_parts` wrote (`gs.wrote`), a bit cleared when the user stores over that section
   (`section_store`) and all cleared by another project (`song_switch`). A LOOP piece writes no section (`safe_secs` 0),
   so an older piece's sections never get the new piece's tracks. SONG END re-arms iff the music before PLAY was the
   GEN piece (`song_keep_gen`, taken by `chain_prepare`). Sections that come in while a song plays (`sec_apply`, in the
   ISR) do not touch `safe_part`, as today (they never cleared it); a section of the user's inside a GEN song keeps the
   GEN arm while it plays: deferred list.
2. **What "a user edit" is (item 3).** Hooking each edit path (step edits, the roll, REC, clears, presets, user presets,
   engines, the editor's SysEx, MIDI CCs, MUTATE) is a dozen call sites; the codebase already detects "the music
   changed" by hashing (`project.c proj_music_sig`, `autosave_poll`'s 250 ms tick). So: `trk_edit_sig(k)` hashes track
   k's steps, every parameter at its base value (`motion_base_value`: automation playing is not an edit) except
   `P_MUTE` (a section's, a scene's mute is not an edit), its engine, preset and FM6 patch; `safe_edit_poll()` runs in
   that tick, takes the signatures again whenever the net is re-armed (`safe_arms`) or the section playing changes
   (`live.cur`), and otherwise takes a track whose signature moved out of the net (`safe_edit`). Any source counts
   (the panel, the web editor, an outside controller's CC, MUTATE). An edited track stays out **for the rest of the
   piece** (also when a later section of it loads: `safe_gen` loses it), until the next GENERATE / RECALL. An edit in
   the first 250 ms after the arm is taken as the piece's (documented).
3. **The release stamp (item 4).** `safe_clear` stamps every lever that is on (delay, texture, arp, TAPE:
   `dsf_since[l] = 0`), as a release by calm does. `dsf_since` / `dsf_rearm` move from `audio.c` to `voice.c` (next to
   `fx_safe`; `voice.c` is included before `audio.c` and in every host test that has `safe_clear`), so no ISR code
   changes. The pin guard pins by name, wherever in the translation unit a variable is defined.
4. **`status`'s fields (item 1a).** `gen_piece` = the panel's name and number (`gp.last`; the short name when the
   panel uses it) while the music is the GEN piece (`safe_live`: GENERATE, RECALL, one of its sections, SONG END back
   to it), else `-` (another project, a section of yours, NEW PROJECT, UNDO, CPU TEST). `gen_cost` = the model's cost
   of the music now (the section playing, its muted tracks free, as a section's fit counts them), `gen_budget` the
   piece's budget. Two more lines, so a row can be rebuilt on the host: `gen_age` (the piece's AGE after fitting) and
   `gen_id` (the identity's nine fields, decimal: `seed two dev form tape main part energy complex`). With no piece the
   four lines read `-`.
5. **The fit's labels and data (item 1).** A row is **late** when its window has more than 5 late halves per 20 s
   (`late20 > 5`, spec's "more than ~5 per 20 s"). 0.8's 15 rows are labelled by their own counters: 1751, 1174, 0036,
   8794, 0965 (7), 8790 (9) and 4493 (~10) late; 1573, 7625, 2020, 7033, 7563, 4459, 4741 and 7848 (~1) clean (the
   calibration doc's words "border"/"clean*"/"bad" are not used). Their model terms come from the doc's per-track
   breakdown (transcribed in Task 9 Step 1, checked against the doc's 0.8 costs). 0.9's ten rows (G01–G10) are not
   used: half are unnamed and none has an identity. The 0.9.1 session's rows are used when the host rebuilds the
   device's piece exactly (`MATCH 1`).
6. **What is fitted.** Five constants: `GC_GEN_X100` (a style's factor), `GC_WILD_X100`, FM6's `GC_ENG` / `GC_VOX`
   (indices 12 and DIGITAL's 1 together), `GC_CCOMP`. The unset-CPU-TEST factors `GC_DEV2_X100` / `GC_DEV2W_X100`
   scale with them (0.9's way: 150 × new / 118, 196 × new / 155). One core: unchanged (no data). TAPE (`GC_TAPE2`):
   unchanged unless the session's optional AGE pair (Task 8 Step 6) is measured; then
   `GC_TAPE2 = round(Δcpu% × 1030 × 100 / (65 × d))` with `d` the styles' factor at the session's CPU TEST (0.9's
   derivation: 8 % → 92), and `GC_TAPE1 = round(GC_TAPE2 × 139 / 169)`.
7. **The fit's host target.** Spec: "at most 1 in 10 pieces late". On the rows: at most one row in ten (of all rows)
   late but within the budget (it would still be played as is), and at most a quarter of the clean rows over it
   (thinned for nothing). If no grid point meets both, Task 9 stops and reports the best point (the controller decides).
8. **The hint (item 5).** Said where GEN opens (`ui_lens.c nav_go`, the branch that calls `gen_open`): HOME's taps,
   SELECT on HOME, HOME + SELECT, and HOME from another section when HOME remembers GEN. Not when GEN is refused (STOP
   TO EDIT). `ux_home_flip` no longer says it (no double message).
9. **The dice level (item 6).** `dk_keep` (taken once, when the dialog opens) keeps `P_LEVEL` too; `dk_back` puts it
   back. SELECT dicing again inside the dialog does not retake it: OCT− goes to the level before the first dice, as the
   steps do.
10. **The bend (item 7).** The routing signature `events_block` watches cannot hold the selection (6 × 5 bits + 1 are
    in use), so a byte `midi_rsel` beside it: with KEYBOARD PLAYS SELECTED on, a new selection runs `midi_reroute`,
    whose last loop already zeroes the bend of every track that has no channel of its own and is not the selected one.
    The newly selected track starts unbent (its bend comes with the next bend message).
11. **BREAK (item 8).** README: the intro's "chopped breakbeats you load yourself," goes; roadmap item 4 goes and the
    list is renumbered 1–5. Landing: the tagline's "chopped breaks," and its roadmap item go. Past entries stay (README
    "128 KiB of flash reserved for breaks" in the 0.2 notes, the manual's 0.2 news). No new roadmap item is written
    (not in the spec).
12. **The installer warning (item 9).** The whole `dual` paragraph goes, in all three languages, with its `<p>`; no
    replacement text (two cores are proven since 0.1; the hold-REC-for-one-core tip is in the manual, `docs/MANUAL.md`
    "CPU CORES").
13. **The renders (item 10).** Chromium's command line only: `--dump-dom` of a copy with a one-line script measures
    the page's height, `--screenshot` at `--window-size` / `--force-device-scale-factor` makes the PNGs, `--print-to-pdf`
    of a copy with an `@page` rule the size of the page makes the PDFs; the map's PNG from the SVG at 2x with a
    transparent background, 256 colours as `gen_nav_map.py` does. `tools/render_cheatsheets.mjs` (Playwright only) is
    retired; `gen_nav_map.py` renders its PNG through the new tool. If Chromium cannot render here, report and leave.
14. **`save_on` / `LZ_SAVE` (item 11).** `save_on()` still feeds `lz_on()` (the SAVE pages' geometry). The deferred note's
    own condition: removed only if every UI render is byte-identical without it (Task 6 Step 1). Otherwise it stays and
    the task says so.
15. **Long lines (item 13).** Code and comments wrapped to ≤ 120 columns with the code unchanged (comment-stripped,
    whitespace-stripped text equal before and after). `flash-map.md`'s long lines are table rows: a Markdown table row
    cannot wrap, so only its prose lines are wrapped (the table stays one row a line).
16. **The missing goldens (item 14).** `regress` then fails on a render with no golden line (today it only prints
    "N renders not in"), so the gap cannot come back silently.

## Codebase map (inventory at b3f8a80; line numbers verified)

- Safety net: `firmware/src/voice.c` `SAFE_*` `:306-309`, `fx_safe` `:310`, `safe_part` `:311`, `safe_hushed` `:312`,
  `safe_clear` `:316-323`; `firmware/src/audio.c` ladder comment `:164-180`, `dsf_rearm` `:186`, `dsf_since` `:188`,
  `fx_safety_tick` `:214-262` (release stamp `:254`); callers of `safe_clear`: `project.c:1958`
  (`project_restore_runtime`), `project.c:2064` (`system_init`), `song_dice.c:94` (`cpu_test_song`); `gen.c gen_build`
  `:2920-2943` (arm `:2928-2929`), `dice_seq` `:3055-3056`; `gen_parts` `:1734-1866` (`dice_secs |= 1u` `:1760`,
  `dice_secs |= 1u << k` `:1844`, `section_load(0)` `:1849`).
- Songs: `firmware/src/song_main.c` `song_keep` `:7`, `chain_prepare` `:126-149` (`chain.kept` `:137`),
  `section_load` `:151-170`, `section_store` `:203-221` (`dice_secs &=` `:207`), `song_switch` `:325-371`
  (`dice_secs = 0` `:341`), `song_poll` `:378-448` (SONG END `:418-430`); `song_chain.c:24` `dice_secs`, `:25-29`
  `NSONG`; `storage.c:16-20` `NSONG`; `song_play.c sec_apply` `:9-46` (ISR, `live.cur` `:43`).
- Autosave tick: `project.c proj_music_sig` `:2598-2620` (`as_mix`, `motion_base_value`), `autosave_poll` `:2665-2700`
  (250 ms block `:2668-2673`).
- GEN: `gen.c` `genid_t` `:39-45`, `gp` `:47-61`, `gs` `:96-116` (`cost_thin` `:112`, `grain_off` `:114`, `in_sec`
  `:115`), CPU model `:2037-2199` (`GC_ENG` / `GC_VOX` `:2055-2058`, `GC_CCOMP` `:2065`, `GC_TAPE1/2` `:2086-2087`,
  `GC_GEN_X100` `:2116`, `GC_WILD_X100` `:2117`, `GC_DEV2_X100` `:2118`, `GC_DEV2W_X100` `:2119`, `gc_dev_of`
  `:2126-2132`, `gcf` `:2137`, `gc_budget` `:2139-2143`, `gc_cost` `:2156-2199`), `gen_fit_tape` `:2311-2318`,
  `gx_fit_piece` `:2337-2366` (`gs.grain_off` `:2350`), `gen_make` `:2602` (`memset(&gs` `:2611`), `gid_name`
  `:2886`, `gid_eq` `:2905`.
- Console: `firmware/src/console.c con_status` `:197-250` (only in the firmware; no host test builds `console.c`).
- HOME / GEN hint: `firmware/src/ux_nav.c ux_home_flip` `:29-35`, `ux_home_hint` `:54-58`; `ui_lens.c` forward
  declarations `:224-230`, `nav_go` `:234-275` (GEN opens `:252-258`), `nav_button` `:290-306`; `ui_input.c:2252`
  (HOME + SELECT → `nav_go(SEC_HOME, UXH_GEN0, 1)`).
- Dice: `ui_input.c` `dk` `:1812-1816`, `dk_keep` `:1889-1901`, `dk_back` `:1903-1922`, `part_dice` `:1960-1983`;
  `song_dice.c dlevel` `:58-69`.
- MIDI: `seq.c midi_claims` `:2270`, `midi_in_mask` `:2278`, `midi_rsig` `:2283`, `midi_heard` `:2284`, `events_block`
  `:2367` (routing watch `:2400-2408`); `midi_control.c midi_reroute` `:414-440` (bend zeroed `:435-439`).
- Tests: `tests/gen_test.c` (`gen_reset` `:46`, `gen_with` `:52`, safety-net block `:1921-1966`, the TAPE sweep and
  the 4 % bound `:1805-1856`, `GT_X1` `:23`, `GEN_HOUSE_HASH` `:219-223`, `grain_off` read `:1708`, `cost_thin` read
  `:1838`, final printf `:1985`); `tests/ui_test.c` (`ui_power_on` `:179-250`, `frame` `:297`, `press` `:304`, `turn`
  `:323`, `held_turn` `:329`, `msg_is` `:347`, `ux_home_at` `:10756`, `test_ux_nav_home` `:10761` (hint `:10819`),
  `test_u1_dice` `:20376-20450` (SEQ back check `:20432`), registry `:22188`, `:22403`); `tests/audio_test.c`
  (`safe_reset` `:396`, `safe_run` `:414`, `safety` `:425`, "new music" check `:599-601`); `tests/midi_control_test.c`
  (`midi_test_reset` `:9`, `queued` `:20`, the KEYBOARD PLAYS SELECTED block `:483-490`); `tests/regress.c` (exit
  `:1347`); `tests/run_tests.sh` (`run` `:164`, python tests `:414-417`, node `:419-425`).
- Docs and pages: `README.md` intro `:5-7`, Status `:13`, roadmap `:126-133`; `web/landing/index.html` tagline `:48`,
  roadmap `:57-64`; `web/index_pkg.html` `dual` `:47`, `:107`, `:155`, `:333`; `docs/cheatsheet/*.html`,
  `docs/cheatsheet-{controls,sound}.{png,pdf}` (last rendered cdae0ae), `docs/navigation-map.svg` (1500 × 2684) and
  `.png` (3000 × 5424, stale since aec409f); `tools/render_cheatsheets.mjs`, `tools/gen_nav_map.py` `RENDER_JS`
  `:565-577`, `main` `:580-605`; `tools/gen_cheatsheets.py:5-6`.
- Dead code / tools / tidy: `ui.c save_on` `:641`, `lz_on` `:659`, `lz_fam` `:661-669`; `ui_lens.c:25` `LZ_SAVE`;
  `tools/gen_aa_keycaps.py` knob arcs `:5-6, :22-25, :51, :100-115, :134, :148-159`; `tools/gen_about_qr.py:11`;
  `tools/build.py:296-303` (`isdigit`); `tests/ux_msg_scan.py:85-92` (`_LBL`); `.github/workflows/emulator.yml:134-135`;
  `docs/braindancer/baseline.md:1`; `docs/braindancer/flash-map.md` (20 lines > 120, 19 of them table rows);
  `firmware/src/editor.c` (19 lines > 120), `firmware/src/felucca.c:99-100`.
- Version and release: `firmware/src/felucca.c:42` `FELUCCA_VERSION "v0.9"`; `web/manual/data.js:617` `news`;
  `web/STABLE` (empty).

## File map

| File | Change |
|---|---|
| `firmware/src/voice.c` | `dsf_since` / `dsf_rearm` moved here; `safe_gen`, `safe_secs`, `safe_live`, `safe_arms`, `safe_arm`, `safe_edit`; `safe_clear` stamps (Task 1) |
| `firmware/src/audio.c` | `dsf_since` / `dsf_rearm` definitions gone (moved), comment (Task 1) |
| `firmware/src/project.c` | `trk_edit_sig`, `safe_edit_poll`, called by `autosave_poll` (Task 1) |
| `firmware/src/song_main.c` | `section_load` re-arms, SONG END re-arms (`song_keep_gen`), `section_store` / `song_switch` clear `safe_secs` (Task 1) |
| `firmware/src/gen.c` | `gs.wrote`; `gen_build` arms; `dice_seq` → `safe_edit` (Task 1); `gen_stat_t`, `gen_status`, `gen_id_fmt` (Task 3); `GEN_PROBE` (Task 6); `gc_terms_t`, `gc_terms`, `gc_sum`, the re-fitted constants (Task 9) |
| `firmware/src/ui_lens.c`, `ux_nav.c` | the hint where GEN opens (Task 2) |
| `firmware/src/ui_input.c` | `dk.level` (Task 2) |
| `firmware/src/seq.c` | `midi_rsel` (Task 2) |
| `firmware/src/console.c` | `gen_piece`, `gen_cost`, `gen_budget`, `gen_age`, `gen_id` (Task 3) |
| `firmware/src/ui.c`, `ui_lens.c` | `save_on` / `LZ_SAVE` if dead (Task 6) |
| `firmware/src/storage.c` | the NSONG tie (Task 6) |
| `firmware/src/editor.c`, `felucca.c` | long lines (Task 6); `felucca.c` version (Task 10) |
| `tools/cal_snap.sh`, `tools/cal_piece.sh`, `tools/gen_cal.py` (new) | the device capture and its CSV (Task 3, used in Tasks 8, 9, 10) |
| `tools/render_chromium.py` (new); `tools/render_cheatsheets.mjs` (deleted); `tools/gen_nav_map.py`, `tools/gen_cheatsheets.py` | Task 5 |
| `tools/gen_aa_keycaps.py`, `tools/gen_about_qr.py`, `tools/build.py` | Task 6 |
| `tools/gen_fit.py` (new) | Task 9 |
| tests: `tests/audio_test.c`, `tests/gen_test.c`, `tests/ui_test.c`, `tests/midi_control_test.c`, `tests/regress.c`, `tests/run_tests.sh`, `tests/about_qr_test.py`, `tests/ux_msg_scan.py` | per task |
| new tests: `tests/gen_cal_test.py` (T3), `tests/stale_pages_test.py` (T4), `tests/build_py_test.py`, `tests/nsong_tie.c`, `tests/nsong_tie_test.sh` (T6), `tests/gen_terms.c`, `tests/gen_fit_test.py`, `tests/gen_cal_rows.h` (T9) | |
| `tests/golden.txt` | the missing lines only (Task 7) |
| `README.md`, `web/landing/index.html`, `web/index_pkg.html`, `web/test_web.mjs` | Task 4 |
| `docs/cheatsheet-*.png/.pdf`, `docs/navigation-map.png` | Task 5 (rendered) |
| `.github/workflows/emulator.yml`, `docs/braindancer/baseline.md`, `docs/braindancer/flash-map.md` | Task 6 |
| `docs/braindancer/gen-calibration-0.9.1.csv`, `docs/braindancer/device-log.md` | Task 8 (controller) |
| `docs/braindancer/gen-cal-rows.csv`, `docs/braindancer/gen-calibration-0.9.1.md` | Task 9 |
| `README.md`, `web/manual/data.js`, `docs/MANUAL.md`, `web/landing/index.html`, `web/manual/nav.js`, `docs/braindancer/device-log.md`, new `docs/superpowers/plans/2026-10-31-braindancer-0.9.1-deferred.txt`, `web/STABLE` | Task 10 |

---

### Task 1: The safety net follows the GEN piece (items 2, 3, 4)

Must come first: the calibration (Task 8) measures with the net as it will ship.

**Files:** Modify `firmware/src/voice.c:301-323`, `firmware/src/audio.c:164-190`, `firmware/src/gen.c` (`gs` `:96-116`,
`gen_parts` `:1760`, `:1844`, `gen_build` `:2928-2932`, `dice_seq` `:3055-3056`), `firmware/src/song_main.c` (`:7`,
`chain_prepare` `:137`, `section_load` `:158-160`, `section_store` `:207`, `song_switch` `:341`, SONG END `:424-429`),
`firmware/src/project.c` (after `proj_music_sig` `:2620`; `autosave_poll` `:2668-2673`). Test `tests/audio_test.c`
(in `safety`, after `:601`), `tests/gen_test.c` (the block `:1921-1966` and a new block after it), `tests/ui_test.c`
(`ui_power_on`).

**Interfaces — Produces** (later tasks rely on these names):
```c
/* voice.c */
static uint16_t dsf_since[4];                 /* moved from audio.c, same initialiser {0xFFFF x4} */
static uint8_t dsf_rearm[4];                  /* moved from audio.c */
static uint8_t safe_gen[2];                   /* the GEN piece's texture and arp tracks (NTRK: none / edited) */
static uint8_t safe_secs;                     /* bit s: section s written by that piece */
static uint8_t safe_live;                     /* the music now is that piece (Task 3: gen_status) */
static uint8_t safe_arms;                     /* counts safe_arm calls (safe_edit_poll retakes its signatures) */
static void safe_arm(uint32_t on);            /* safe_part = on ? safe_gen : none; safe_live = on */
static void safe_edit(uint32_t k);            /* track k out of safe_gen and safe_part */
/* gen.c */
gs.wrote                                      /* uint8_t: the sections gen_parts wrote for this piece */
/* project.c */
static uint32_t trk_edit_sig(uint32_t k);
static void safe_edit_poll(void);             /* autosave_poll's 250 ms tick; tests call it directly */
/* song_main.c */
static uint8_t song_keep_gen;
```

- [ ] **Step 1: Write the failing tests.**

  (a) `tests/audio_test.c`, in `safety()`, right after `check("  (0.8) new music (safe_clear): TAPE's rung off", !fx_safe);`:
```c
        {   /* (0.9.1 spec 4) new music (safe_clear) drops the levers on as a calm release does: each stamped released, so
             * one on again within 5 s takes the longer calm (4 s) and the net cannot flap once on the next overload */
            uint32_t h;
            int ok;
            safe_reset(4u, LOAD_LITE);
            safe_part[0] = 5u;
            for (h = 0; h < 400u; h++) safe_run(1u, 125u, 0);
            ok = (fx_safe & (SAFE_DLY | SAFE_TEX)) == (SAFE_DLY | SAFE_TEX);
            safe_clear();
            ok &= !fx_safe && dsf_since[0] == 0u && dsf_since[1] == 0u && dsf_since[2] == 0xFFFFu && dsf_since[3] == 0xFFFFu;
            safe_part[0] = 5u;                          /* (re-armed: a section of the piece loaded) */
            for (h = 0; h < 400u; h++) safe_run(1u, 125u, 0);
            printf("audio: after safe_clear, the delay's and the texture's levers on again: calm x%u, x%u\n",
                   1u << dsf_rearm[0], 1u << dsf_rearm[1]);
            check("  (0.9.1) new music (safe_clear): the levers on stamped released; on again within 5 s: the 4 s calm",
                  ok && (fx_safe & (SAFE_DLY | SAFE_TEX)) == (SAFE_DLY | SAFE_TEX) && dsf_rearm[0] == 2u && dsf_rearm[1] == 2u);
        }
```
  (b) `tests/gen_test.c`, the existing block "(ACID FM 0.5.2 review I1, M1)" (`:1921-1966`): a section load of GEN's
  own section now re-arms (spec item 2 reverses this check). Replace the two lines
```c
        fx_safe = SAFE_TEX;
        ok_sec = !section_load(0) && safe_part[0] == NTRK && safe_part[1] == NTRK && !fx_safe;
```
  with
```c
        {   uint8_t k0 = safe_part[0], k1 = safe_part[1];   /* (0.9.1 spec 2) a section the piece wrote: armed again, levers off */
            fx_safe = SAFE_TEX;
            ok_sec = !section_load(0) && safe_part[0] == k0 && safe_part[1] == k1 && !fx_safe; }
```
  and in its `gcheck` text replace `"a section load (a project load), a part diced "` with `"(0.9.1: a section of the
  piece loaded: armed again, the levers off), a part diced "`.

  (c) `tests/gen_test.c`, a new block right after that block's closing `}`:
```c
    {   /* (0.9.1 spec 2, 3) the safety net's parts follow the GEN piece: armed again when one of its sections loads and
         * when SONG END brings it back; not for a section stored over (yours) or another project's; a track the user
         * edits (its steps, its sound, its engine) out for the rest of the piece; a mute is not an edit */
        uint32_t sd, a0 = NTRK, a1 = NTRK, s0;
        int ok_load, ok_mine, ok_end, ok_end_other, ok_switch, ok_keep, ok_steps, ok_rest, ok_regen, ok_sound, ok_engine, ok_ui;
        mc_host_v12 = 1; gt_cpt = 74; gen_reset();
        for (sd = 0; sd < 200u; sd++) {                 /* (a PARTS piece that plays both, its sections A B C written) */
            gen_with(1, sd * 7u + 1u, 50, 1);
            if (safe_part[0] < NTRK && safe_part[1] < NTRK && (gs.wrote & 7u) == 7u) break;
        }
        a0 = safe_part[0]; a1 = safe_part[1];
        fx_safe = SAFE_TEX;
        ok_load = a0 < NTRK && a1 < NTRK && safe_live && safe_secs == gs.wrote &&
                  !section_load(1) && safe_part[0] == a0 && safe_part[1] == a1 && !fx_safe && safe_live;
        ok_load &= !section_load(0) && safe_part[0] == a0 && safe_part[1] == a1;
        section_store(1);                               /* (B stored over: the user's now) */
        ok_mine = !section_load(1) && safe_part[0] == NTRK && safe_part[1] == NTRK && !safe_live;
        ok_mine &= !section_load(0) && safe_part[0] == a0 && safe_part[1] == a1 && safe_live;   /* (A still the piece's) */
        gen_with(1, gp.seed, 50, 1);                    /* SONG END: the music before PLAY back, the net with it */
        ok_end = !chain_prepare();
        transport_req = 0; chain.armed = 0; chain.ended = 1; song.playing = 0; fx_safe = SAFE_ARP;
        song_poll();
        ok_end &= msg_is("SONG END") && safe_part[0] == a0 && safe_part[1] == a1 && !fx_safe && safe_live;
        gen_with(1, gp.seed, 50, 1);                    /* .. not when the music before PLAY was not the piece */
        safe_clear();
        ok_end_other = !chain_prepare();
        transport_req = 0; chain.armed = 0; chain.ended = 1; song.playing = 0;
        song_poll();
        ok_end_other &= safe_part[0] == NTRK && safe_part[1] == NTRK && !safe_live;
        gen_with(1, gp.seed, 50, 1);                    /* another project: none of these sections the piece's */
        ok_switch = !song_switch(1u, 0) && !safe_secs;
        ok_switch &= !song_switch(0u, 0);
        /* the user's edits (safe_edit_poll: autosave_poll's 250 ms tick; its first pass after an arm takes the signatures) */
        gen_with(1, gp.seed, 50, 1);
        safe_edit_poll();
        trk[a0].p[P_MUTE] = 1; safe_edit_poll();
        ok_keep = safe_part[0] == a0 && safe_part[1] == a1;   /* (a mute: no edit) */
        trk[a0].p[P_MUTE] = 0;
        trk[a0].step[0].vel ^= 1u; safe_edit_poll();    /* a step edited on the texture */
        ok_steps = safe_part[0] == NTRK && safe_part[1] == a1 && safe_gen[0] == NTRK;
        ok_rest = !section_load(1) && safe_part[0] == NTRK && safe_part[1] == a1;   /* (for the rest of the piece) */
        gen_with(1, gp.seed, 50, 1);
        ok_regen = safe_part[0] == a0 && safe_part[1] == a1;   /* (the next piece: its own parts) */
        safe_edit_poll();
        apply_preset_to(&trk[a1], (trk[a1].preset + 1u) % 4u); safe_edit_poll();   /* the arp's sound */
        ok_sound = safe_part[1] == NTRK && safe_part[0] == a0;
        gen_with(1, gp.seed, 50, 1); safe_edit_poll();
        set_engine_of(&trk[a0], trk[a0].eng_req == ENGI_ANALOG ? ENGI_LOFI : ENGI_ANALOG); safe_edit_poll();   /* its engine */
        ok_engine = safe_part[0] == NTRK && safe_part[1] == a1;
        gen_with(1, gp.seed, 50, 1); fm1_ms += 300u; autosave_poll();   /* the panel's way, through the real tick */
        song.sel = (uint8_t)a1; go_home(); frame(); press(B_EDIT); frames(64);
        s0 = trk_edit_sig(a1);
        turn(EN_K1, 3); frames(64);
        if (trk_edit_sig(a1) == s0) { turn(EN_K1, -6); frames(64); }
        fm1_ms += 300u; autosave_poll();
        ok_ui = trk_edit_sig(a1) != s0 && safe_part[1] == NTRK && safe_part[0] == a0;
        if (!(ok_load && ok_mine && ok_end && ok_end_other && ok_switch && ok_keep && ok_steps && ok_rest && ok_regen &&
              ok_sound && ok_engine && ok_ui))
            printf("gen:   (0.9.1 net) load %d mine %d end %d end-other %d switch %d keep %d steps %d rest %d regen %d "
                   "sound %d engine %d ui %d\n", ok_load, ok_mine, ok_end, ok_end_other, ok_switch, ok_keep, ok_steps,
                   ok_rest, ok_regen, ok_sound, ok_engine, ok_ui);
        bad += gcheck("(0.9.1 spec 2) the net's parts again when a section of the piece loads and after SONG END; not for "
                      "a section of yours, nor after another project", ok_load && ok_mine && ok_end && ok_end_other && ok_switch);
        bad += gcheck("(0.9.1 spec 3) a track the user edits (steps, sound, engine; the panel too) out of the net for the "
                      "rest of the piece; a mute is not an edit", ok_keep && ok_steps && ok_rest && ok_regen && ok_sound &&
                      ok_engine && ok_ui);
        fx_safe = 0; mc_host_v12 = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
```
  (d) `tests/ui_test.c ui_power_on`, after `transport_req = 0;` (`:229`): the power-on's net, as the firmware's
  statics start:
```c
    safe_clear();                                 /* (0.9.1) the safety net as at power-on: no GEN piece, no parts, no */
    safe_gen[0] = safe_gen[1] = NTRK;             /* sections of one */
    safe_secs = 0;
```

- [ ] **Step 2: Run to see them fail.**
  `tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/audio_test tests/audio_test.c -lm && build/host/audio_test | grep -a -E "0.9.1|FAIL|passed"`
  — expected: the build fails or the new check FAILs (`dsf_since` 0xFFFF after `safe_clear`, calm x1).
  `tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/gen_test tests/gen_test.c -lm` — expected: does
  not build (`safe_gen`, `safe_secs`, `safe_live`, `gs.wrote`, `trk_edit_sig`, `safe_edit_poll` undeclared). That is
  the red state.

- [ ] **Step 3: Implement.**

  `voice.c`: delete nothing above `fx_safe`; replace `:310-323` (from `static volatile uint8_t fx_safe;` to the end of
  `safe_clear`) with:
```c
static volatile uint8_t fx_safe;
static uint8_t safe_part[2] = {NTRK, NTRK};             /* the GEN piece's texture and arp tracks (NTRK: none) */
/* (audio.c fx_safety_tick) each lever's calm (1 s << it) and the halves since its release ([0] delay [1] texture [2] arp
 * [3] TAPE). (0.9.1) Here, before audio.c, so safe_clear stamps them (no ISR code changed) */
static uint8_t dsf_rearm[4];
static uint16_t dsf_since[4] = {0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu};
/* (0.9.1 spec 2, 3) the GEN piece the net belongs to: its texture and arp tracks (a track the user edits: out, for the
 * rest of the piece; project.c safe_edit_poll), the sections it wrote (bit s; a section stored over and another
 * project's: not; song_main.c), the music now that piece (gen_build; one of its sections loaded; SONG END back to it),
 * and a count of the arms (safe_edit_poll takes the parts' signatures again) */
static uint8_t safe_gen[2] = {NTRK, NTRK};
static uint8_t safe_secs, safe_live, safe_arms;
static uint32_t safe_hushed(uint32_t k)                 /* track k hushed by the safety net */
{
    return ((fx_safe & SAFE_TEX) && k == safe_part[0]) || ((fx_safe & SAFE_ARP) && k == safe_part[1]);
}
/* other music in (a project, a section, UNDO, NEW PROJECT, the editor's restore): no part of it the net may hush (they
 * are the user's), the levers off (the new music starts as it is; the net comes on again if it must). (0.9.1 spec 4)
 * Each lever on is stamped as released (dsf_since 0, as a calm release does, audio.c): on again within 5 s it takes
 * the longer calm, so it never flaps once. The audio ISR off or not running */
static void safe_clear(void)
{
    uint32_t l, on = fx_safe;
    for (l = 0; l < 4u; l++)
        if (on & (l == 3u ? SAFE_TAPE : 1u << l))
            dsf_since[l] = 0;
    safe_part[0] = safe_part[1] = NTRK;
    safe_live = 0;
    fx_safe = 0;
}
/* (0.9.1 spec 2) the GEN piece's parts the net may hush again (on: the music is the piece, or one of its sections), or
 * none (another music); after the load's safe_clear */
static void safe_arm(uint32_t on)
{
    safe_part[0] = on ? safe_gen[0] : (uint8_t)NTRK;
    safe_part[1] = on ? safe_gen[1] : (uint8_t)NTRK;
    safe_live = (uint8_t)(on != 0u);
    safe_arms++;
}
/* (0.9.1 spec 3) track k rewritten by the user (or diced): never hushed by the net for the rest of the piece */
static void safe_edit(uint32_t k)
{
    uint32_t i;
    for (i = 0; i < 2u; i++) {
        if (safe_gen[i] == k) safe_gen[i] = NTRK;
        if (safe_part[i] == k) safe_part[i] = NTRK;
    }
}
```
  `audio.c`: delete the two definitions `static uint8_t dsf_rearm[4]; ...` (`:186`) and `static uint16_t dsf_since[4] =
  ...` (`:188`) (now in voice.c); in the ladder comment (`:179-180`) replace `New music (voice.c safe_clear): all off,
  TAPE back` with `New music (voice.c safe_clear): all off, TAPE back, each lever on stamped as released (0.9.1)`.

  `gen.c`: in `gs` after `uint8_t in_sec;` (`:115`) add
  `uint8_t wrote;                                      /* (0.9.1) the sections gen_parts wrote (bit s: the net's, safe_secs) */`;
  in `gen_parts` after `dice_secs |= 1u;` (`:1760`) add `gs.wrote |= 1u;` and after `dice_secs |= (uint8_t)(1u << k);`
  (`:1844`) add `gs.wrote |= (uint8_t)(1u << k);`. In `gen_build` replace the two `safe_part[...] = ...` statements and
  their comment (`:2928-2932`) with:
```c
    safe_gen[0] = gs.act[RL_TEX] && gs.trk_of[RL_TEX] < trk_live ? gs.trk_of[RL_TEX] : (uint8_t)NTRK;   /* (ACID FM 0.5.2) */
    safe_gen[1] = gs.act[RL_ARP] && gs.trk_of[RL_ARP] < trk_live ? gs.trk_of[RL_ARP] : (uint8_t)NTRK;   /* the parts the
                                                         * safety net hushes (audio.c): the piece's texture and arp, if it
                                                         * plays them; (0.9.1) kept for its sections (safe_secs) */
    safe_secs = gs.wrote;
    safe_arm(1u);
```
  In `dice_seq` replace
```c
    if (safe_part[0] == k || safe_part[1] == k)         /* (ACID FM 0.5.2) a part diced on it: the safety net's no more */
        safe_part[safe_part[1] == k] = NTRK;
```
  with `safe_edit(k);                                       /* (ACID FM 0.5.2; 0.9.1 for the rest of the piece) a part diced on it */`.

  `song_main.c`: after `static project_store_t song_keep ...` (`:7`) add
  `static uint8_t song_keep_gen;                    /* (0.9.1) the music before PLAY was the GEN piece: SONG END arms its net */`;
  in `chain_prepare` after `chain.kept = (uint8_t)proj_pack(&song_keep, &proj_scratch);` add `song_keep_gen = safe_live;`;
  in `section_load` after `if (rc)\n        return 1;` add
```c
    safe_arm((safe_secs >> (s & 3u)) & 1u);             /* (0.9.1 spec 2) a section the GEN piece wrote: its net again */
```
  in `section_store` after `dice_secs &= (uint8_t)~(1u << s);` add `safe_secs &= (uint8_t)~(1u << s);   /* (0.9.1) yours: not the GEN piece's */`;
  in `song_switch` after `dice_secs = 0;` add `safe_secs = 0;                                    /* (0.9.1) .. nor the GEN piece's */`;
  in the SONG END branch after `str_cpy(proj_name, name, PROJ_NAME_LEN + 1u);` add
  `safe_arm(song_keep_gen);                      /* (0.9.1 spec 2) the GEN piece back: its net too */`.

  `project.c`, right after `proj_music_sig` (after `:2620`):
```c
/* (0.9.1 spec 3) what a user's edit of track k changes: its steps, its sound (every parameter at its base: automation
 * playing is not an edit; not its mute, a section's or a scene's), its engine and preset, its FM6 patch */
static uint32_t trk_edit_sig(uint32_t k)
{
    const track_t *t = &trk[k % NTRK];
    uint32_t h = 2166136261u, j;
    for (j = 0; j < P_COUNT; j++)
        if (j != P_MUTE) {
            int16_t v = motion_base_value(t, j);
            h = as_mix(h, &v, sizeof v);
        }
    h = as_mix(h, &t->eng_req, 1);
    h = as_mix(h, &t->preset, 1);
    h = as_mix(h, t->step, sizeof t->step);
    return as_mix(h, fm6_patch[k % NTRK], sizeof fm6_patch[k % NTRK]);
}
/* (0.9.1 spec 3) autosave_poll's 250 ms tick: the net's parts' signatures taken when it is armed again or another
 * section plays (a song's next row); a part whose signature moved since was edited: out of the net for the rest of the
 * piece (safe_edit). Any source counts: the panel, the web editor, a controller's CC, MUTATE. An edit in the first
 * 250 ms after the arm is taken as the piece's */
static uint32_t safe_sig[2];
static uint8_t safe_seen_arms;
static int8_t safe_seen_sec = -2;
static void safe_edit_poll(void)
{
    uint32_t i, k;
    if (safe_seen_arms != safe_arms || safe_seen_sec != live.cur) {
        safe_seen_arms = safe_arms;
        safe_seen_sec = live.cur;
        for (i = 0; i < 2u; i++)
            safe_sig[i] = safe_part[i] < NTRK ? trk_edit_sig(safe_part[i]) : 0u;
        return;
    }
    for (i = 0; i < 2u; i++)
        if ((k = safe_part[i]) < NTRK && trk_edit_sig(k) != safe_sig[i])
            safe_edit(k);
}
```
  and in `autosave_poll`'s 250 ms block, after `proj_fill_poll(sig_rows(m));`, add `safe_edit_poll();`. If `as_mix`
  or `motion_base_value` is declared after `proj_music_sig`, place the new code after both (they are used by
  `proj_music_sig`, so they precede it).

- [ ] **Step 4: Run to see them pass.** `audio_test`, `gen_test`, `ui_test` (build lines as Step 2; `ui_test`:
  `tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/ui_test tests/ui_test.c -lm && build/host/ui_test | grep -a -E "FAIL|passed"`):
  all pass. If `ok_ui` fails because K1 on EDIT's first page moves no parameter of the track, use the first knob that
  does on that page (`turn(EN_K2, ...)`), never drop the check. `./build.sh` (image ≥ 16 KiB free, the `layout:`
  line, RAM); `python3 tests/target_budget.py` (every line unchanged, `fm1_alnk0_irq` 300, `tape_block` 780); the full
  suite (no golden line changes).

- [ ] **Step 5: Commit** — `git add firmware/src/voice.c firmware/src/audio.c firmware/src/gen.c firmware/src/song_main.c firmware/src/project.c tests/audio_test.c tests/gen_test.c tests/ui_test.c && git commit`
  with `fix(gen): the safety net follows the GEN piece through its sections; user edits leave it; cleared levers stamped`;
  body: the three rulings (pre-flight 1–3), the image / RAM / budget lines.

---

### Task 2: GEN's hint, the dice level, the bend (items 5, 6, 7)

**Files:** Modify `firmware/src/ui_lens.c` (`:228-230`, `nav_go` `:234-275`), `firmware/src/ux_nav.c:29-35`,
`firmware/src/ui_input.c` (`dk` `:1812-1816`, `dk_keep` `:1889-1901`, `dk_back` `:1903-1922`), `firmware/src/seq.c`
(`:2283-2284`, `:2400-2408`), `firmware/src/midi_control.c:409-413` (comment). Test `tests/ui_test.c` (two new
functions before `int main`, registered after `bad += test_ux_nav_home();` `:22188`; `test_u1_dice` `:20432`),
`tests/midi_control_test.c` (after `:490`).

**Interfaces — Consumes:** `ux_home_hint()` (`ux_nav.c`), `msg_is`, `held_turn`, `ux_home_at` (tests). **Produces:**
`dk.level` (`int16_t`), `midi_rsel` (`static uint8_t`, seq.c).

- [ ] **Step 1: Write the failing tests.**

  (a) `tests/ui_test.c`, new functions above `int main`:
```c
/* (0.9.1 spec 5) GEN's hint on every way into GEN, not only HOME's taps: SELECT on HOME, HOME + SELECT, HOME from
 * another section when HOME remembers GEN (the message cleared before each, so each way is seen on its own) */
static void msg_clear(void) { ui.msg_t = 0; ui.msg[0] = 0; }
static int test_091_gen_hint(void)
{
    int bad = 0, ok, sel, hs, back, taps;
    ui_power_on(); frame();
    msg_clear();
    turn(EN_SELECT, 1); frame(); fm1_ms += 1000u; turn(EN_SELECT, 1); frame();   /* SELECT: TRACKS VISUAL GEN */
    sel = ux_home_at(UXH_GEN) && msg_is("HOME AGAIN: TRACKS");
    press(B_OCTDN); frame(); msg_clear();                                        /* (OCT-: TRACKS) */
    fm1_ms += 1200u; held_turn(B_HOME, 1); frame();                              /* HOME + SELECT */
    hs = gen_on() && msg_is("HOME AGAIN: TRACKS");
    press(B_FX); frame(); msg_clear();                                           /* (another section) */
    press(B_HOME); frame();                                                      /* HOME: the page it remembers, GEN */
    back = ux_home_at(UXH_GEN) && msg_is("HOME AGAIN: TRACKS");
    ui_power_on(); frame(); msg_clear();
    press(B_HOME); frame(); press(B_HOME); frame();                              /* HOME's taps: as before */
    taps = ux_home_at(UXH_GEN) && msg_is("HOME AGAIN: TRACKS");
    ok = sel && hs && back && taps;
    if (!ok) printf("  GEN's hint: SELECT %d, HOME + SELECT %d, HOME from FX %d, HOME's taps %d (message '%s')\n", sel, hs,
                    back, taps, ui.msg);
    bad += check("0.9.1: GEN's hint (HOME AGAIN: TRACKS) on every way in: SELECT, HOME + SELECT, HOME from another "
                 "section, HOME's taps", ok);
    ui_power_on();
    return bad;
}
/* (0.9.1 spec 6) SEQ's dice, then OCT-: the track's level back too (dice_seq lowers a part that sits high, dlevel) */
static int test_091_dice_level(void)
{
    int bad = 0, ok, lowered = 0;
    uint32_t tries, j;
    track_t *t;
    ui_power_on(); frame(); song.playing = 0;
    song.sel = 2; t = TSEL;
    t->p[P_LEVEL] = 120;
    dk_keep(CF_DICE_SEQ);
    t->p[P_LEVEL] = 70;                                 /* (what dlevel does to a high part) */
    ok = !dk_back(CF_DICE_SEQ, 2u) && t->p[P_LEVEL] == 120;
    for (tries = 0; tries < 8u && !lowered; tries++) {  /* the panel's way: SEQ + SELECT on a part written high, OCT- */
        ui_power_on(); frame(); song.playing = 0;
        song.sel = 2; t = TSEL;
        for (j = 0; j < 16u; j++) {
            step_t *s = &t->step[j];
            memset(s, 0, sizeof *s);
            s->time = ST_NOTE; s->n = 1; s->note[0] = (uint8_t)(84u + j % 5u); s->vel = 100;
        }
        t->p[P_SLEN] = 16; t->p[P_LEVEL] = 120;
        go_home(); frames(30); fm1_ms += 1200u + tries * 7u;
        held_turn(B_SEQ, 1); frame();
        if (ui.confirm == CF_DICE_SEQ && t->p[P_LEVEL] != 120) {
            lowered = 1;
            press(B_OCTDN); frame();
            ok &= !ui.confirm && msg_is("SEQ BACK") && t->p[P_LEVEL] == 120;
        } else {
            press(B_OCTDN); frame();
        }
    }
    if (!lowered) printf("  dice level: no dice in 8 lowered the level (dlevel); raise the notes\n");
    bad += check("0.9.1: SEQ + SELECT then OCT-: the steps, LEN and the level as before the dice", ok && lowered);
    ui_power_on();
    return bad;
}
```
  registered after `bad += test_ux_nav_home();`: `bad += test_091_gen_hint(); bad += test_091_dice_level();`. In
  `test_u1_dice` (`:20432`) the SEQ back condition becomes
  `back = D[i].kind == CF_DICE_SEQ ? !memcmp(s0, TSEL->step, sizeof s0) && TSEL->p[P_SLEN] == p0[P_SLEN] && TSEL->p[P_LEVEL] == p0[P_LEVEL] : u1_same(p0, s0, g0, 0);`
  and its comment's `the level dice_seq sets, dlevel, stays as before 1b` becomes `(0.9.1) the level dlevel set: back too`.

  (b) `tests/midi_control_test.c`, after `host_route_sel(0); events_block(CTL);` (`:490`):
```c
    /* (0.9.1 spec 7) KEYBOARD PLAYS SELECTED: another track selected, the bend of the one it leaves back to 0; a track
     * on its own channel keeps its own */
    midi_test_reset(); host_route_sel(1); song.sel = 2;
    song.g[G_MRIN1 + 1] = 1; song.g[G_MRCH1 + 1] = 7; events_block(CTL);   /* (T2 takes channel 7) */
    queued(0xEB, 127, 127, 1);                         /* channel 12, no track's: the selected T3 bent up */
    queued(0xE6, 127, 127, 1);                         /* channel 7: T2 bent up */
    ok = midi_bend_target[2] == 512 && midi_bend_target[1] == 512;
    song.sel = 4; events_block(CTL);
    bad += check("(0.9.1) KEYBOARD PLAYS SELECTED: another track selected, the one left unbent; a track on its own "
                 "channel keeps its bend", ok && midi_bend_target[2] == 0 && midi_bend_target[4] == 0 &&
                 midi_bend_target[1] == 512);
    host_route_sel(0); song.g[G_MRIN1 + 1] = 0; events_block(CTL);
```

- [ ] **Step 2: Run to see them fail.** `ui_test` (Task 1's line): FAIL on "GEN's hint" (SELECT and HOME + SELECT
  show no message) and on "the level" (`dk_back` leaves 70). `tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/midi_control_test tests/midi_control_test.c -lm && build/host/midi_control_test | grep -a -E "0.9.1|FAIL|passed"`:
  FAIL (T3's bend still 512).

- [ ] **Step 3: Implement.**

  `ui_lens.c`: after `static void ui_message(const char *s);` (`:230`) add
  `static const char *ux_home_hint(void);                /* (ux_nav.c) */`. In `nav_go`, declare `int hint = 0;` with
  the other locals; in the GEN branch after `gen_open();` add `hint = 1;`; replace the HOME branch's
  `page_flash();\n        return;` with
```c
        page_flash();
        if (hint)                                       /* (0.9.1 spec 5) GEN's hint on every way in (its screen has no */
            ui_message(ux_home_hint());                 /* status line: a message), HOME's taps, SELECT, HOME + SELECT */
        return;
```
  `ux_nav.c ux_home_flip`: delete the two lines `if (gen_on()) ... ui_message(ux_home_hint());` and their comment.

  `ui_input.c`: in `dk` add `int16_t level;` after `int16_t slen, v[9], g[6];` (as `int16_t slen, level, v[9], g[6];`);
  `dk_keep`'s SEQ branch: after `dk.slen = t->p[P_SLEN];` add `dk.level = t->p[P_LEVEL];                 /* (0.9.1: dice_seq's dlevel lowers it) */`;
  `dk_back`'s SEQ branch: after `t->p[P_SLEN] = dk.slen;` add `t->p[P_LEVEL] = dk.level;`. The dialog's lost line
  ("OCT- puts it back.") stays true.

  `seq.c`: after `static uint8_t midi_heard[16]; ...` (`:2284`) add
  `static uint8_t midi_rsel;                  /* (0.9.1) .. the selected track then (KEYBOARD PLAYS SELECTED: a new one, midi_reroute) */`;
  in `events_block`'s routing watch replace `if (sig != midi_rsig) {\n            midi_rsig = sig;` with
```c
        if (sig != midi_rsig || (song.g[G_MRSEL] && song.sel % NTRK != midi_rsel)) {   /* (0.9.1 spec 7: the selection
                                                         * moved, its bend zeroed on the track left, midi_reroute) */
            midi_rsig = sig;
            midi_rsel = (uint8_t)(song.sel % NTRK);
```
  `midi_control.c` comment `:409-413`: append `(0.9.1) Also when KEYBOARD PLAYS SELECTED's selection moves: the track
  left loses the bend it took from it.`

- [ ] **Step 4: Run to see them pass.** `ui_test`, `midi_control_test`, `drumx_oct_test` (it uses `host_route_sel`);
  `./build.sh`; `python3 tests/target_budget.py` — `fm1_alnk0_irq` must stay within its line (300 +10 %); if it moved at
  all, report the figure (the check is two byte compares outside any loop); `python3 tests/ux_msg_scan.py build/gen/ux_msg.h`
  is run by the suite; the full suite.

- [ ] **Step 5: Commit** — `git add firmware/src/ui_lens.c firmware/src/ux_nav.c firmware/src/ui_input.c firmware/src/seq.c firmware/src/midi_control.c tests/ui_test.c tests/midi_control_test.c && git commit`
  with `fix(ui): GEN's hint on every way in; SEQ dice's OCT- restores the level; the bend left with the selection`.

---

### Task 3: The console names the GEN piece (item 1a) and the capture tools

**Files:** Modify `firmware/src/gen.c` (after `gid_eq` `:2905-2909`), `firmware/src/console.c` (`con_status`, after
`voices_stolen` `:216`). Create `tools/cal_snap.sh`, `tools/cal_piece.sh`, `tools/gen_cal.py`, `tests/gen_cal_test.py`.
Test `tests/gen_test.c` (new block after Task 1's), `tests/gen_cal_test.py`; `tests/run_tests.sh` (after the
`about_qr_test.py` line `:414`).

**Interfaces — Consumes:** `safe_live` (Task 1), `gc_cost`, `gc_budget`, `gp.last`, `gs.age`, `gs.in_sec`.
**Produces:**
```c
typedef struct { const char *piece; int32_t cost, budget, age; genid_t id; uint8_t on; } gen_stat_t;
static void gen_status(gen_stat_t *s);
static void gen_id_fmt(const genid_t *id, char *b, uint32_t n);   /* "seed two dev form tape main part energy complex" */
```
A `status` reply gains, after `voices_stolen`: `gen_piece <name>`, `gen_cost <n>`, `gen_budget <n>`, `gen_age <n>`,
`gen_id <9 numbers>` (each `-` with no piece). `tools/gen_cal.py` subcommands `delta`, `csv`, `ids`, `rows` (Task 9
uses `ids` and `rows`); the device CSV's columns (`COLS` below).

- [ ] **Step 1: Write the failing tests.**

  (a) `tests/gen_test.c`, a new block after Task 1's:
```c
    {   /* (0.9.1 spec 1) the console's status names the GEN piece playing and the model's figures for it (console.c),
         * and the identity it prints rebuilds the same piece on the host (the calibration's way: tests/gen_terms.c) */
        gen_stat_t st;
        genid_t x;
        char b[64], w[64], nm[28];
        unsigned f[9];
        uint32_t c1, c0;
        int ok;
        mc_host_v12 = 1; gt_cpt = 74; gen_reset();
        gen_status(&st);
        ok = !st.on && str_eq(st.piece, "-") && st.cost < 0 && st.budget < 0 && st.age < 0;   /* (power-on: none) */
        gen_with(1, 421, 50, 1);
        gen_status(&st);
        gen_id_fmt(&gp.id, b, sizeof b);
        snprintf(w, sizeof w, "%u %u %u %u %u %u %u %u %u", gp.id.seed, gp.id.two, gp.id.dev, gp.id.form, gp.id.tape,
                 gp.id.main, gp.id.part, gp.id.energy, gp.id.complex);
        ok &= st.on && str_eq(st.piece, gp.last) && st.cost == (int32_t)gc_cost(1) && st.budget == (int32_t)gc_budget() &&
              st.age == (int32_t)gs.age && gid_eq(&st.id, &gp.id) && str_eq(b, w);
        if (!section_load(1)) {                         /* a section of the piece: its own cost, its muted tracks free */
            gs.in_sec = 1; c1 = gc_cost(1); gs.in_sec = 0;
            gen_status(&st);
            ok &= st.on && st.cost == (int32_t)c1 && !gs.in_sec;
        } else ok = 0;
        system_init();                                  /* another music: none */
        gen_status(&st);
        ok &= !st.on && str_eq(st.piece, "-");
        system_init();                                  /* the identity printed, read back: the same piece */
        gen_with(1u + GEN_SHOWN[3], 77, 60, 1);
        gen_id_fmt(&gp.id, b, sizeof b);
        str_cpy(nm, gp.last, sizeof nm); c0 = gc_cost(1);
        system_init();
        memset(&x, 0, sizeof x);
        ok &= sscanf(b, "%u %u %u %u %u %u %u %u %u", &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6], &f[7], &f[8]) == 9;
        x.seed = f[0]; x.two = f[1]; x.dev = f[2]; x.form = f[3]; x.tape = f[4];
        x.main = (uint8_t)f[5]; x.part = (uint8_t)f[6]; x.energy = (uint8_t)f[7]; x.complex = (uint8_t)f[8];
        gen_build(&x);
        ok &= str_eq(gp.last, nm) && gc_cost(1) == c0;
        bad += gcheck("(0.9.1 spec 1) status: gen_piece (the panel's name, '-' without a piece), gen_cost / gen_budget "
                      "of the music now, gen_age, gen_id that rebuilds the piece", ok);
        mc_host_v12 = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
```
  (b) `tests/gen_cal_test.py` (new):
```python
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
```
  `tests/run_tests.sh`, after the `about_qr_test.py` line:
  `run "GEN calibration data: the log to the device CSV (tools/gen_cal.py)" python3 tests/gen_cal_test.py`

- [ ] **Step 2: Run to see them fail.** `gen_test` does not build (`gen_stat_t`, `gen_status`, `gen_id_fmt`);
  `python3 tests/gen_cal_test.py`: `ModuleNotFoundError: No module named 'gen_cal'`.

- [ ] **Step 3: Implement.**

  `gen.c`, after `gid_eq`:
```c
/* (0.9.1 spec 1) the serial console's status (console.c): the GEN piece playing, named as the panel names it, or "-"
 * (another project, a section of yours, NEW PROJECT, UNDO, CPU TEST: voice.c safe_live), and the CPU model's figures:
 * the cost of the music now (the section playing, its muted tracks free, as a section's fit counts them), the piece's
 * budget and AGE, its identity (to rebuild it on the host: tests/gen_terms.c). Main loop */
typedef struct { const char *piece; int32_t cost, budget, age; genid_t id; uint8_t on; } gen_stat_t;
static void gen_status(gen_stat_t *s)
{
    uint8_t sec = gs.in_sec;
    s->on = (uint8_t)(safe_live && gs.g);
    s->piece = s->on ? gp.last : "-";
    s->id = gp.id;
    gs.in_sec = 1;
    s->cost = s->on ? (int32_t)gc_cost(1) : -1;
    gs.in_sec = sec;
    s->budget = s->on ? (int32_t)gc_budget() : -1;
    s->age = s->on ? (int32_t)gs.age : -1;
}
static void gen_id_fmt(const genid_t *id, char *b, uint32_t n)   /* "seed two dev form tape main part energy complex" */
{
    const uint32_t f[9] = {id->seed, id->two, id->dev, id->form, id->tape, id->main, id->part, id->energy, id->complex};
    uint32_t k, l = 0;
    char d[12];
    b[0] = 0;
    for (k = 0; k < 9u; k++) {
        fmt_int(d, (int32_t)f[k]);
        if (l + str_len(d) + 2u > n) break;
        if (k) b[l++] = ' ';
        str_cpy(b + l, d, n - l);
        l += str_len(d);
    }
}
```
  `console.c con_status`, after `con_kv("voices_stolen", ...)`:
```c
    {   /* (0.9.1 spec 1) the GEN piece playing and the model's figures for it, so every calibration row is named
         * (tools/cal_snap.sh, docs/braindancer/gen-calibration-0.9.1.md); "-" without a piece */
        gen_stat_t g;
        char b[48];
        gen_status(&g);
        con_puts("gen_piece ");
        con_puts(g.piece);
        con_puts("\r\n");
        if (g.on) {
            con_kv("gen_cost", g.cost);
            con_kv("gen_budget", g.budget);
            con_kv("gen_age", g.age);
            gen_id_fmt(&g.id, b, sizeof b);
            con_puts("gen_id ");
            con_puts(b);
            con_puts("\r\n");
        } else
            con_puts("gen_cost -\r\ngen_budget -\r\ngen_age -\r\ngen_id -\r\n");
    }
```
  `tools/cal_snap.sh` (new, `chmod +x`):
```sh
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
```
  `tools/cal_piece.sh` (new, `chmod +x`):
```sh
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
```
  `tools/gen_cal.py` (new):
```python
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
```

- [ ] **Step 4: Run to see them pass.** `gen_test`; `python3 tests/gen_cal_test.py` (`gen_cal: all ok`); `sh -n
  tools/cal_snap.sh && sh -n tools/cal_piece.sh` (syntax only: no serial port here); `./build.sh` (console.c builds:
  the firmware is the only build that has it; image, RAM); the full suite.

- [ ] **Step 5: Commit** — `git add firmware/src/gen.c firmware/src/console.c tools/cal_snap.sh tools/cal_piece.sh tools/gen_cal.py tests/gen_test.c tests/gen_cal_test.py tests/run_tests.sh && git commit`
  with `feat(console): status names the GEN piece (gen_piece, gen_cost, gen_budget, gen_age, gen_id); capture tools`;
  body: pre-flight 4, the image line.

---

### Task 4: BREAK out of the pages; the installer's two-core warning gone (items 8, 9)

**Files:** Modify `README.md:5-7, :126-133`, `web/landing/index.html:48, :57-64`, `web/index_pkg.html:47, :107, :155,
:333`, `web/test_web.mjs` (in the block that reads `index_pkg.html` at `:1688`). Create `tests/stale_pages_test.py`;
`tests/run_tests.sh` (after Task 3's line).

**Interfaces:** none (pages only).

- [ ] **Step 1: Write the failing test** `tests/stale_pages_test.py`:
```python
#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1 spec 8, 9) What the pages say now: BREAK (dropped 2026-10-28) is on neither the README's intro and roadmap nor
the landing page's tagline and roadmap (past entries keep it); the installer has no two-core warning (two cores run on
the FM-1 since 0.1)."""
import re
import sys
from pathlib import Path

R = Path(__file__).resolve().parent.parent
bad = 0


def check(what, ok):
    global bad
    print(f"pages: {what:<72} {'ok' if ok else 'FAIL'}")
    bad += not ok


def no_break(text):
    return not re.search(r"\bBREAK\b", text) and not re.search(r"breakbeat|chopped break", text, re.I)


readme = (R / "README.md").read_text()
intro = readme.split("**[Install]", 1)[0]
road = readme.split("The Braindancer features arrive in steps", 1)[1].split("\n\n")[1]
nums = [int(n) for n in re.findall(r"^(\d+)\. ", road, re.M)]
check("README: the intro without BREAK", no_break(intro))
check("README: the roadmap without BREAK, numbered 1..n", no_break(road) and nums == list(range(1, len(nums) + 1)))
land = (R / "web/landing/index.html").read_text()
tag = re.search(r'<p class="tag">(.*?)</p>', land, re.S).group(1)
steps = re.search(r"<h2>Where it is</h2>\s*<ol>(.*?)</ol>", land, re.S).group(1)
check("landing: the tagline and the roadmap without BREAK", no_break(tag) and no_break(steps))
pkg = (R / "web/index_pkg.html").read_text()
check("installer: no two-core warning (EN / JA / ES)", 'data-t="dual"' not in pkg and not re.search(r"\bdual:", pkg) and
      "0.6.71" not in pkg)
print("pages: all ok" if not bad else f"pages: {bad} FAILED")
sys.exit(bad != 0)
```
  `tests/run_tests.sh`: `run "pages: no BREAK on the README and the landing page, no two-core warning in the installer" python3 tests/stale_pages_test.py`.
  `web/test_web.mjs`, inside the block that reads `index_pkg.html` (`:1688`), after `html` is read:
  `ok(!html.includes('data-t="dual"') && !/\bdual:/.test(html), "installer: no two-core warning (0.9.1: two cores proven since 0.1)");`

- [ ] **Step 2: Run to see it fail.** `python3 tests/stale_pages_test.py`: FAIL on all four checks.

- [ ] **Step 3: Implement.**
  `README.md` intro (`:5-6`): `Braindancer is becoming a firmware with virtual analog, FM, chopped\nbreakbeats you load
  yourself, and a generator that writes whole songs on the device.` → `Braindancer is becoming a firmware with virtual
  analog, FM and a generator that writes whole\nsongs on the device.` (the next sentence unchanged; rewrap the
  paragraph to ≤ 120 columns). Roadmap: delete `4. BREAK: load breakbeats from the web page, chop and rearrange them`,
  renumber `5. Sound (0.8)` → `4.`, `6. The song generator (0.9)` → `5.`. Item 2 "room made for samples" stays (it is
  what 0.2 did).
  `web/landing/index.html:48`: `virtual analog, FM, chopped breaks, songs that write themselves.` → `virtual analog,
  FM, songs that write themselves.`; delete the `<li>BREAK: load breakbeats ...</li>` line.
  `web/index_pkg.html`: delete the `<p data-t="dual" ...></p>` line (`:47`) and the three `dual: "..."` entries
  (`:107`, `:155`, `:333`); check with `grep -n "dual" web/index_pkg.html` that nothing reads `TEXT.*.dual` any more
  (if the apply loop reads keys from the `data-t` attributes only, nothing else changes).

- [ ] **Step 4: Run to see it pass.** `python3 tests/stale_pages_test.py`; `node web/test_web.mjs`; `python3
  tests/built_on_test.py`; `python3 tools/gen_manual.py --check`; `node web/test_manual.mjs` (the landing page is
  not the manual, but it must stay green).

- [ ] **Step 5: Commit** — `git add README.md web/landing/index.html web/index_pkg.html web/test_web.mjs tests/stale_pages_test.py tests/run_tests.sh && git commit`
  with `docs: BREAK off the README and the landing page; the installer's stale two-core warning gone`.

---

### Task 5: The printed cheat sheets and the map's PNG, with the system Chromium (item 10)

**Files:** Create `tools/render_chromium.py`. Modify `tools/gen_nav_map.py` (docstring `:9-16`, `RENDER_JS` `:565-577`,
`main` `:592-601`), `tools/gen_cheatsheets.py:5-6`, `tests/run_tests.sh` (after the `gen_nav_map.py --check` line
`:417`). Delete `tools/render_cheatsheets.mjs`. Rendered: `docs/cheatsheet-controls.png`, `docs/cheatsheet-controls.pdf`,
`docs/cheatsheet-sound.png`, `docs/cheatsheet-sound.pdf`, `docs/navigation-map.png`.

**Interfaces — Produces:** `render_chromium.render_map(svg: Path, png: Path) -> None` (used by `gen_nav_map.py`);
`python3 tools/render_chromium.py [--verify]`.

- [ ] **Step 1: Write the check first** — `tools/render_chromium.py` with `--verify` only working (the renders may be
  stubs that `sys.exit("not yet")` at this step), so the red state is seen:
```python
#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1) The printed cheat sheets (docs/cheatsheet-*.png and .pdf) and docs/navigation-map.png, rendered with the
system's Chromium from its command line, headless (no Playwright): the sizes tools/render_cheatsheets.mjs and
tools/gen_nav_map.py gave them.

  python3 tools/render_chromium.py            render all five files
  python3 tools/render_chromium.py --verify   exit 1 unless each file has the size its source gives now

A cheat sheet: 1600 px wide at 1.5x (the PNG 2400 px wide), the page's whole height (measured: a copy of the page with a
script that puts its height in the title, --dump-dom); the PDF one page of 1600 px x the height + 2, backgrounds
printed (a copy with an @page rule). The map: the SVG's own size at 2x, a transparent background, 256 colours.
CHROMIUM names another Chromium (default /usr/bin/chromium)."""
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image

R = Path(__file__).resolve().parent.parent
CHROME = os.environ.get("CHROMIUM", "/usr/bin/chromium")
SHEETS = ("controls", "sound")
W_SHEET, SCALE_SHEET = 1600, 1.5


def chrome(*args):
    prof = tempfile.mkdtemp(prefix="render-chromium-")
    try:
        return subprocess.run([CHROME, "--headless", "--disable-gpu", "--hide-scrollbars", "--no-first-run",
                               "--allow-file-access-from-files", f"--user-data-dir={prof}",
                               "--virtual-time-budget=5000", *args],
                              check=True, capture_output=True, text=True, timeout=120).stdout
    finally:
        shutil.rmtree(prof, ignore_errors=True)


def copy_of(name, head_extra="", body_extra=""):
    """docs/cheatsheet/NAME.html as a temporary file that still finds its fonts (a <base>), with extras"""
    src = R / "docs" / "cheatsheet" / f"{name}.html"
    html = src.read_text()
    base = f'<base href="{src.parent.as_uri()}/">'
    html = re.sub(r"<head>", "<head>" + base + head_extra, html, count=1)
    html = re.sub(r"</body>", body_extra + "</body>", html, count=1)
    f = tempfile.NamedTemporaryFile("w", suffix=".html", delete=False)
    f.write(html)
    f.close()
    return Path(f.name)


def sheet_height(name):
    p = copy_of(name, body_extra="<script>document.title='H='+document.documentElement.scrollHeight</script>")
    try:
        dom = chrome(f"--window-size={W_SHEET},1000", "--dump-dom", p.as_uri())
    finally:
        p.unlink()
    return int(re.search(r"<title>H=(\d+)</title>", dom).group(1))


def svg_size(svg):
    head = svg.read_text()[:400]
    return int(re.search(r'width="(\d+)"', head).group(1)), int(re.search(r'height="(\d+)"', head).group(1))


def render_sheet(name):
    h = sheet_height(name)
    png, pdf = R / "docs" / f"cheatsheet-{name}.png", R / "docs" / f"cheatsheet-{name}.pdf"
    p = copy_of(name)
    try:
        chrome(f"--window-size={W_SHEET},{h}", f"--force-device-scale-factor={SCALE_SHEET}", f"--screenshot={png}",
               p.as_uri())
    finally:
        p.unlink()
    p = copy_of(name, head_extra=f"<style>@page{{size:{W_SHEET}px {h + 2}px;margin:0}}html{{-webkit-print-color-adjust:"
                                 f"exact;print-color-adjust:exact}}</style>")
    try:
        chrome("--no-pdf-header-footer", f"--print-to-pdf={pdf}", p.as_uri())
    finally:
        p.unlink()
    print(f"{png.relative_to(R)}, {pdf.relative_to(R)}: {W_SHEET} x {h} px")


def render_map(svg, png):
    w, h = svg_size(svg)
    chrome(f"--window-size={w},{h}", "--force-device-scale-factor=2", "--default-background-color=00000000",
           f"--screenshot={png}", svg.as_uri())
    Image.open(png).quantize(256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE).save(png, optimize=True)
    print(f"{Path(png).relative_to(R)}: 2x, {Path(png).stat().st_size // 1024} KB")


def pdf_pages(pdf):
    b = pdf.read_bytes()
    boxes = re.findall(rb"/MediaBox\s*\[\s*0\s+0\s+([\d.]+)\s+([\d.]+)\s*\]", b)
    return len(re.findall(rb"/Type\s*/Page\b", b)), [(float(x), float(y)) for x, y in boxes]


def verify():
    bad = []
    for name in SHEETS:
        h = sheet_height(name)
        png, pdf = R / "docs" / f"cheatsheet-{name}.png", R / "docs" / f"cheatsheet-{name}.pdf"
        w0, h0 = Image.open(png).size
        if w0 != round(W_SHEET * SCALE_SHEET) or abs(h0 - round(h * SCALE_SHEET)) > 2:
            bad.append(f"{png.name}: {w0} x {h0}, the page gives {round(W_SHEET * SCALE_SHEET)} x {round(h * SCALE_SHEET)}")
        n, boxes = pdf_pages(pdf)
        if n != 1 or not boxes or abs(boxes[0][0] - W_SHEET * 0.75) > 1 or abs(boxes[0][1] - (h + 2) * 0.75) > 1:
            bad.append(f"{pdf.name}: {n} pages, {boxes[:1]}, the page gives 1 of {W_SHEET * 0.75} x {(h + 2) * 0.75} pt")
    svg, png = R / "docs" / "navigation-map.svg", R / "docs" / "navigation-map.png"
    w, h = svg_size(svg)
    im = Image.open(png)
    if im.size != (2 * w, 2 * h) or im.mode != "P":
        bad.append(f"{png.name}: {im.size[0]} x {im.size[1]} {im.mode}, the SVG gives {2 * w} x {2 * h} P")
    for b in bad:
        print("render_chromium: FAIL", b)
    print("render_chromium: the renders are the size their sources give" if not bad else "")
    return not bad


def main():
    if not Path(CHROME).exists():
        sys.exit(f"render_chromium: no Chromium at {CHROME} (nothing rendered; nothing may be installed)")
    if "--verify" in sys.argv:
        sys.exit(0 if verify() else 1)
    for name in SHEETS:
        render_sheet(name)
    render_map(R / "docs" / "navigation-map.svg", R / "docs" / "navigation-map.png")


if __name__ == "__main__":
    main()
```
  `tests/run_tests.sh`, after the `gen_nav_map.py --check` line:
```sh
if [ -x "${CHROMIUM:-/usr/bin/chromium}" ]; then
    run "printed cheat sheets and the map's PNG: the size their sources give (render_chromium.py --verify)" python3 tools/render_chromium.py --verify
else
    echo "== skip render sizes (no Chromium)"
fi
```

- [ ] **Step 2: Run to see it fail.** `python3 tools/render_chromium.py --verify`: FAIL at least on
  `navigation-map.png: 3000 x 5424 P, the SVG gives 3000 x 5368 P`, and on each cheat sheet whose height moved since
  cdae0ae. If Chromium cannot run headless here (an error from `chrome()`), stop: report the error, leave the files
  (spec item 10).

- [ ] **Step 3: Render and wire.** `python3 tools/render_chromium.py`. Look at each PNG (Read the image): the fonts
  are Chakra Petch (not a fallback serif), the backgrounds are the sheet's, nothing is cut at the bottom; the
  MASTER box reads `CLIP · PUNCH · DUCK · AGE` (not DUCK TIME on KNOB 4; checked at planning: the sound sheet renders
  2400 × 5148 here with its fonts); the map's PNG shows the
  map on a transparent background. Open each PDF's first page as an image the same way (Read the PDF): one page.
  If fonts fall back, add `--font-render-hinting=none` is **not** the fix: check the `<base>` path and that
  `--allow-file-access-from-files` is passed, then report if still wrong.
  `tools/gen_nav_map.py`: delete `RENDER_JS`; in `main`, replace the PNG block (the `env` / `subprocess.run(['node',
  ...])` / quantize / print lines) with
```python
        if '--svg' not in sys.argv:
            import render_chromium                     # (0.9.1: the system Chromium's command line, no Playwright)
            render_chromium.render_map(out, out.with_suffix('.png'))
```
  (add `sys.path.insert(0, str(Path(__file__).resolve().parent))` before it if `tools/` is not already on the path);
  docstring `:16`: `The PNG (2x) comes from the SVG through the system Chromium (tools/render_chromium.py).`
  `tools/gen_cheatsheets.py:5-6`: `then tools/render_chromium.py makes the PNGs and PDFs:\n#   python3 tools/gen_cheatsheets.py && python3 tools/render_chromium.py`.
  `git rm tools/render_cheatsheets.mjs`; `grep -rn "render_cheatsheets" --include=*.py --include=*.md --include=*.sh --include=*.yml .`
  (outside `docs/superpowers/` and `build/`) prints nothing.

- [ ] **Step 4: Run to see it pass.** `python3 tools/render_chromium.py --verify`; `python3 tools/gen_nav_map.py
  --check`; `python3 tools/gen_nav_map.py` (renders the PNG through the new path; `git diff --stat docs/navigation-map.png`
  may show a byte change only, the size the same); `python3 tools/gen_manual.py --check`.

- [ ] **Step 5: Commit** — `git add tools/render_chromium.py tools/gen_nav_map.py tools/gen_cheatsheets.py tests/run_tests.sh docs/cheatsheet-controls.png docs/cheatsheet-controls.pdf docs/cheatsheet-sound.png docs/cheatsheet-sound.pdf docs/navigation-map.png && git commit`
  (the `git rm` is staged) with `docs: cheat sheets and the map's PNG rendered with the system Chromium (render_chromium.py)`;
  body: the five files' sizes, what was looked at.

---

### Task 6: Dead code out, tools hardened, comments and docs tidied (items 11, 12, 13)

**Files:** Modify `firmware/src/ui.c:641, :659-669`, `firmware/src/ui_lens.c:25` (Step 1, if dead),
`firmware/src/gen.c` (`gs` `:112-114`, `gen_fit_tape` `:2311-2318`, `gx_fit_piece` `:2350`, `gen_make` `:2611`),
`firmware/src/storage.c:16-20`, `tools/gen_aa_keycaps.py`, `tools/gen_about_qr.py:9-11`, `tests/about_qr_test.py`,
`tools/build.py:296-303`, `tests/ux_msg_scan.py:85-92`, `.github/workflows/emulator.yml:134-135`,
`docs/braindancer/baseline.md:1`, `docs/braindancer/flash-map.md`, `firmware/src/editor.c`, `firmware/src/felucca.c:99-100`.
Test `tests/gen_test.c` (top, `:1708`, `:1838`). Create `tests/build_py_test.py`, `tests/nsong_tie.c`,
`tests/nsong_tie_test.sh`; `tests/run_tests.sh`.

**Interfaces — Produces:** `GEN_PROBE(field, value)` (gen.c, a no-op unless the including test defines it);
`build.small_int(v: str) -> int | None`; `ux_msg_scan.LBL_ONLY`, `ux_msg_scan.table_kind(name) -> str`,
`ux_msg_scan.selfcheck()`; `ST_NSONG`, `ST_NSONG_INDEX` (storage.c).

- [ ] **Step 1: `save_on` / `LZ_SAVE` — only if dead (pre-flight 14).** Render every UI scene at base:
  `FV=$(sed -n 's/^#define FELUCCA_VERSION "\(.*\)".*/\1/p' firmware/src/felucca.c); tools/host-cc -O1 -w "-DFELUCCA_VERSION=\"$FV\"" -Ibuild/gen -Ifirmware/src -Itests -o build/host/ui_render tests/ui_render.c -lm && rm -rf /tmp/ui091a && build/host/ui_render /tmp/ui091a /tmp/ui091a_slot`.
  Then in `ui.c`: delete `save_on` (`:641`), drop `save_on() || ` from `lz_on` (`:659`), make `lz_fam`'s last line
  `return fx_on() ? LZ_FX : LZ_SEQ;`; in `ui_lens.c:25` delete `LZ_SAVE, ` from the enum (renumbering: grep
  `LZ_` uses compare by name only: `grep -n "LZ_SAVE\|<= LZ_FX\|LZ_FX\b" firmware/src/*.c`; `lz_mock_cards` uses
  `lz_fam() <= LZ_FX`, which still holds with LZ_SEQ 0, LZ_FX 1). Render again into `/tmp/ui091b` (same command);
  `diff -r /tmp/ui091a/ppm /tmp/ui091b/ppm` must print nothing. If it prints anything: `git checkout firmware/src/ui.c
  firmware/src/ui_lens.c` and report "save_on is not dead: the SAVE pages read the lens's geometry (N renders differ)".

- [ ] **Step 2: Write the failing tests.**
  (a) `tests/gen_test.c`, before `#include "ui_test.c"`:
```c
static struct { uint32_t cost_thin; uint8_t grain_off; } gt_probe;   /* (0.9.1) what gen.c hands the tests (GEN_PROBE):
                                                         * not in the firmware's RAM */
#define GEN_PROBE(f, v) (gt_probe.f = (v))
```
  and replace `gs.grain_off` (`:1708`) with `gt_probe.grain_off` and `gs.cost_thin` (`:1838`) with `gt_probe.cost_thin`.
  (b) `tests/build_py_test.py`:
```python
#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1) tools/build.py: FELUCCA_NVOICE / FELUCCA_VOICES_2CORE take an ASCII integer 1..16 only (str.isdigit also
says yes to other scripts' digits and to superscripts, which int() then refuses or reads)."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import build  # noqa: E402

CASES = {"8": 8, "1": 1, "16": 16, "0": None, "17": None, "": None, "x": None, " 8": None, "\u0663": None,
         "\u00b2": None, "\uff11\uff12": None}
bad = [repr(v) for v, want in CASES.items() if build.small_int(v) != want]
print("build: small_int " + ("ok" if not bad else "FAIL on " + ", ".join(bad)))
sys.exit(bool(bad))
```
  (c) `tests/nsong_tie.c` and `tests/nsong_tie_test.sh`:
```c
/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 NorfolkHacker
 * (0.9.1) song_chain.c and storage.c agree on NSONG / NSONG_INDEX: with song_chain.c's NSONG another value (TIE_NSONG,
 * default 5) storage.c must not build. tests/nsong_tie_test.sh builds it both ways (syntax only) */
#ifndef TIE_NSONG
#define TIE_NSONG 5u
#endif
#define NSONG TIE_NSONG
#define NSONG_INDEX 8u
#include "hostsim.c"
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
#include "../firmware/src/menu_items.c"
#include "../firmware/src/storage.c"
```
```sh
#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
# (0.9.1) the NSONG tie: tests/nsong_tie.c builds with NSONG 4 and does not with 5 (storage.c's static assert)
CC=${CC:-tools/host-cc}
F="-fsyntax-only -w -Ibuild/gen -Ifirmware/src -Itests"
$CC $F -DTIE_NSONG=4u tests/nsong_tie.c || { echo "nsong_tie: FAIL: does not build with NSONG 4 (the test's includes)"; exit 1; }
if $CC $F tests/nsong_tie.c 2>/tmp/nsong_tie.err; then echo "nsong_tie: FAIL: builds with NSONG 5"; exit 1; fi
grep -q "NSONG" /tmp/nsong_tie.err || { echo "nsong_tie: FAIL: another error"; cat /tmp/nsong_tie.err; exit 1; }
echo "nsong_tie: ok"
```
  `tests/run_tests.sh`, after Task 4's line:
  `run "tools/build.py: FELUCCA_NVOICE as an ASCII integer 1..16 only" python3 tests/build_py_test.py`,
  `run "song_chain.c and storage.c agree on NSONG (a static assert)" sh tests/nsong_tie_test.sh`,
  `run "ux_msg_scan: _LBL tables by an explicit list" python3 -c "import sys; sys.path.insert(0, 'tests'); import ux_msg_scan; ux_msg_scan.selfcheck()"`.

- [ ] **Step 3: Run to see them fail.** `gen_test`: FAIL on "(0.9) every piece fitted with TAPE counted" (`cut_bad`:
  `gt_probe.cost_thin` 0) and the grain line; `python3 tests/build_py_test.py`: `AttributeError: ... small_int`;
  `sh tests/nsong_tie_test.sh`: `FAIL: builds with NSONG 5`; the selfcheck line: `AttributeError`;
  `/usr/bin/python3 tests/about_qr_test.py` (the system Python, no segno): a `ModuleNotFoundError` traceback.

- [ ] **Step 4: Implement.**
  `gen.c`: delete `uint16_t cost_thin; ...` and `uint8_t grain_off; ...` (with their comments) from `gs`; above `gs`
  add
```c
#ifndef GEN_PROBE                                       /* (0.9.1) what the fitter did, for the tests only (tests/gen_test.c */
#define GEN_PROBE(f, v) ((void)0)                       /* defines it): nothing in the firmware's RAM */
#endif
```
  `gen_make` after `memset(&gs, 0, sizeof gs);`: `GEN_PROBE(grain_off, 0); GEN_PROBE(cost_thin, 0);`.
  `gen_fit_tape` becomes:
```c
static void gen_fit_tape(void)
{
    uint32_t c = gc_cost(1);                            /* (after the first rung, before the music is thinned) */
    GEN_PROBE(cost_thin, c);
    gs.age_cut = 0;
    if (gp.id.tape != GT_AUTO || !gs.age || c <= gc_budget() || gen_keeps_age()) return;
    gs.age = 0;                                         /* (written once, after the fit: gen_make, review M4) */
    gs.age_cut = 1;
}
```
  `gx_fit_piece`: `gs.grain_off = (uint8_t)gr;` → `GEN_PROBE(grain_off, (uint8_t)gr);`.
  `tools/gen_aa_keycaps.py`: delete `KNOBS`, `knob_quadrant`, the knob loop in `main` (`knob_bytes` and its four
  lines), `import math` if nothing else uses it, the docstring's knob paragraph (`:22-25`) and "and the knob arc masks"
  (`:5`); the header's first line becomes `/* generated by tools/gen_aa_keycaps.py: keycaps / badges */`; the print
  drops `knob arcs {knob_bytes} B`. Check: `./build.sh` regenerates `build/gen/ui_keycaps.h`; `grep -c KNOB_
  build/gen/ui_keycaps.h` prints 0.
  `storage.c:16-20` becomes:
```c
#define ST_NSONG 4u                                    /* (ACID FM 0.2) songs in use; (0.9.1) song_chain.c's must be the */
#define ST_NSONG_INDEX 8u                              /* same (asserted): rows in the stored song index (JIANT's 8) */
#ifndef NSONG                                  /* (song_chain.c defines them, before this file in the firmware; the host tests that
                                                * include this file alone get them here, the same values) */
#define NSONG ST_NSONG
#define NSONG_INDEX ST_NSONG_INDEX
#endif
_Static_assert(NSONG == ST_NSONG && NSONG_INDEX == ST_NSONG_INDEX, "NSONG / NSONG_INDEX: song_chain.c and storage.c differ");
```
  `tools/gen_about_qr.py`: replace `import segno` with
```python
try:
    import segno
except ImportError:
    sys.exit("gen_about_qr: the Python package segno is missing (it is in the project's .venv: . .venv/bin/activate)")
```
  `tests/about_qr_test.py`: replace the `subprocess.run(..., check=True, ...)` call with
```python
p = subprocess.run([sys.executable, str(ROOT / "tools/gen_about_qr.py"), ABOUT_URL], capture_output=True, text=True)
if p.returncode and "segno is missing" in p.stderr:
    print("skip ABOUT QR: segno missing (activate .venv)")
    sys.exit(0)
if p.returncode:
    sys.exit("FAIL: gen_about_qr.py: " + p.stderr.strip())
out = p.stdout
```
  `tools/build.py`: above `def build_app():` add
```python
def small_int(v):
    """an ASCII integer 1..16 (str.isdigit alone takes other scripts' digits and superscripts), else None"""
    return int(v) if v.isascii() and v.isdigit() and 1 <= int(v) <= 16 else None
```
  and in `build_app` replace `if not (v.isdigit() and 1 <= int(v) <= 16):` with `if small_int(v) is None:` (the
  `flags.append(f"-D{flag}={int(v)}")` line stays).
  `tests/ux_msg_scan.py`: above `def main(out):` add
```python
LBL_ONLY = ("UXG_ARROW_LBL", "UXG_MIDI_LBL")   # (0.9.1) tables drawn in UF_LBL alone, by name (was any *_LBL)


def table_kind(name):
    """a UX*_ table's faces: 'val' (UF_VAL), 'hdr' (UF_HDR), 'lbl' (UF_LBL, LBL_ONLY), 'both' (UF_HDR and UF_LBL)"""
    if name.endswith("_VAL"):
        return "val"
    if name.endswith("_HDR"):
        return "hdr"
    if name in LBL_ONLY:
        return "lbl"
    if name.endswith("_LBL"):
        sys.exit(f"ux_msg_scan: {name} ends in _LBL but is not in LBL_ONLY: list it (UF_LBL alone) or rename it")
    return "both"


def selfcheck():
    assert table_kind("UXD_Q_VAL") == "val" and table_kind("UXD_LOST_HDR") == "hdr"
    assert table_kind("UXG_MIDI_LBL") == "lbl" and table_kind("UXS_LABEL") == "both"
    try:
        table_kind("UXQ_NEW_LBL")
    except SystemExit:
        print("ux_msg_scan: selfcheck ok")
        return
    raise AssertionError("an unlisted _LBL table passed")
```
  and in `main` rewrite the `if t.group(1).endswith('_VAL') ... elif ... elif t.group(1) != 'UXH_PAGES':` chain to
  dispatch on `k = table_kind(t.group(1))` (`'val'` → `val`, `'hdr'` → `hdr`, `'lbl'` → `lbl`, `'both'` unless
  `UXH_PAGES` → `hdr` and `lbl` and, for `UXD_`, `val`), each branch counting `tables += 1` as before; keep the
  comments.
  Tidy: `.github/workflows/emulator.yml:134-135` comment → `# (Braindancer) the site's front page (web/landing: what
  Braindancer is, install / try / read the manual, what comes next) at the`; `docs/braindancer/baseline.md`: first two
  lines `<!-- SPDX-License-Identifier: GPL-3.0-only -->` and `<!-- Copyright (C) 2026 NorfolkHacker -->`;
  `docs/braindancer/flash-map.md`: wrap the prose lines over 120 columns (pre-flight 15; table rows stay);
  `firmware/src/editor.c`, `firmware/src/felucca.c`: wrap every line over 120 columns (comments re-flowed with the file's
  ` * ` style; code broken after an operator, aligned as the file does).

- [ ] **Step 5: Run to see them pass.** `gen_test` (all, the TAPE sweep's figures unchanged: compare its printed
  "two cores: N of M over" line with the base's); `python3 tests/build_py_test.py`; `sh tests/nsong_tie_test.sh`;
  the selfcheck line; `/usr/bin/python3 tests/about_qr_test.py` → `skip ABOUT QR: segno missing (activate .venv)`;
  `python3 tests/about_qr_test.py` (venv) → `ok`; `python3 tests/ux_msg_scan.py build/gen/ux_msg.h` and `git diff
  --stat build/` is not tracked: instead compare its output file before / after (`cp build/gen/ux_msg.h /tmp/ux_a.h`
  first; `cmp` after: identical). Long lines: `awk 'length > 120' firmware/src/editor.c firmware/src/felucca.c` prints
  nothing; `awk 'length > 120 && !/^\|/' docs/braindancer/flash-map.md` prints nothing; the code unchanged:
```sh
for f in firmware/src/editor.c firmware/src/felucca.c; do
  python3 - "$f" <<'PY'
import re, subprocess, sys
f = sys.argv[1]
strip = lambda s: re.sub(r"\s+", "", re.sub(r"//[^\n]*", "", re.sub(r"/\*.*?\*/", "", s, flags=re.S)))
old = subprocess.run(["git", "show", "HEAD:" + f], capture_output=True, text=True, check=True).stdout
print(f, "code unchanged" if strip(old) == strip(open(f).read()) else "CODE CHANGED")
PY
done
```
  `./build.sh` (image: the freed `gs` bytes; RAM line); `python3 tests/target_budget.py`; the full suite.

- [ ] **Step 6: Commit** — `git add -A firmware/src/ui.c firmware/src/ui_lens.c firmware/src/gen.c firmware/src/storage.c firmware/src/editor.c firmware/src/felucca.c tools/gen_aa_keycaps.py tools/gen_about_qr.py tools/build.py tests/gen_test.c tests/about_qr_test.py tests/ux_msg_scan.py tests/build_py_test.py tests/nsong_tie.c tests/nsong_tie_test.sh tests/run_tests.sh .github/workflows/emulator.yml docs/braindancer/baseline.md docs/braindancer/flash-map.md && git commit`
  with `chore: dead code out (save_on, knob arcs, gs probes), NSONG tied, tools hardened, long lines and headers tidied`;
  body: Step 1's outcome (removed, or kept and why), the image line.

---

### Task 7: The goldens the renders lack (item 14)

**Files:** Modify `tests/golden.txt` (lines added only), `tests/regress.c:1270-1272, :1341-1347`.

**Interfaces:** none.

- [ ] **Step 1: Write the failing test.** `tests/regress.c`: a render with no golden line fails (pre-flight 16). The
  `else if (g_new)` branch's message becomes
  `printf("regress: %u renders not in %s (FAIL: every render has a golden; add only their lines, docs: Task 7 of 0.9.1)\n", g_new, gpath);`
  and the return becomes
  `return (!gupd && (g_changed || g_gone || g_new)) || h_fail || k_fail || (!cupd && c_fail) || crash;`.

- [ ] **Step 2: Run to see it fail.** `tools/host-cc -O2 -w -Ibuild/gen -Ifirmware/src -o build/host/regress tests/regress.c -lm && build/host/regress tests/golden.txt tests/cpu_baseline.txt | grep -a -E "renders not in|GOLDEN|golden renders"; echo "exit $?"`
  — expected `45 renders not in tests/golden.txt (FAIL ...)` (report the number seen) and a non-zero exit status
  (check with `build/host/regress tests/golden.txt tests/cpu_baseline.txt > /tmp/regress.log; echo $?`).

- [ ] **Step 3: Add only the missing lines.**
```sh
cp tests/golden.txt /tmp/golden-091.txt
GOLDEN_UPDATE=1 build/host/regress /tmp/golden-091.txt tests/cpu_baseline.txt | grep -a "golden file"
python3 - <<'PY'
old_lines = open("tests/golden.txt").read().splitlines(True)
head = [l for l in old_lines if l.startswith("#")]
old = {l.split()[0]: l for l in old_lines if l.strip() and not l.startswith("#")}
new = [l for l in open("/tmp/golden-091.txt") if l.strip() and not l.startswith("#")]
names = [l.split()[0] for l in new]
changed = [n for n, l in zip(names, new) if n in old and old[n] != l]
gone = [n for n in old if n not in names]
assert not changed and not gone, ("changed", changed, "gone", gone)
added = [l for n, l in zip(names, new) if n not in old]
open("tests/golden.txt", "w").write("".join(head + [old.get(n, l) for n, l in zip(names, new)]))
print(len(added), "lines added:", " ".join(l.split()[0] for l in added))
PY
git diff --numstat tests/golden.txt
```
  The assertion must hold (no existing line changed or gone: if it fails, stop and report; the sound moved). `git diff
  --numstat` must read `<N> 0 tests/golden.txt` with N the number Step 2 printed (additions only; existing lines keep
  their order).

- [ ] **Step 4: Run to see it pass.** `build/host/regress tests/golden.txt tests/cpu_baseline.txt > /tmp/regress.log; echo $?`
  → 0, and `grep -a "renders not in" /tmp/regress.log` prints nothing; the full suite.

- [ ] **Step 5: Commit** — `git add tests/golden.txt tests/regress.c && git commit` with
  `test(regress): goldens for the N renders that had none; a render without one fails`; body: the added names.

---

### Task 8: The device calibration session (the controller, with the user)

Not an implementer task. Tasks 1–7 merged on `cleanup-091`, the suite green, the tree clean. Every serial and device
command below is the controller's; the user presses the buttons and listens.

**Files (data):** `build/cal/cal-091.log` (raw, not committed), `build/cal/notes-091.txt` (the user's remarks, not
committed), `docs/braindancer/gen-calibration-0.9.1.csv` (committed), `docs/braindancer/device-log.md` (committed).

- [ ] **Step 1: Backup and build.** In `~/MvaveFM1Unbricker`: `softkey --port "BRAINDANCER" --yes`, then the user runs
  `sudo .venv/bin/python fm1_unbrick.py backup ~/fm1-backups/2026-10-31-braindancer-0.9` (or the session's date);
  record the backup's folder and sha256 in the notes. In the repo: `git status` clean; `./build.sh --release 0.9.1-cal`
  (`build/braindancer-0.9.1-cal.fwsc`); record its sha256 and the HEAD commit (the **calibration commit**).
- [ ] **Step 2: Install and check.** `tools/fm1_install.py build/braindancer-0.9.1-cal.fwsc --port "BRAINDANCER" --yes`;
  `tools/cal_snap.sh idle build/cal/cal-091.log` → the line shows `gen_piece -` and the status' first line (read with
  the old `build/cal/snap.sh` once, or `stty`/`cat` by hand) says `braindancer v0.9.1-cal`.
- [ ] **Step 3: The conditions (ask the user to set and confirm each):** MENU › MORE › CPU CORES = **2 CORES**; TRACKS
  **6**; a CPU TEST stored (if none since the install: SAVE › TOOLS › CPU TEST played ~20 s, then stopped; the
  `gen_id`'s third number is its ratio, 0 = none); NEW PROJECT once (SAVE › TOOLS: the session starts from the power-on
  parts, as the host harness does); GEN's four knobs at their defaults (ENERGY 50, FORM PARTS, COMPLEX 50, TAPE AUTO).
  Record them in `build/cal/notes-091.txt`.
- [ ] **Step 4: About 20 GENERATE presses.** For n = 01..20: the user opens GEN (HOME + SELECT), presses OCT+ GENERATE
  (and PLAY if the transport is stopped), says "go" after ~3 s; the controller runs
  `tools/cal_piece.sh G<n> build/cal/cal-091.log` (~25 s) and reads its printed row; the user says whether clicks were
  heard (y / n) and anything else, noted against G<n> in `build/cal/notes-091.txt`. Do not change knobs, tracks or FX
  between presses. Every 5 presses check `audio_lost` (the row's `lost`): if any row has `lost > 0`, stop the session
  and report to the user (audio lost is a fault, not a data point).
- [ ] **Step 5: CSV.** `python3 tools/gen_cal.py csv build/cal/cal-091.log docs/braindancer/gen-calibration-0.9.1.csv`;
  check every G row has a piece and no note (a row with `piece changed in the window` stays in the CSV and is left
  out of the fit by `gen_cal.py ids`).
- [ ] **Step 6 (optional, only if the user agrees, ~5 min): the TAPE pair.** On a piece whose `gen_age` is above 0
  (a RETROBOC or AMBIENT one): press GENERATE until one comes, run `tools/cal_piece.sh G<n>` as above, then the user
  turns FX › MASTER AGE to 0 and the controller runs `tools/cal_piece.sh G<n>-AGE0`; once more with AGE back at its
  value: `G<n>-AGE<value>`. Regenerate the CSV (Step 5). Task 9 uses the cpu difference (pre-flight 6).
- [ ] **Step 7: Log and commit.** `docs/braindancer/device-log.md`: a section `## 2026-10-31 — Braindancer 0.9.1
  calibration session (0.9.1-cal, <calibration commit>)` with the conditions, the backup, the package's sha256, a table
  `| Tag | Piece | AGE | cpu % | late /20 s | shed /20 s | lost | gen_cost / budget | clicks |` from the CSV and the
  notes, and how many rows were late (`late20 > 5`). `git add docs/braindancer/gen-calibration-0.9.1.csv
  docs/braindancer/device-log.md && git commit` with `data: GEN calibration on 0.9.1-cal (<N> pieces, 2 CORES, CPU
  TEST <x> %)`. Leave the device on 0.9.1-cal for now.

---

### Task 9: The model re-fitted on the device rows (item 1)

**Files:** Modify `firmware/src/gen.c` (`gc_cost` `:2156-2199` split; the constants `:2055-2058`, `:2065`,
`:2116-2119`, maybe `:2086-2087`; their comments `:2037-2119`), `tests/gen_test.c` (`GT_X1` `:23` and its comment;
re-recorded figures; a new block). Create `tests/gen_terms.c`, `tools/gen_fit.py`, `tests/gen_fit_test.py`,
`tests/gen_cal_rows.h` (generated), `docs/braindancer/gen-cal-rows.csv`, `docs/braindancer/gen-calibration-0.9.1.md`;
`tests/run_tests.sh` (after Task 6's lines).

**Interfaces — Consumes:** `docs/braindancer/gen-calibration-0.9.1.csv` (Task 8), `tools/gen_cal.py ids / rows`
(Task 3), `gen_build`, `section_load`, `gs.wrote`, `gs.in_sec`, `gc_dev_of`, `gc_budget`, `gcf`. **Produces:**
```c
typedef struct {
    uint16_t fixed;                                     /* the base, the reverb, the grain, the delay's line, PLAYHEADS */
    uint8_t tape, ccomp, n;                             /* TAPE counted (AGE above 0); the chord steps with ECHO / ARP NOTES; parts */
    uint8_t eng[NTRK];                                  /* each part's engine (GC_ENG / GC_VOX) */
    uint16_t v[NTRK];                                   /* .. and its voices x100 */
} gc_terms_t;
static void gc_terms(int steps, gc_terms_t *m);
static uint32_t gc_sum(const gc_terms_t *m);
static uint32_t gc_cost(int steps);                     /* = gc_sum of gc_terms, unchanged in value */
```
`tests/gen_cal_rows.h`: `gcal_row_t {const char *tag; uint8_t wild, two, late; uint16_t base; gc_terms_t t;}`,
`GCAL[]`, `GCAL_N`. `tools/gen_fit.py`: `load`, `cost`, `budget`, `judge`, `fit`, `THETA0`; CLI `ROWS.csv [--emit-h
OUT.h | --check-h OUT.h]`.

- [ ] **Step 1: The rows file, 0.8's part** (pre-flight 5; transcribed from the calibration doc's per-track
  breakdown: `fixed` = 295 + the reverb's GC_REV + the delay's GC_DLO; a part `engine:voices x100`, ANALOG 0, PHASE 2,
  LOFI 3, VOICE 5, DRUM 10, NOISE 11, FM6 12; `ccomp` the chord steps with ECHO / ARP NOTES; CPU TEST 74 % → base 117).
  Create `docs/braindancer/gen-cal-rows.csv`:
```
session,tag,name,wild,two,base,fixed,tape,ccomp,parts,late20,timed
0.8,0.8 4741,AMBIENT+AMBIENT 4741,0,1,117,405,0,1,12:100;12:400;0:28;2:99;11:136,0,0
0.8,0.8 7848,EXPLORE 7848,1,1,117,320,0,0,3:72;0:332;10:187;3:100,1,0
0.8,0.8 4493,GLITCH+BREAKS 4493,0,1,117,342,0,0,0:98;12:400;0:32;10:200;0:118;3:45,10,0
0.8,0.8 0036,AMBIENT 0036,0,1,117,405,0,0,0:100;0:306;0:46;10:178;12:188;0:117,25,1
0.8,0.8 0965,ACID+IDM 0965,0,1,117,342,0,1,2:64;12:260;0:16;10:200;11:122,7,1
0.8,0.8 8794,BREAKS+ACID 8794,0,1,117,295,0,2,12:100;2:84;0:28;10:200;5:0;3:208,6,1
0.8,0.8 7625,RETROBOC 7625,0,1,117,295,0,0,0:22;12:244;12:227;10:200;0:92;5:96,2,1
0.8,0.8 2020,AMB TECHNO 2020,0,1,117,342,0,0,0:63;12:252;0:28;10:150;12:162;11:141,1,1
0.8,0.8 7033,IDM+BREAKS 7033,0,1,117,342,0,0,2:44;12:400;2:48;10:200;0:75;11:67,2,1
0.8,0.8 1751,AMBTECH+IDM 1751,0,1,117,342,0,2,2:100;2:72;0:45;10:200;0:75;0:109,81,1
0.8,0.8 1174,BREAKS+IDM 1174,0,1,117,320,0,2,12:100;0:96;0:41;10:200;3:83;3:25,44,1
0.8,0.8 8790,BREAKS+ELECTRO 8790,0,1,117,320,0,2,0:37;0:232;0:32;10:178;0:85;11:64,9,1
0.8,0.8 7563,AMBIENT+D'N'BASS 7563,0,1,117,327,0,0,2:52;0:164;0:46;10:121;2:87;11:123,1,1
0.8,0.8 4459,ELECTRO+D'N'BASS 4459,0,1,117,327,0,0,0:24;2:412;3:28;10:200;0:100;11:151,4,1
0.8,0.8 1573,BREAKS+D'N'BASS 1573,0,1,117,327,0,1,0:21;0:88;3:67;10:200;0:97;11:103,4,1
```
  (Checks the test makes in Step 3: under 0.8's FM6 79 / 26 and no component term, 0036 836, 2020 845, 1174 668, 8794
  707, 1751 672, 7848 583 — the doc's 0.8 costs.)

- [ ] **Step 2: Split `gc_cost` (value unchanged) and the harness.** `gen.c`: replace `gc_cost` with `gc_terms_t` (as
  in Interfaces), and:
```c
static void gc_terms(int steps, gc_terms_t *m)          /* (0.9.1) gc_cost's terms: what the constants weigh (the fit,
                                                         * tests/gen_terms.c, tests/gen_test.c) */
{
    const ggenre_t *g = gs.g;
    uint32_t k, i, h, bpm = steps ? (uint32_t)song.g[G_BPM] : (g->bpm_lo + g->bpm_hi) / 2u;
    uint32_t c = (gcf.two ? 295u : 380u) + GC_REV[(uint32_t)(steps ? song.g[G_RTYPE] : g->rtype) % 5u];
    if (steps && (song.g[G_DPIT] || song.g[G_DSPRY])) c += GC_GRAIN;
    if (steps) c += GC_DLO[gcf.two ? 1 : 0][gc_dly_stage()];   /* (0.5.2) the delay's line past the full rate */
    m->tape = gs.age != 0;                              /* (0.9) TAPE while the piece's AGE is above 0 */
    m->ccomp = (uint8_t)(steps ? gc_ccomp_n() : 0u);    /* (0.9) the chord steps with ECHO / ARP NOTES */
    m->n = 0;
    for (k = 0; k < NTRK && k < trk_live; k++) {
        const track_t *t = &trk[k];
        uint32_t r = gs.role[k], e = t->eng_req % NENGINES, len = dlen(t), on = 0, ns = 0, tie = 0, v, f, cap;
        if (!gs.act[r] || (r == RL_TEX && k != gs.trk_of[RL_TEX])) continue;
        if (steps && gs.in_sec && t->p[P_MUTE]) continue;   /* (0.9) a later section's muted track (GEN's: B INTRO, the
                                                         * BREAK ..): no notes, its voices ended, it costs nothing
                                                         * (voice.c trk_note_on); only there (re-review N1) */
        for (h = 0; h < FG_HEADS; h++) c += fg_set[k][h].rate ? GC_HEAD : 0u;
        for (i = 0; steps && i < len; i++) {
            const step_t *s = &t->step[i];
            if (s->time == ST_NOTE && (s->n || s->hit)) { on++; ns += s->n; }
            tie += s->time == ST_TIE;
        }
        if (r == RL_DRUM) v = steps ? (on * 300u / (len ? len : 1u) < 200u ? on * 300u / (len ? len : 1u) : 200u) : 200u;
        else if (!steps) v = r == RL_PAD ? 300u : 100u;
        else if (!on) v = 0;
        else {
            uint32_t g100 = t->p[P_SGATE] < 100 ? (uint32_t)t->p[P_SGATE] : 100u;
            uint32_t tail = 5u * TIME_MS_X10[e == ENGI_FM6 ? 112u : t->p[P_REL] & 127] / 600u * bpm *   /* (0.9.14: FM6's own
                                                         * release, its patch's R4: ~2..4 s) */
                            GC_DIVQ[(uint32_t)t->p[P_SDIV] % 6u] / 100u;
            f = (on * (g100 + tail) + tie * 100u) / len;
            cap = t->p[P_VOICE] == V_MONO || t->p[P_VOICE] == V_LEGATO ? 100u : e == ENGI_FM6 || e == EG_VO ? 400u : 800u;
            v = cap == 100u ? f : ns * f / on;
            if (v > cap) v = cap;
        }
        m->eng[m->n] = (uint8_t)e;
        m->v[m->n] = (uint16_t)v;
        m->n++;
    }
    m->fixed = (uint16_t)c;
}
static uint32_t gc_sum(const gc_terms_t *m)             /* (0.9.1) the cost of terms under the constants (gcf's cores) */
{
    uint32_t c = m->fixed + m->ccomp * GC_CCOMP + (m->tape ? (gcf.two ? GC_TAPE2 : GC_TAPE1) : 0u), k;
    for (k = 0; k < m->n; k++)
        c += GC_ENG[m->eng[k]] + GC_VOX[m->eng[k]] * m->v[k] / 100u;
    return c;
}
static uint32_t gc_cost(int steps)                      /* steps 0: before the parts (a chord track's triads) */
{
    gc_terms_t m;
    gc_terms(steps, &m);
    return gc_sum(&m);
}
```
  Run `gen_test` now: every check and every printed figure as before (`GEN_HOUSE_HASH`, the sweep's counts: compare
  the printed lines with a run at the previous commit, `git stash`-free: build the previous commit's `gen_test` in a
  temporary worktree `git worktree add /tmp/w091 HEAD~0` before the edit, run it, `git worktree remove /tmp/w091`). A
  difference is a bug in the split.
  `tests/gen_terms.c` (new):
```c
/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 NorfolkHacker
 * (0.9.1) GEN's calibration (docs/braindancer/gen-calibration-0.9.1.md): each device row's piece rebuilt from the
 * identity the console printed (gen_id), in the session's order from a NEW PROJECT (as the device made them: a piece
 * reads which track is the DRUM track), and the CPU model's terms of its heaviest section (gen.c gc_terms), for
 * tools/gen_cal.py rows. MATCH 1: the rebuilt name is the device's and one section's cost is a cost the device printed.
 *   tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/gen_terms tests/gen_terms.c -lm
 *   python3 tools/gen_cal.py ids docs/braindancer/gen-calibration-0.9.1.csv | build/host/gen_terms > build/cal/terms-091.txt
 * in:  TAG|seed two dev form tape main part energy complex|cost_s|cost_e|NAME
 * out: TAG|MATCH|FIXED|TAPE|CCOMP|e:v;e:v..|COST|NAME */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

int main(void)
{
    char line[256], tag[64], nm[64], cs[16], ce[16];
    unsigned f[9];
    ui_power_on();
    ui_rec_prefs |= PREF_SIX >> 8; rp_apply();          /* TRACKS 6 (Task 8 Step 3) */
    settings.palette = UI_ACID_INDEX; palette_set(UI_ACID_INDEX);
    system_init();                                      /* (NEW PROJECT: the power-on parts, Task 8 Step 3) */
    while (fgets(line, sizeof line, stdin)) {
        genid_t id;
        gc_terms_t m, best;
        uint32_t s, cost, bc = 0, hit = 0, k;
        if (sscanf(line, "%63[^|]|%u %u %u %u %u %u %u %u %u|%15[^|]|%15[^|]|%63[^\n]", tag, &f[0], &f[1], &f[2], &f[3],
                   &f[4], &f[5], &f[6], &f[7], &f[8], cs, ce, nm) != 13)
            continue;
        memset(&id, 0, sizeof id);
        id.seed = f[0]; id.two = f[1]; id.dev = f[2]; id.form = f[3]; id.tape = f[4];
        id.main = (uint8_t)f[5]; id.part = (uint8_t)f[6]; id.energy = (uint8_t)f[7]; id.complex = (uint8_t)f[8];
        mc_host_v12 = id.two;                           /* (two cores: the host's 12 voices, as tests/gen_test.c) */
        gen_build(&id);
        memset(&best, 0, sizeof best);
        for (s = 0; s < 4u; s++) {
            if (s && (!((gs.wrote >> s) & 1u) || section_load(s))) continue;
            gs.in_sec = 1;                              /* (the device's figure: the music now, its muted tracks free) */
            gc_terms(1, &m);
            gs.in_sec = 0;
            cost = gc_sum(&m);
            hit |= cost == (uint32_t)atoi(cs) || cost == (uint32_t)atoi(ce);
            if (cost >= bc) { bc = cost; best = m; }
        }
        if (gs.wrote & 1u) section_load(0);             /* (A again: the music the next press starts from) */
        printf("%s|%u|%u|%u|%u|", tag, (unsigned)(hit && str_eq(gp.last, nm)), best.fixed, best.tape, best.ccomp);
        for (k = 0; k < best.n; k++) printf("%s%u:%u", k ? ";" : "", best.eng[k], best.v[k]);
        printf("|%u|%s\n", bc, gp.last);
    }
    return 0;
}
```
  Before the run: `git diff <calibration commit> -- firmware/src/gen.c firmware/src/gen_tables.c firmware/src/song_dice.c`
  shows only the split (no generator or constant change): the harness then builds what the device built.
  Run: `tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/gen_terms tests/gen_terms.c -lm && python3 tools/gen_cal.py ids docs/braindancer/gen-calibration-0.9.1.csv | build/host/gen_terms > build/cal/terms-091.txt`;
  `python3 tools/gen_cal.py rows docs/braindancer/gen-calibration-0.9.1.csv build/cal/terms-091.txt docs/braindancer/gen-cal-rows.csv`
  (appends the 0.9.1 rows; the rows not rebuilt exactly are printed and left out). If more than 3 rows are left out,
  stop and report (the harness does not reproduce the device's state: the controller decides).

- [ ] **Step 3: Write the failing tests.**
  `tools/gen_fit.py` (new):
```python
#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1) GEN's CPU model re-fitted on the device's rows (docs/braindancer/gen-cal-rows.csv: 0.8's 15 pieces and the
0.9.1 session's; docs/braindancer/gen-calibration-0.9.1.md). A row is late when the FM-1 played it with more than 5 late
halves per 20 s (spec 0.9.1 item 1). The fit: over a grid of five constants (a style's factor GC_GEN_X100, WILD's
GC_WILD_X100, FM6's GC_ENG / GC_VOX, GC_CCOMP), the fewest rows on the wrong side of the budget (a late row within it
counts 2, a clean row over it 1), then the widest margin, then the nearest to 0.9's constants.

  tools/gen_fit.py ROWS.csv                  the fit, and each row's cost / budget under 0.9's constants and the fit's
  tools/gen_fit.py ROWS.csv --emit-h OUT.h   and the rows as C for tests/gen_test.c (tests/gen_cal_rows.h)
  tools/gen_fit.py ROWS.csv --check-h OUT.h  exit 1 unless OUT.h is what --emit-h writes"""
import csv
import sys

ENG = [27, 40, 45, 27, 27, 64, 27, 27, 27, 27, 70, 60, 40, 27, 27]   # gen.c GC_ENG at 0.9 (FM6 12, DIGITAL 1: fitted)
VOX = [19, 13, 15, 17, 19, 23, 19, 19, 19, 19, 11, 16, 13, 19, 19]   # gen.c GC_VOX at 0.9
FM6 = (1, 12)
TAPE = (76, 92)                                                       # GC_TAPE1 / GC_TAPE2 (not fitted)
THETA0 = {"sty": 118, "wild": 155, "e6": 40, "v6": 13, "k": 70}
DEV2_0 = (150, 196)                                                   # GC_DEV2_X100 / GC_DEV2W_X100 at THETA0
GRID = {"sty": range(100, 182, 2), "wild": range(120, 225, 5), "e6": range(20, 85, 5), "v6": range(6, 28, 2),
        "k": range(0, 150, 10)}
LATE = 5.0
NTRK = 6


def load(path):
    rows = []
    for r in csv.DictReader(open(path)):
        parts = [tuple(int(x) for x in p.split(":")) for p in r["parts"].split(";") if p]
        rows.append({"tag": r["tag"], "wild": int(r["wild"]), "two": int(r["two"]), "base": int(r["base"]),
                     "fixed": int(r["fixed"]), "tape": int(r["tape"]), "ccomp": int(r["ccomp"]), "parts": parts,
                     "late20": float(r["late20"]), "late": float(r["late20"]) > LATE})
    return rows


def dev2(th):
    return round(DEV2_0[0] * th["sty"] / THETA0["sty"]), round(DEV2_0[1] * th["wild"] / THETA0["wild"])


def dev(r, th):                                                       # gen.c gc_dev_of
    if not r["two"]:
        d = r["base"] or 160
    elif r["base"]:
        d = r["base"] * (th["wild"] if r["wild"] else th["sty"]) // 100
    else:
        d = dev2(th)[1 if r["wild"] else 0]
    return min(max(d, 100), 400)


def budget(r, th):                                                    # gen.c gc_budget
    d = dev(r, th)
    return 1030 * 100 // d if r["two"] else 714 * 100 // d - 120


def cost(r, th):                                                      # gen.c gc_sum
    c = r["fixed"] + r["ccomp"] * th["k"] + (TAPE[1 if r["two"] else 0] if r["tape"] else 0)
    for e, v in r["parts"]:
        en, vx = (th["e6"], th["v6"]) if e in FM6 else (ENG[e], VOX[e])
        c += en + vx * v // 100
    return c


def judge(rows, th):
    """(score, margin, late rows within, clean rows over)"""
    ml = mc = 0
    m = None
    for r in rows:
        c, b = cost(r, th), budget(r, th)
        x = c / b - 1.0 if r["late"] else 1.0 - c / b
        m = x if m is None else min(m, x)
        ml += r["late"] and c <= b
        mc += (not r["late"]) and c > b
    return 2 * ml + mc, m, ml, mc


def side(rows, costs, budgets):
    """(score, margin or None) of rows with these costs and budgets"""
    s, m = 0, None
    for r, c, b in zip(rows, costs, budgets):
        x = c / b - 1.0 if r["late"] else 1.0 - c / b
        m = x if m is None else min(m, x)
        s += (2 if c <= b else 0) if r["late"] else (1 if c > b else 0)
    return s, m


def fit(rows):
    """(theta, judge(rows, theta)) of the best grid point. A cost depends on e6 v6 k alone, a style row's budget on sty
    alone, a WILD row's on wild alone: the budgets once, the costs once a (e6, v6, k)"""
    sr, wr = [r for r in rows if not r["wild"]], [r for r in rows if r["wild"]]
    bs = {s: [budget(r, dict(THETA0, sty=s)) for r in sr] for s in GRID["sty"]}
    bw = {w: [budget(r, dict(THETA0, wild=w)) for r in wr] for w in GRID["wild"]}
    best = None
    for e6 in GRID["e6"]:
        for v6 in GRID["v6"]:
            for k in GRID["k"]:
                th = dict(THETA0, e6=e6, v6=v6, k=k)
                cs, cw = [cost(r, th) for r in sr], [cost(r, th) for r in wr]
                d0 = abs(e6 / THETA0["e6"] - 1) + abs(v6 / THETA0["v6"] - 1) + abs(k / THETA0["k"] - 1)
                S = [(s, *side(sr, cs, bs[s])) for s in GRID["sty"]]
                W = [(w, *side(wr, cw, bw[w])) for w in GRID["wild"]]
                for s, ss, ms in S:
                    for w, sw, mw in W:
                        m = min([x for x in (ms, mw) if x is not None] or [0.0])
                        key = (ss + sw, -m, d0 + abs(s / THETA0["sty"] - 1) + abs(w / THETA0["wild"] - 1))
                        if best is None or key < best[0]:
                            best = (key, dict(th, sty=s, wild=w))
    return best[1], judge(rows, best[1])


def emit_h(rows):
    out = ["/* generated by tools/gen_fit.py from docs/braindancer/gen-cal-rows.csv: do not edit */",
           "typedef struct { const char *tag; uint8_t wild, two, late; uint16_t base; gc_terms_t t; } gcal_row_t;",
           "static const gcal_row_t GCAL[] = {"]
    for r in rows:
        eng = [e for e, _ in r["parts"]] + [0] * (NTRK - len(r["parts"]))
        v = [x for _, x in r["parts"]] + [0] * (NTRK - len(r["parts"]))
        out.append(f'    {{"{r["tag"]}", {r["wild"]}, {r["two"]}, {int(r["late"])}, {r["base"]}, {{{r["fixed"]}, '
                   f'{r["tape"]}, {r["ccomp"]}, {len(r["parts"])}, {{{", ".join(map(str, eng))}}}, '
                   f'{{{", ".join(map(str, v))}}}}}}},')
    out += ["};", "#define GCAL_N ((uint32_t)(sizeof GCAL / sizeof GCAL[0]))", ""]
    return "\n".join(out)


def main(argv):
    if not argv:
        sys.exit(__doc__)
    rows = load(argv[0])
    if "--check-h" in argv:
        h = argv[argv.index("--check-h") + 1]
        sys.exit(0 if open(h).read() == emit_h(rows) else f"gen_fit: {h} is not the rows' (run --emit-h)")
    th, (score, margin, ml, mc) = fit(rows)
    j0 = judge(rows, THETA0)
    print(f"gen_fit: {len(rows)} rows, {sum(r['late'] for r in rows)} late")
    print(f"  0.9's  {THETA0}: score {j0[0]}, late within {j0[2]}, clean over {j0[3]}, margin {j0[1]:+.3f}")
    print(f"  fitted {th}: score {score}, late within {ml}, clean over {mc}, margin {margin:+.3f}; "
          f"GC_DEV2_X100 {dev2(th)[0]}, GC_DEV2W_X100 {dev2(th)[1]}")
    for r in rows:
        print(f"  {r['tag']:<14} {'LATE ' if r['late'] else 'clean'} late20 {r['late20']:6.1f}  0.9 {cost(r, THETA0)}/"
              f"{budget(r, THETA0)}  fit {cost(r, th)}/{budget(r, th)}")
    if "--emit-h" in argv:
        open(argv[argv.index("--emit-h") + 1], "w").write(emit_h(rows))


if __name__ == "__main__":
    main(sys.argv[1:])
```
  `tests/gen_fit_test.py` (new):
```python
#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""(0.9.1) tools/gen_fit.py: the model as gen.c computes it (0.8's and 0.9's figures in the calibration doc), a fit that
puts every row of a known model on its side, and tests/gen_cal_rows.h the rows' own."""
import random
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import gen_fit as F  # noqa: E402

bad = 0


def check(what, ok):
    global bad
    print(f"gen_fit: {what:<74} {'ok' if ok else 'FAIL'}")
    bad += not ok


rows = F.load(ROOT / "docs/braindancer/gen-cal-rows.csv")
by = {r["tag"]: r for r in rows}
th08 = dict(F.THETA0, e6=79, v6=26, k=0)
check("0.8's costs from the doc's breakdown (FM6 79 / 26, no component term)",
      [F.cost(by["0.8 " + t], th08) for t in ("0036", "2020", "1174", "8794", "1751", "7848")] ==
      [836, 845, 668, 707, 672, 583])
check("0.9's model: 0036 773 of 746, 1751 812, WILD 7848 583 of 569",
      F.cost(by["0.8 0036"], F.THETA0) == 773 and F.budget(by["0.8 0036"], F.THETA0) == 746 and
      F.cost(by["0.8 1751"], F.THETA0) == 812 and F.cost(by["0.8 7848"], F.THETA0) == 583 and
      F.budget(by["0.8 7848"], F.THETA0) == 569)
check("the late rows: 0.8's seven (late20 > 5)", sorted(r["tag"] for r in rows if r["late"] and r["tag"].startswith("0.8")) ==
      sorted("0.8 " + t for t in ("1751", "1174", "0036", "8794", "0965", "8790", "4493")))
rnd = random.Random(91)
true = {"sty": 100, "wild": 170, "e6": 50, "v6": 16, "k": 40}   # (21 of the 40 rows late)
syn = []
for i in range(40):
    parts = [(rnd.choice((0, 2, 3, 5, 10, 11, 12)), rnd.randrange(0, 401)) for _ in range(rnd.randrange(4, 7))]
    r = {"tag": f"s{i}", "wild": int(i % 10 == 0), "two": 1, "base": 117, "fixed": rnd.choice((295, 320, 342, 405)),
         "tape": rnd.randrange(2), "ccomp": rnd.randrange(4), "parts": parts, "late20": 0.0}
    r["late"] = F.cost(r, true) > F.budget(r, true)
    syn.append(r)
th, (score, margin, ml, mc) = F.fit(syn)
check(f"a known model's rows ({sum(r['late'] for r in syn)} late of 40): every row on its side", score == 0)
p = subprocess.run([sys.executable, str(ROOT / "tools/gen_fit.py"), str(ROOT / "docs/braindancer/gen-cal-rows.csv"),
                    "--check-h", str(ROOT / "tests/gen_cal_rows.h")])
check("tests/gen_cal_rows.h is the rows'", p.returncode == 0)
print("gen_fit: all ok" if not bad else f"gen_fit: {bad} FAILED")
sys.exit(bad != 0)
```
  `python3 tools/gen_fit.py docs/braindancer/gen-cal-rows.csv --emit-h tests/gen_cal_rows.h` (the header from the
  rows; the fit's output is kept for the doc). `tests/gen_test.c`: `#include "gen_cal_rows.h"` after `#include
  "ui_test.c"`; a new block after Task 3's:
```c
    {   /* (0.9.1 spec 1) the model against the FM-1: every row measured (0.8's 15 pieces, the 0.9.1 session's;
         * docs/braindancer/gen-cal-rows.csv, tests/gen_cal_rows.h): a row played late (more than 5 late halves per 20 s)
         * over the budget (fitted), a clean one within. At most 1 row in 10 late but within (spec: at most 1 piece in
         * 10 late), at most a quarter of the clean rows over (thinned for nothing) */
        uint32_t i, ml = 0, mc = 0, nl = 0, nc = 0;
        uint16_t d0 = gcf.dev;
        uint8_t t0 = gcf.two;
        for (i = 0; i < GCAL_N; i++) {
            const gcal_row_t *r = &GCAL[i];
            uint32_t c, b;
            gcf.two = r->two;
            gcf.dev = (uint16_t)gc_dev_of(r->base, r->two, r->wild);
            c = gc_sum(&r->t);
            b = gc_budget();
            if (r->late) { nl++; ml += c <= b; } else { nc++; mc += c > b; }
            if (r->late ? c <= b : c > b)
                printf("gen:   %s %s: cost %u, budget %u\n", r->tag, r->late ? "late but within" : "clean but over", c, b);
        }
        gcf.dev = d0; gcf.two = t0;
        printf("gen: (0.9.1) the device's rows: %u late (%u within the budget), %u clean (%u over)\n", nl, ml, nc, mc);
        bad += gcheck("(0.9.1) the model against the FM-1's rows: at most 1 in 10 late but within, at most a quarter of "
                      "the clean over", nl && nc && ml * 10u <= GCAL_N && mc * 4u <= nc);
    }
```
  `tests/run_tests.sh`, after Task 6's lines: `run "GEN's CPU model fit (tools/gen_fit.py) and its rows" python3 tests/gen_fit_test.py`.

- [ ] **Step 4: Run to see them fail.** `python3 tests/gen_fit_test.py`: all ok (the tool and its rows are new: this
  pins them); `gen_test`: the new check FAILs with the constants of 0.9 (the session's late pieces were fitted within
  0.9's budget, so they are "late but within"). Keep its printed lines. If it passes, stop and report: the model
  already meets the target, the re-fit is the controller's call.

- [ ] **Step 5: Fit and set the constants.** `python3 tools/gen_fit.py docs/braindancer/gen-cal-rows.csv | tee
  build/cal/fit-091.txt`. If the fitted point's late-within × 10 > rows or clean-over × 4 > clean rows, stop and report
  the best point (pre-flight 7). Otherwise in `gen.c`: `GC_GEN_X100` = sty, `GC_WILD_X100` = wild, `GC_ENG[1]` =
  `GC_ENG[12]` = e6, `GC_VOX[1]` = `GC_VOX[12]` = v6, `GC_CCOMP` = k, `GC_DEV2_X100` / `GC_DEV2W_X100` = the printed
  values. TAPE: only if Task 8 Step 6's rows exist: Δcpu = the mean of `cpu_e` over the AGE-on windows minus that over
  the AGE 0 windows of the same piece; `GC_TAPE2 = round(Δcpu × 1030 × 100 / (65 × d))`, `d = base × sty / 100` at the
  session's CPU TEST; `GC_TAPE1 = round(GC_TAPE2 × 139 / 169)`; outside 4..12 % of Δcpu: leave TAPE and report. Rewrite
  the comments above the constants with the 0.9.1 figures (rows, late count, the fitted values, the budget at the
  session's CPU TEST: `1030 × 100 / (base × sty / 100)`); keep 0.9's sentences as history in one line each.
  `tests/gen_test.c`: `GT_X1` becomes `max(c for c in 1..99 if (c * 100 // 63) * max(sty, wild) // 100 <= 100)`
  (compute it: `python3 -c "S,W=<sty>,<wild>; print(max(c for c in range(1,100) if (c*100//63)*max(S,W)//100 <= 100))"`)
  and its comment's figures (`41 / 63 x 1.18 (0.77)`, `x 1.55 (1.00)`) are recomputed; every other figure gen_test
  prints or pins that moved (`grep -n "746\|569\|118\|155\|x1.18\|x1.55" tests/gen_test.c`, `GEN_HOUSE_HASH*`) is
  re-recorded, each named in the commit body. Never loosen a bound: the two-core "4 % at most" and "none with AUTO's
  AGE on" checks, the tape styles' "AUTO's AGE kept" check must pass as they are; if one fails, stop and report.

- [ ] **Step 6: Run to see them pass.** `gen_test` (all, the new check ok); `python3 tests/gen_fit_test.py`; `ui_test`,
  `stems_test`, `hush_test` (they build GEN pieces); `regress` (`tests/golden.txt` unchanged, or the GEN-made lines named:
  spec §4); `./build.sh`; `python3 tests/target_budget.py`; the full suite.

- [ ] **Step 7: The record.** `docs/braindancer/gen-calibration-0.9.1.md` (new): §1 setup (the calibration commit, the
  package, CORES, TRACKS, CPU TEST, NEW PROJECT, the knobs), §2 the data (the rows file; the 0.9.1 table from the CSV;
  the rows left out and why; 0.8's labels: pre-flight 5), §3 the fit (`build/cal/fit-091.txt` pasted: 0.9's and the
  fitted score, every row under both), §4 what changed in gen.c and what the gen_test sweep prints now (two cores
  over / AGE lowered / texture or arp lost, against 0.9's), §5 how strong it is (rows, one device, one session, the
  snapshot noise), §6 TAPE (the pair, or "not measured: GC_TAPE2 92 kept").

- [ ] **Step 8: Commit** — `git add firmware/src/gen.c tests/gen_terms.c tests/gen_test.c tests/gen_cal_rows.h tests/gen_fit_test.py tests/run_tests.sh tools/gen_fit.py docs/braindancer/gen-cal-rows.csv docs/braindancer/gen-calibration-0.9.1.md && git commit`
  with `fix(gen): the CPU model re-fitted on 0.9's pieces played on the FM-1 (0.9.1 session + 0.8's 15)`; body: the
  fitted constants, the score against 0.9's, the re-recorded figures, the image line.

---

### Task 10: Docs, the 0.9.1 news, release and the device (the implementer, then the controller with the user)

**Files:** `firmware/src/felucca.c:42` (`FELUCCA_VERSION "v0.9.1"`), `web/manual/nav.js` (regenerated: the version),
`README.md` (Status), `web/manual/data.js` (news), `docs/MANUAL.md`, `web/landing/index.html` (version line, if it names
one), `docs/braindancer/device-log.md`, new `docs/superpowers/plans/2026-10-31-braindancer-0.9.1-deferred.txt`,
`web/STABLE` (Step 6, on the user's yes).

- [ ] **Step 1 (implementer): Docs.** `README.md`: `## Status: 0.9` → `## Status: 0.9.1`, a paragraph above 0.9's:
  `0.9.1 is a clean-up release on top of 0.9: GEN's CPU model is measured again on 0.9's own pieces on the FM-1, so
  fewer pieces run it past its deadline (some heavy pieces are thinned a little more); the safety net keeps a GEN
  piece's texture and arp in hand through its sections and never hushes a part you rewrote; GEN's hint shows on every
  way in; SEQ's dice puts the level back with OCT−; KEYBOARD PLAYS SELECTED drops the pitch bend of the track it
  leaves.` `web/manual/data.js`: first in `news`:
```js
  {v: "0.9.1", items: [
    "0.9.1 is a clean-up release. GEN's CPU model is measured again on 0.9's own pieces played on the FM-1 (2 CORES): fewer pieces run the FM-1 past its deadline, and some heavy pieces are thinned a little more than in 0.9. One-core mode is unchanged.",
    "The safety net that quiets a GEN piece's texture and arp when the FM-1 is overloaded now keeps working when you jump to another section of the piece or the song ends, and it never quiets a track you have edited (its steps, its sound or its engine).",
    "GEN shows its hint (HOME AGAIN: TRACKS) however you open it: HOME, SELECT on HOME or HOME + SELECT.",
    "SEQ + SELECT (dice the sequence), then OCT−: the track's level comes back too, not only its steps and length.",
    "KEYBOARD PLAYS SELECTED: selecting another track resets the pitch bend of the track you leave.",
    "The printed cheat sheets and the navigation map's picture are up to date again."]},
```
  `docs/MANUAL.md`: where SEQ + SELECT's dice and its OCT− are described, add that the level comes back too; under
  **KEYBOARD PLAYS SELECTED** (`:1891`) add `Selecting another track resets the pitch bend of the track you leave.`;
  where GEN is opened (its section), `GEN shows its hint as a message however you open it.`; the GEN section's
  sentence about the budget: `calibrated on the device (2 CORES; measured again in 0.9.1)`. Plain upper-case control
  names, English only, no band names. `grep -n "0\.9\b" web/landing/index.html` — if a version line names 0.9 as the
  current release, say 0.9.1.
- [ ] **Step 2 (implementer): Release prep.** `felucca.c:42` `"v0.9.1"`; `python3 tools/gen_manual.py build/ui_new/ACID`
  (nav.js version; the renders from the suite's `build/ui_new`); `python3 tools/gen_cheatsheets.py` (the sheets
  name the firmware's version) and, if `git status` shows `docs/cheatsheet/*.html` changed, `python3
  tools/render_chromium.py` (Task 5's renders again, so `--verify` stays green; add the five files to the commit);
  the deferred list
  `docs/superpowers/plans/2026-10-31-braindancer-0.9.1-deferred.txt`: every item the task reports flagged, plus: a
  user's own section inside a GEN song keeps the GEN arm while the song plays (pre-flight 1); edits in the first 250 ms
  after an arm are taken as the piece's (pre-flight 2); one-core calibration (no data); the old 0.9 rows G01–G10
  (unnamed) not used; TAPE's model cost (measured or not, Task 9 §6); the triage's "left for later" list (TAPE / AGE
  quick access first). `./build.sh` (image ≥ 16 KiB free, RAM, the `layout:` line); `python3 tests/target_budget.py`
  (ISR 300 / TIMER5 2170 / slicer 821 / tape 780); `node web/test_web.mjs`, `node web/test_manual.mjs`, `python3
  tools/gen_manual.py --check`, `python3 tests/built_on_test.py`; the full suite green; commit `release prep:
  Braindancer 0.9.1 (the clean-up release)` with `firmware/src/felucca.c web/manual/nav.js README.md web/manual/data.js
  docs/MANUAL.md web/landing/index.html docs/superpowers/plans/2026-10-31-braindancer-0.9.1-deferred.txt`; tree clean.
- [ ] **Step 3 (controller): the whole-branch review.** Dispatch a code review of `main..cleanup-091` (spec §5);
  fixes as their own commits, the suite green after them.
- [ ] **Step 4 (controller + user): install 0.9.1.** Backup as Task 8 Step 1 (from 0.9.1-cal); `./build.sh --release
  0.9.1` on the clean HEAD; `tools/fm1_install.py build/braindancer-0.9.1.fwsc --port "BRAINDANCER" --yes`; status:
  `braindancer v0.9.1`, idle cpu, `audio_lost` 0, `safety_tape` 0, `gen_piece -`.
- [ ] **Step 5 (controller + user): verify on the device.** Conditions as Task 8 Step 3. (a) Item 1: 10 GENERATE presses,
  `tools/cal_piece.sh V<n> build/cal/cal-091-verify.log` each; `python3 tools/gen_cal.py csv build/cal/cal-091-verify.log
  docs/braindancer/gen-calibration-0.9.1-verify.csv`. Pass: at most 1 row with `late20 > 5`, `lost` 0 on every row, every
  piece led by RETROBOC or AMBIENT with `gen_age` above 0 (the tape styles keep their tape), no click the user hears.
  (b) Item 5: from TRACKS, HOME + SELECT: the user sees `HOME AGAIN: TRACKS`; from VISUAL, SELECT: the same. (c) Item 6:
  on a track playing a high part, SEQ + SELECT, then OCT−: the TRACKS level bar back where it was. (d) Item 7, only if
  a MIDI keyboard with a bend wheel is at hand: KEYBOARD PLAYS SELECTED ON, hold a bend on T3, select T4: T3 plays in
  tune. (e) Items 2–4 are host-tested (overload only): no device check. Write the device log section `## 2026-10-31 —
  Braindancer 0.9.1 (the clean-up release)` (backup, package sha256, install, status, each check with pass / fail, the
  verify table, `Proven commit: <sha>`); commit `docs: device log, Braindancer 0.9.1` with the verify CSV. Merge ff to
  main and push **only on the user's go-ahead**.
- [ ] **Step 6 (controller): `web/STABLE`, asking first.** Once 0.9.1 is proven and on main, ask the user: "Pin
  web/STABLE to the proven 0.9.1 commit <sha> (the public installer then builds it, not main)?" Only on yes:
  `printf '%s\n' <sha> > web/STABLE`, commit `release: pin web/STABLE to Braindancer 0.9.1 (<sha>)`, push on the user's
  go-ahead.

---

## Self-review (done while writing)

- Spec §1 item 1: the status fields Task 3 (pre-flight 4); the device session Task 8 (GENERATE ~20, 2 CORES, the stored
  CPU TEST); the re-fit Task 9 (0.8's 15 rows and the session's, late = more than 5 per 20 s, target pre-flight 7);
  "gen_test's budget figures re-recorded; the two-core bound (≤ 4 % over) holds": Task 9 Step 5 (re-recorded, bound
  never loosened); the device target (1 in 10, no audio lost, tape styles keep tape): Task 10 Step 5(a). Item 2: Task 1
  (section load, SONG END; pre-flight 1). Item 3: Task 1 (steps, sound, engine; pre-flight 2). Item 4: Task 1
  (pre-flight 3). Item 5: Task 2 (pre-flight 8). Item 6: Task 2 (pre-flight 9). Item 7: Task 2 (pre-flight 10).
  §2 item 8: Task 4 (pre-flight 11); item 9: Task 4 (pre-flight 12); item 10: Task 5 (pre-flight 13, report-and-leave).
  §3 item 11: Task 6 (save_on / LZ_SAVE conditional, knob arcs, `gs.grain_off` / `gs.cost_thin` → `GEN_PROBE`, the NSONG
  assert); item 12: Task 6 (segno, `isascii`, `LBL_ONLY`); item 13: Task 6 (emulator.yml, baseline.md, flash-map.md,
  editor.c, felucca.c); item 14: Task 7 (lines added only). §4 limits: Global Constraints; each task's Step 4 runs the
  build, the budget and the suite. §5 release: Task 10 (docs, news, the whole-branch review, the device session with
  item 1's measurements and the hint / dice checks, merge and push on the go-ahead, `web/STABLE` after asking).
- Placeholders: none. Values the device decides (the fitted constants, the session's date, commit hashes, N) are
  produced by named commands in the steps that use them; "<sha>", "<n>", "<N>" in commit and log lines stand for those
  outputs.
- Names used across tasks: `safe_part`, `safe_gen`, `safe_secs`, `safe_live`, `safe_arms`, `safe_arm(uint32_t)`,
  `safe_edit(uint32_t)`, `safe_clear`, `dsf_since`, `dsf_rearm`, `gs.wrote`, `trk_edit_sig(uint32_t)`,
  `safe_edit_poll(void)`, `song_keep_gen` (Task 1); `ux_home_hint`, `dk.level`, `midi_rsel` (Task 2); `gen_stat_t
  {piece, cost, budget, age, id, on}`, `gen_status(gen_stat_t *)`, `gen_id_fmt(const genid_t *, char *, uint32_t)`,
  `tools/gen_cal.py` (`parse`, `snaps`, `row`, `COLS`, `ROWS`, subcommands `delta csv ids rows`), `tools/cal_snap.sh`,
  `tools/cal_piece.sh` (Task 3, used in 8, 9, 10); `render_chromium.render_map(svg, png)` (Task 5); `GEN_PROBE`,
  `gt_probe`, `small_int`, `LBL_ONLY`, `table_kind`, `selfcheck`, `ST_NSONG`, `ST_NSONG_INDEX` (Task 6);
  `gc_terms_t {fixed, tape, ccomp, n, eng[NTRK], v[NTRK]}`, `gc_terms(int, gc_terms_t *)`, `gc_sum(const gc_terms_t *)`,
  `gcal_row_t {tag, wild, two, late, base, t}`, `GCAL`, `GCAL_N`, `tools/gen_fit.py` (`load cost budget judge fit
  emit_h THETA0 GRID dev2`) (Task 9). The rows CSV columns (`session,tag,name,wild,two,base,fixed,tape,ccomp,parts,
  late20,timed`) are the same in `gen_cal.py ROWS`, Task 9 Step 1's file and `gen_fit.load`; `gen_terms`'s output
  (`TAG|MATCH|FIXED|TAPE|CCOMP|parts|COST|NAME`) is what `gen_cal.py rows` splits.
- Order: Task 1 before Task 8 (the net as it ships while measured); Task 3 before Task 8 (the rows named); Task 6's
  `GEN_PROBE` and Task 9's split change no generator output (checked in Task 6 Step 5 and Task 9 Step 2), so the
  harness at Task 9 rebuilds what the calibration commit played.
