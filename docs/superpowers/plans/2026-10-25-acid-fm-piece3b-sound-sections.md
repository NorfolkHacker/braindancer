# ACID FM piece 3b (sound sections, genres, power-on demo) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ACID FM 0.4: EDIT, ENV, LFO, FX, ARP and SCL on the 3a look with ≤ 3 pages + MORE each, the held layers
restyled, ten generator genres, a power-on demo that plays, TRACKS calmer under load.

**Architecture:** JIANT's page model stays the backend: each new page is a `PAGES` row (`params.c`) and each section a
`CHAINS` entry (`ui_lens.c`), so knob edits keep going through JIANT's `edit_param(slot, steps)` path (undo, locks,
automation, recording unchanged). The ux layer draws them: a section renderer (`ux_section.c`) draws any page of these
six sections with the 3a widgets plus a per-page picture, and a new MORE list widget lists a section's remaining
settings. JIANT's old page drawing for these sections and the EDIT creatures are deleted as each section moves.

**Tech Stack:** C (JieLi clang / host gcc via `tools/host-cc`), Python generators, Node for web tests.

**Spec:** `docs/superpowers/specs/2026-10-25-acid-fm-ui-3b-sound-sections.md` (authority for every page table and
rule); parent `docs/superpowers/specs/2026-10-24-acid-fm-ui-design.md` (§0 intuitive first, §3 look, §4.1 rules, §5,
§6).

## Global Constraints

- GPL-3.0-only; never remove JIANT/Felucca copyright lines; changed files gain `Portions Copyright (C) 2026
  NorfolkHacker`; new files `SPDX-License-Identifier: GPL-3.0-only` + `Copyright (C) 2026 NorfolkHacker`.
- Setting ids (`P_*`, `G_*`, menu ids), stored formats, the editor protocol and MIDI CCs do not change. If the web
  editor enumerates `PAGES` titles (check `editor.c` / `web/EDITOR_PROTOCOL.md` for page listings), keep what it reads
  compatible or document an append-only change; `web/test_web.mjs` must pass.
- Goldens (sound) unchanged, except Task 2's power-on demo if a golden renders power-on parts (list it).
- Budgets: app slot 584,336 B (now 562,948); RAM .data+.bss < 98,304 (79,396); **no new `.pool` data** (spare 8,416,
  floor 8,192); DC cells per page ≤ 1,920; tables in `.rodata`.
- Every new `.c` is added to `firmware/src/felucca.c` (or the file that includes UI parts), `tests/ui_test.c`'s include
  list, and `tools/size_fns.py` `SIZE_FILES`.
- Every status / label / MORE row literal passes the 3a glyph scan (`tests/ux_msg_scan.py`); extend the scan to the
  new literal sites.
- Knob colours K1 ACID, K2 SLIDE, K3 VIOLET, K4 ACCENT; labels in plain words (spec §0 rule 4); `ux_knob_touch` on
  every knob turn.
- Environment: `cd ~/synth/mvavefirmware && . .venv/bin/activate && export JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK PATH=$PWD/.venv/bin:$PWD/build/node/bin:$PATH`;
  build `./build.sh`; suite `CC=tools/host-cc sh tests/run_tests.sh` (check the web groups ran); renders
  `build/ui_new/ACID/`.
- Process: run everything in the foreground (Bash timeout ≤ 600000 ms); never start background wait loops; stop
  anything you start before reporting.
- Branch `piece3b-ui`; commits end with a blank line + `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`;
  never on main, never push; no device access (Task 11 is the controller's with the user).

## Codebase map (inventory 2026-10-25; verify line numbers)

- `ui_lens.c`: `CHAINS` (`:46-105`) of `nv_t{PAGES title, name, group, level}`; `CE_SYN` / `CE_FM6` / `CE_DRM` EDIT
  chains by `edit_kind` (`:35`), `CE_ENV` (`:86-88`), `CE_LFO` (`:89-91`), `CE_FX` (`:75-82`), SCL (`:92-93`), ARP
  (`:94-95`); `nv_page` (`:126`) shows an entry only if a visible PAGES row has that title; visibility `ui.c:542-575`.
- `params.c`: `PAGES` (`:567-680`), `TP` track params (`:83`), `GP` globals (`:169`); formats `F_PCT`, `F_BIPCT`,
  `F_TIME`, `F_CUTOFF`, `F_LFOHZ`, `F_DB`; `param_format`.
- `ui_input.c`: `edit_param(slot, steps)` (`:1075`) dispatches by `cur_page()->graph` (special pages GR_DXSND →
  `dx_knob`, GR_FOCUS → `focus_knob`, …) and otherwise turns the PAGES row's param (`param_turn`, `knob_cap`).
- Engines' EDIT pages: `.edit[]` names + `.page_title` (`eng_analog.c:275`, `eng_phase.c:227`, `eng_lofi.c:629` with
  mode relabels `:374`, `eng_noise.c:258` relabels `:45`, `eng_formant.c:447` WORDS relabels `:293-309`, `eng_fm6.c:521`,
  `eng_drum.c:592`); FM6 operator pages `ui_fm6op.c`; DRUM-X pages `ui_dx.c:37-41`.
- Layers `ui_layer.c:56-66` (`LAYERS`), `layer_key :559`, `LY_SCL :69`; FX performance `perform.c:56-72`, `pfx.c`.
- Generator: `GENRE[]` `gen_tables.c:403-585`, `NGENRE :586`, `GEN_EXPLORE :592`; `gen_knob` `gen.c:2674` (walk
  `:2680-2682`), names/gauge `:2710`, `:2736`; random draws `gr() % NGENRE` at `gen.c:2319`, `:2325`, `:2616`; form
  `:2778`; seed hashing uses stored `gp.genre` (`:1669`, `:2551`).
- Power-on: `TRK_DEF` `engines.c:162` {engine, preset, pattern}; `power_on_parts` `ui.c:1472` (honours the pattern
  column via `load_pat16`), `system_init` `project.c:1872` (NEW PROJECT: empty), `power_sound_of` `ui.c:1456`;
  `PATTERNS[]` `engines.c:121-155` (1 ACID, 5 PAD, 12 BEAT, …); ANALOG preset "ACID" `eng_analog.c:249`.
- 3a ux layer: `ux_theme.h`, `ux_widgets.h` (`ux_header`, `ux_knobs`, `ux_knob_touch`, `ux_panel_begin/end`,
  `ux_status`, `ux_footer`, `ux_transport`, `ux_pace`, `UX_Y_*`), `ux_nav.h` (HOME), `ux_home.c`.

## File map

| File | Change |
|---|---|
| `firmware/src/gen.c`, `gen_tables.c` | genre filter + display names (Task 1) |
| `firmware/src/engines.c`, `ui.c` | power-on demo (Task 2) |
| `firmware/src/ux_more.h/.c` (new) | MORE list widget + model (Task 3) |
| `firmware/src/ux_section.h/.c` (new) | section renderer + per-page pictures (Tasks 3–8) |
| `firmware/src/params.c`, `ui_lens.c`, `ui_input.c`, `ui.c` | new PAGES rows / CHAINS for the six sections; MORE paging (Tasks 4–8) |
| `firmware/src/ui_engvis.c`, `ui_being.c`, `ui_organic.c`, `ui_graph.c`, `ui_lzedit.c`, `ui_lzfx.c`, `ui_lzpage.c` | old drawing for these sections and the creatures deleted as sections move |
| `firmware/src/ui_layer.c` | layers restyled (Task 9) |
| `firmware/src/ux_home.c` | TRACKS half rate under load (Task 10) |
| tests: `tests/ui_test.c`, `tests/ui_render.c`, `tests/gen_test.c`, `tests/ux_msg_scan.py`, new `tests/ux_cover_test.c` | per task |
| `firmware/src/felucca.c`, `README.md`, `web/landing/index.html`, `web/manual/data.js`, `docs/acid-fm/device-log.md`, `docs/superpowers/plans/2026-10-25-acid-fm-piece3b-deferred.txt` | Task 11 |

---

### Task 1: Ten generator genres

**Files:** `firmware/src/gen.c`, `firmware/src/gen_tables.c`, `tests/gen_test.c`.

**Interfaces — Produces:** `static const uint8_t GEN_SHOWN[10]` (indices into `GENRE[]`, in spec order) and
`static const char *const GEN_SHOWN_NAME[10]`; `uint32_t gen_shown_index(uint32_t genre)` (position or 255);
`uint32_t gen_random_genre(void)` (returns a stored genre value 1 + index of a shown genre).

Spec §3: IDM, BROKEN BEAT (BROKEN), BREAKS, DRILL'N'BASS (DRUM & BASS), ACID (ACID HOUSE), ELECTRO, MICRO (MICRO
HOUSE), RETROBOC (BOOM BAP), AMBIENT, DRIFT (IMPRESSION); plus EXPLORE and RANDOM.

- [ ] Step 1 — failing tests in `tests/gen_test.c`: walking `gen_knob(0, ±1)` from EXPLORE visits exactly EXPLORE,
  RANDOM and the ten in order (with their display names from the name function) and wraps; RANDOM / mutant draws over
  ≥ 2,000 seeds never produce a hidden genre; a stored hidden genre value (e.g. HOUSE) still generates the same piece
  as before (hash of the generated song identical to the base commit's, captured in the test) and its name shows as
  JIANT's. RED.
- [ ] Step 2 — implement: filter only the walk (`gen.c:2680-2682`), the name/gauge (`:2710`, `:2736`, gauge over 12
  positions) and every `gr() % NGENRE` draw (`:2319`, `:2325`, `:2616` → `gen_random_genre()`); stored values,
  `GENRE[]` order and seed hashing unchanged. GREEN; suite; commit `feat(gen): ten IDM genres shown; RANDOM picks among them`.

---

### Task 2: Power-on demo

**Files:** `firmware/src/engines.c` (`TRK_DEF`), `firmware/src/ui.c` (`power_on_parts`, a HOME hint), `firmware/src/ux_home.c`
(status hint), tests in `tests/ui_test.c` (or `tests/startup_test.c`).

Spec §4: with nothing to restore, T1 DRUM-X with PATTERNS BEAT (12), T5 ANALOG preset "ACID" with PATTERNS ACID (1),
T6 FM6 with the first factory preset of sound type PAD (else the first KEYS) and PATTERNS PAD (5); T2–T4 sounds, empty
patterns; tempo 124; HOME status "DEMO: EDIT ANYTHING, OR HOME → GENERATE" until the first edit (any knob turn,
step write, preset load) or a PLAY→STOP cycle; NEW PROJECT (`system_init`) still empty; RESTORE LAST unchanged.

- [ ] Step 1 — failing tests: fresh boot, no autosave → T1/T5/T6 have the patterns' steps and the presets (T5's
  preset name "ACID"), bpm 124, PLAY starts voices on T1/T5/T6 within one bar (host: run the sequencer); the demo hint
  shows on HOME and clears after a knob turn and after PLAY→STOP; NEW PROJECT → all patterns empty; RESTORE LAST with
  an autosave → the autosave, not the demo. RED.
- [ ] Step 2 — implement (choose the FM6 pad by sound type at build or boot time, not by hard-coded index if the
  type is available; else document the index). Check goldens; if a power-on golden changes, list it in the commit.
  GREEN; suite; commit `feat: power-on demo: beat, acid line, pad; NEW PROJECT stays empty`.

---

### Task 3: The MORE list and the section renderer

**Files:** create `firmware/src/ux_more.h/.c`, `firmware/src/ux_section.h/.c`; `firmware/src/ui_draw.c` (route the six
sections' pages to `ux_section_draw()` behind a per-section switch so sections move one at a time),
`firmware/src/ui_input.c` (MORE input), register files; tests.

**Interfaces — Produces:**
```c
typedef struct { uint16_t id; uint8_t kind; } ux_row_t;   /* kind: UXR_TRACK (P_*), UXR_GLOBAL (G_*), UXR_ACTION, UXR_HEAD (a section heading row) */
typedef struct { const ux_row_t *rows; uint8_t n; } ux_more_t;
void ux_more_draw(const ux_more_t *m, uint32_t sel, uint32_t top);   /* panel list: name, value (param_format), picked row highlighted */
int  ux_more_turn(const ux_more_t *m, uint32_t *sel, uint32_t *top, uint32_t knob, int32_t steps); /* K1 picks a row, K2 sets it (through the same param write as edit_param), SELECT scrolls */
void ux_section_draw(void);            /* draws cur_page(): header, ux_knobs from the PAGES row, the page's picture, status, footer "SECTION · PAGE n/N" */
int  ux_section_owns(uint32_t sec);    /* 1 once a section has moved to ux */
```
- A MORE page is a PAGES row whose graph is a new `GR_MORE` with a pointer to its `ux_more_t`; the MORE rows for each
  section are filled in by Tasks 4–8. Rows hidden by JIANT's visibility rules (LOAD ECO chorus/slicer, engine-specific
  pages) are skipped. Values edited from MORE go through the same write path `edit_param` uses for a PAGES knob (factor
  that path out of `edit_param` into `static void param_knob(track_t *t, uint32_t id, int32_t steps, uint32_t knob)`
  so undo/locks/automation/recording behave identically); actions ask first (OCT+ yes / OCT− no) when JIANT did.
- The renderer reads the four knob labels from the PAGES row (plain-word labels per spec — add a `label` override
  table where JIANT's label is cryptic, e.g. `RESO`, `ATTACK`), values via `param_format`, bars from the value's
  position, colours K1–K4; the footer shows `SECTION · PAGE` and `n / N` (N counts MORE); status line: first-time hint
  per section ("EDIT AGAIN: NEXT PAGE", §0 rule 7, once per power-on), the turned knob's full name and value, refusal
  messages.
- [ ] Step 1 — failing tests: a MORE list of 20 rows scrolls with SELECT, K1 picks, K2 changes the picked setting and
  the value equals what JIANT's page edit would produce for the same id/steps (compare against `edit_param` on the old
  page), undo restores it; a hidden row is skipped; the renderer draws a demo section page (S_UX_SECTION) lint-clean;
  the footer counts pages incl. MORE. RED.
- [ ] Step 2 — implement; nothing routes to ux yet except the test page; GREEN; suite; commit
  `feat(ui): the MORE list and the section renderer`.

---

### Task 4: EDIT — the synth engines (ANALOG, PHASE, LOFI, NOISE, VOICE)

**Files:** `params.c` (PAGES rows), `ui_lens.c` (`CE_SYN`), `ux_section.c` (waveform picture), `ui_input.c`, the
old EDIT drawing for synths (`ui_lzedit.c`, `vx_*` synth pictures in `ui_engvis.c`, `graph_being`) deleted when
unused; tests (`ui_test.c` navigation + renders, `tests/ux_cover_test.c` new).

Spec §2.1 rows ANALOG and PHASE/LOFI/NOISE/VOICE: page 1 SOUND = E1–E4, page 2 TONE = E5–E8, page 3 VOICE (ANALOG) or
FILTER (others), MORE as listed (incl. VOICE's LANGUAGE, LYRIC, POSITION, TYPE LYRIC action; LOFI RETRIG). FOCUS
dropped. Mode-dependent relabels kept (LOFI WRAM/BYTE/FLOAT, NOISE LFSR/META, VOICE WORDS). Picture: the engine's
waveform (reuse the cheapest existing source: JIANT's scope buffer or a short render of the oscillator's shape; must
cost ≤ one panel redraw per knob change, none per frame while idle).
- [ ] Step 1 — failing tests: for each of the five engines, EDIT walks SOUND → TONE → page 3 → MORE → SOUND
  (EDIT press) and SELECT stops at the ends; the knob labels per spec; a knob turn on each page changes the spec'd id
  through `edit_param`; MORE lists every row; the **coverage test** (`tests/ux_cover_test.c`): every id JIANT's
  synth EDIT chain reached (enumerate the base commit's `CE_SYN` pages' ids at test-build time from `params.c` /
  `ui_lens.c` of `git show <base>`, or a reviewed constant list) is on a new page or MORE row; renders per engine
  (S_EDIT_ANALOG_1..3, S_EDIT_ANALOG_MORE, …) lint-clean. RED.
- [ ] Step 2 — implement; switch `ux_section_owns(SEC_EDIT)` on for synth kinds; delete the synth creature drawing
  and FOCUS once unused (keep FM6/DRUM-X paths for Task 5). GREEN; suite; size; commit `feat(ui): EDIT for the synth engines`.

---

### Task 5: EDIT — FM6 and DRUM-X

**Files:** as Task 4 plus `ui_fm6op.c`, `ui_dx.c` (their knob handlers stay; their drawing moves to ux).

Spec §2.1 rows FM6 and DRUM-X (OP and LANE shared across pages and MORE, one selection per track). Pictures: FM6 the
algorithm diagram (operators as dots in the blend colours joined by lines; the picked OP outlined); DRUM-X eight lane
cells with the picked lane lit and its level bar.
- [ ] Step 1 — failing tests: FM6 pages 1–3 + MORE (OP picked on OPERATOR is the same on OP ENV and in MORE; editing
  an op value marks the patch OWN as JIANT does); DRUM-X KIT / SOUND / SOUND 2 + MORE (LANE shared; levels `P_LN*`;
  kit values per section as JIANT); coverage for `CE_FM6` / `CE_DRM` ids and patch/kit fields; renders lint-clean;
  DRUM black-key mutes still work (layer unchanged). RED.
- [ ] Step 2 — implement; delete the remaining EDIT creatures (`vx_*` FM6 colony/kit pictures, `graph_colony`,
  `ui_being.c` if unused, `ui_organic.c` helpers if unused — grep), keeping `og_*`/`bg_energy`/`heat_col` while any
  caller remains. GREEN; suite; size (record what deleting freed); commit `feat(ui): EDIT for FM6 and DRUM-X; the creatures are gone`.

---

### Task 6: ENV, ARP, SCL

**Files:** `params.c`, `ui_lens.c` (`CE_ENV`, ARP, SCL chains), `ux_section.c` (envelope shape, arp dots, scale
keyboard pictures), `ui_input.c`; old drawing for them deleted; tests.

Spec §2.2 and §2.5 (ENV: AMP, MOD ENV; STAGE view dropped. ARP: ARP, ARP 2. SCL: KEY, CHORD with ROTATE `P_SOFS`;
MORE: PITCH OFFSET `P_POFS`, TRANSPOSE `P_TRANS`).
- [ ] Step 1 — failing tests: navigation per section; labels; edits through `edit_param`; ROTATE and PITCH OFFSET
  editable without the SCL layer; coverage for these chains + `LY_SCL`; renders lint-clean. RED.
- [ ] Step 2 — implement; GREEN; suite; commit `feat(ui): ENV, ARP and SCL`.

---

### Task 7: LFO

**Files:** `params.c`, `ui_lens.c` (`CE_LFO`), `ux_section.c` (LFO wave, destination bars, matrix lines),
`ui_input.c`; old drawing deleted; tests.

Spec §2.3 (LFO, DEST, MOD MATRIX; MORE: PHASE, MSEQ LENGTH / SLEW / DIVISION / STEP 1–16). The scrolling wave obeys
`ux_pace` (no faster than the ridgelines; frozen when shedding).
- [ ] Step 1 — failing tests: navigation; MOD MATRIX slot pick + SOURCE/DEST/AMOUNT equal JIANT's MOD page results;
  MSEQ steps editable from MORE; coverage; pace of the wave panel (host scheduler test); renders. RED.
- [ ] Step 2 — implement; GREEN; suite; commit `feat(ui): LFO`.

---

### Task 8: FX

**Files:** `params.c`, `ui_lens.c` (`CE_FX`), `ux_section.c` (send bars, echo/tail line, master meter), `ui_input.c`;
old FX drawing (`ui_lzfx.c` parts) deleted; tests.

Spec §2.4 (SENDS, SPACE, MASTER; MORE as listed incl. SLICER steps and PUNCH CLEAR asking first; CHORUS/SLICER rows
hidden under LOAD ECO; DJ FILTER stays GLO's).
- [ ] Step 1 — failing tests: navigation; SENDS per track; SPACE/MASTER globals; REVERB's 4th-knob label per type in
  MORE (PRE / AIR / PITCH); LOAD ECO hides the rows; PUNCH CLEAR asks; coverage for `CE_FX` (all 16 old pages);
  renders. RED.
- [ ] Step 2 — implement; GREEN; suite; commit `feat(ui): FX`.

---

### Task 9: The held layers restyled

**Files:** `firmware/src/ui_layer.c` (EDIT, LFO MACRO, FX, SCL layers' overlays), tests.

Spec §2.6: same functions; overlay drawn in UX colours / UF fonts (key map, knob row with K1–K4 colours, status line
naming the layer and what OCT/keys do). EDIT layer K1–K4 = the engine's page 1.
- [ ] Step 1 — failing tests: each layer's keys/knobs do exactly what they did (existing layer tests stay green, plus
  renders S_LAYER_EDIT / _MACRO / _FX / _SCL lint-clean); the layer overlay uses ux colours (sample pixels). RED.
- [ ] Step 2 — implement; GREEN; suite; commit `feat(ui): the EDIT, MACRO, FX and SCL layers in the ACID look`.

---

### Task 10: TRACKS under load and the [3b] backlog

**Files:** `firmware/src/ux_home.c`, `ux_widgets.c` (pace), tests; `docs/superpowers/plans/2026-10-24-acid-fm-piece3a-deferred.txt` (mark done).

- TRACKS ridge panel pace while `load_tight`: 100 → 200 ms (spec §5); LOAD LITE/ECO also 200 ms.
- Tempo on HOME (spec §5): TRACKS knobs LEVEL · LEVEL · LEVEL · BPM; the three-track window follows the selected track; BPM edits `G_BPM` as GLO › GLOBAL does (range, accel, BPM LOCK, external clock shows "CLOCK: USB"/"CLOCK: TRS" on a turn); ridgelines mark the three controlled tracks in K1–K3 colours; tests: window per selected track, BPM turn equals GLO's edit, lock/clock cases, render S_HOME_BPM.
- HC font slot thrash on EDIT pages: profile which Huffman faces each EDIT page draws; keep ≤ 2 per page or pin the
  slot assignment; one sentence in the report.
- Knob-value width test: every value string any 3b knob can show (iterate each id's range through `param_format`) fits
  its knob column in UF_VAL.
- [ ] Step 1 — failing tests (pace intervals; value widths). Step 2 — implement; GREEN; suite; commit `fix(ui): TRACKS half rate under load; value widths checked`.

---

### Task 11: Version 0.4, docs, device

**Files:** `firmware/src/felucca.c` (`v0.4`), README Status, `web/landing/index.html`, `web/manual/data.js` (0.4 news
entry ES+EN; the EDIT/ENV/LFO/FX/ARP/SCL manual entries corrected briefly — full rewrite stays 3d), regenerate what
tests pin, `docs/acid-fm/device-log.md`, `docs/superpowers/plans/2026-10-25-acid-fm-piece3b-deferred.txt`.

- [ ] Step 1 (implementer) — version + docs; full suite; commit `release prep: ACID FM 0.4 (sound sections, genres, demo)`; tree clean.
- [ ] Step 2 (controller + user) — fresh backup; `./build.sh --release 0.4` on the clean HEAD; install; serial
  `status`; power-cycle → the demo plays on PLAY; the no-manual check ("make T5's acid line brighter", "add delay to
  T3", "change the scale to minor", "press PLAY on a fresh boot"); CPU TEST late blocks/s on MENU, TRACKS, an EDIT page,
  the LFO page; log; merge ff to main and push on the user's go-ahead.
