# ACID FM piece 3c-1 (SEQ, GLO, the MIDI page) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ACID FM 0.5: SEQ (STEPS, STEP EXTRAS, PATTERN, MORE) and GLO (LIVE, MIX, MIDI, MORE) on the 3b section
engine, their held layers restyled, a per-track MIDI page with clock source and clock out made visible, and ACID FM
presented as its own firmware built on JIANT FM and Felucca.

**Architecture:** As in 3b, the inherited page model stays the backend: each new page is a `PAGES` row (`params.c`)
and each section a `CHAINS` entry (`ui_lens.c`), so knob edits keep going through `edit_param(slot, steps)` (undo,
locks, automation, recording unchanged). `ux_section.c` draws the pages with the 3a widgets plus a per-page picture;
`ux_more.c` lists each section's remaining settings; `ux_layer.c` draws the held layers. MIDI routing changes from one
global (`G_ROUTE`) to per-track IN / OUT / CHANNEL stored in a new appended project block, with `G_ROUTE` read once on
load for old projects. Inherited SEQ / GLO drawing is deleted as each section moves.

**Tech Stack:** C (JieLi clang / host gcc via `tools/host-cc`), Python generators, Node for web tests.

**Spec:** `docs/superpowers/specs/2026-10-26-acid-fm-ui-3c1-seq-glo-midi.md` (authority for every page table and
rule); parents `docs/superpowers/specs/2026-10-24-acid-fm-ui-design.md` (§0 intuitive first, §3 look, §4.1 rules, §4.4
SEQ STEPS, §5, §6) and `docs/superpowers/specs/2026-10-25-acid-fm-ui-3b-sound-sections.md` (the section engine).

## Global Constraints

- GPL-3.0-only; never remove JIANT/Felucca copyright lines; changed files gain `Portions Copyright (C) 2026
  NorfolkHacker`; new files `SPDX-License-Identifier: GPL-3.0-only` + `Copyright (C) 2026 NorfolkHacker`.
- Setting ids (`P_*`, `G_*`, menu ids), the editor protocol and MIDI CCs do not change. **New stored data is appended
  only** (spec §4.3): a project or settings blob written by 0.4 or older loads and behaves as before; NEW PROJECT keeps
  the MIDI settings like `KEEP[]` in `project.c:1873`. If the web editor enumerates `PAGES` titles or globals (check
  `editor.c`, `web/EDITOR_PROTOCOL.md`), keep it compatible; `web/test_web.mjs` must pass.
- Goldens (sound) unchanged.
- Budgets at 0.4: image 552,092 B of 584,336 (32,244 free) — 3c-1 must leave **≥ 16 KiB free** (spec §6); RAM
  .data+.bss 80,612 of 98,304; **no new `.pool` data** (spare 9,952); DC cells per page ≤ 1,920; tables in `.rodata`;
  target budgets (`tests/target_budget.py`) unchanged unless a task explains the cause (the ISR-reached variables are
  pinned by `tools/build.py`; its guard fails the build if one is missed).
- Every new `.c` is added to `firmware/src/felucca.c` (or the file that includes UI parts), `tests/ui_test.c`'s include
  list, `tools/size_fns.py` `SIZE_FILES` and `web/emu/felucca_web.c` if the emulator includes UI parts.
- Every status / label / MORE row literal passes the glyph scan (`tests/ux_msg_scan.py`); extend the scan to new
  literal sites. Values fit their knob column (`ux_cover_test.c` width sweep; wide names go on its allowed list only
  with a reason).
- Plain words (UI spec §0 rule 4), one short form per word everywhere; no band names on the device; user-facing text
  says ACID FM, provenance as "built on JIANT FM and Felucca" (memory rule).
- Knob colours K1 ACID, K2 SLIDE, K3 VIOLET, K4 ACCENT; `ux_knob_touch` on every knob turn; pictures paced with
  `ux_pace` / `ux_pace_at` (never freeze under load, never redraw every frame without a change).
- Environment: `cd ~/synth/mvavefirmware && . .venv/bin/activate && export JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK PATH=$PWD/.venv/bin:$PWD/build/node/bin:$PATH`;
  build `./build.sh`; suite `CC=tools/host-cc sh tests/run_tests.sh`; renders `build/ui_new/ACID/`.
- Process: run everything in the foreground (Bash timeout ≤ 600000 ms); **never start background wait loops** (no
  `until pgrep` / `while pgrep`); stop anything you start before reporting; never leave HEAD detached.
- Branch `piece3c-ui`; commits end with a blank line + `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`;
  never on main, never push; no device access (Task 11 step 2 is the controller's with the user).

## Codebase map (inventory 2026-10-26; verify line numbers)

- Chains `ui_lens.c:74-108` (CE_SEQ groups PATTERN/TRACK/BAR/STEP/AUTO, opens on STEP via `NAV_DEF`; CE_GLO groups
  LIVE/MIX/GLOBAL/SYSTEM, opens on MIXER); nav rules `ui_lens.c:8-19`; PAGES rows `params.c:783-830`; knob labels
  `ui_draw.c:1176-1570`; action pages `ui.c:1797-1830`, `ui_input.c` `act_do` (~1262).
- SEQ pages today: PATTERN (`P_SLEN P_SDIV P_SSWING P_SGATE`), STRETCH (STRETCH REPEAT NUDGE ROTATE, `ui.c shp_*`),
  PLAYHEADS (`fugue.c`), TRACK (`P_POFS P_TCHAN P_HVEL P_HRQ`), MUTATE (`P_MSND P_MPAT P_MAMT P_MMODE`), STEP (STEP NOTE
  TIME FLAG; DRUM grid STEP LANE HIT ACC `ui_draw.c:1497`), NOTE (FOCUS PITCH MOVE MICRO), COMP / COMP 2 (STEP TYPE
  + type-dependent CHANCE RATCH COND MODE RANGE SHIFT BEND EVERY TO STEP / RATE LAP), CHANCE (STEP CHANCE RATCH),
  AUTOMATION (PLAY EVENT LOCK CLEAR → CF_CLEAR_MOTION), AUTO LIST (ROW STEP PARAM VALUE, `ev_oct`), PHRASES (via
  PRESETS). Lock card `ui_draw.c:1464`. SEQ held + SELECT = CF_DICE_SEQ (`ui_input.c` ~2110).
- GLO pages today: MANO (`ui_live.c:4-24`: RATE LOOP THIN DJ FILTER; keys), RIELES (PISTAS RATE CORTE DENS), MIXER
  (`P_LEVEL`, PAN, REV, `G_DJF`), SENDS (`P_DIST P_CHOR P_DLY P_REV`), GLOBAL (`G_BPM G_SWING G_CLOCK G_TUNE`), SYSTEM
  (`G_TRKS G_LOADP G_ROUTE G_INFO`, picture `graph_system` `ui_lzpage.c:12-63`). `G_MIDI` (`params.c:183`) on no page.
- Layers `ui_layer.c:5-67`: GLO SET (`:551-600`), SEQ TOOLS (`ui_tools.c:4-12`), REC SET; 3b layers drawn by
  `ux_layer.c` (`ux_layer_owns`, `uxl_state`, per-layer view bits `ui.uxlv`).
- Roll held-step knobs: `lock_turn` / `lock_keys` / `lock_select` (`ui_input.c` ~678-731); ARP chord entry `ent_knob`
  (`ui_input.c` ~2525); external-clock refusal in `param_knob` (`ui_input.c:1055`); GLO+SELECT tempo (`ui_input.c`
  ~2512); TAP (`ui_layer.c` ~338).
- MIDI: `midi_track(ch)` `seq.c:2217` (CH1-6 → parts 1-6, SEL → TSEL); `midi_channel_t.targets` runtime mask
  (`midi_control.c:10-44`); Program Change: ch 16 section / project (`midi_control.c:388`), else sound (`:411`); out:
  `trk_midi_ch(i) = i % NTRK` (`seq.c:54`), `mo_send`, MO_KEYS/CLOCK/ALL/OFF (`seq.c:58-66`), `mo_clock_on`
  (`seq.c:117`), `midi_out_mode` = `favorites.factory[15][24]` (`ui.c:122`); clock follow `midi_clock.c`,
  `seq.c:2303-2330`; TRS input only (`midi_uart.c:3-9`).
- Storage: song globals `core.h:150-186` (`G_ROUTE` `:154`, `G_COUNT`); project blocks appended by tag (`'DM'`, `'DT'`,
  `'PX'` in `project.c`); load clamps `project.c:1799-1801`; NEW PROJECT keeps `KEEP[5]` (`project.c:1873`); settings
  `settings_persist.c:1-71` (spare bytes `favorites.factory[15][*]`).
- MENU › MIDI rows `menu_items.c` (MIDI OUT, LEARN, LEARN CLEAR); ABOUT text `ui_menu.c:240-253`.
- 3b section engine: `ux_section.h/.c` (`ux_section_draw`, `ux_section_owns`, per-page pictures, `uxs_knobs`),
  `ux_more.h/.c` (`ux_row_t {id, kind}`, `UXR_TRACK/GLOBAL/ACTION/HEAD`, `ux_more_draw`, `ux_more_turn`), `GR_MORE`.

## File map

| File | Change |
|---|---|
| `firmware/src/core.h`, `project.c`, `seq.c`, `midi_control.c`, `midi_clock.c` | per-track MIDI routing + storage (Task 1); tempo writers (Task 8) |
| `firmware/src/ux_seq.c` (new) | SEQ pictures: roll, strip, drum grid, extras, pattern (Tasks 2–3) |
| `firmware/src/ux_glo.c` (new) | GLO pictures: LIVE timeline, MIX columns, MIDI table (Tasks 6–8) |
| `firmware/src/ux_section.c`, `ux_more.c`, `ux_layer.c`, `params.c`, `ui_lens.c`, `ui_input.c`, `ui.c`, `ui_draw.c` | pages, MORE rows, layers, roll fixes |
| `firmware/src/ui_lzpage.c`, `ui_live.c`, `ui_tools.c`, `ui_draw.c` | inherited SEQ / GLO drawing deleted as sections move |
| `firmware/src/menu_items.c`, `ui_menu.c` | MENU › MIDI pointer row (Task 8); ABOUT wording (Task 10) |
| web: `web/index_pkg.html`, `web/404.html`, `web/manual/*`, `web/editor.html`, `web/fm1*.js` | "built on" wording (Task 10) |
| tests: `tests/ui_test.c`, `tests/ui_render.c`, `tests/ux_cover_test.c`, `tests/ux_msg_scan.py`, new `tests/midi_route_test.c` (or extend an existing MIDI test) | per task |
| `firmware/src/felucca.c`, `README.md`, `web/landing/index.html`, `web/manual/data.js`, `docs/acid-fm/device-log.md`, `docs/superpowers/plans/2026-10-26-acid-fm-piece3c1-deferred.txt` | Task 11 |

---

### Task 1: Per-track MIDI routing and storage (no UI)

**Files:** `firmware/src/core.h`, `firmware/src/project.c`, `firmware/src/seq.c`, `firmware/src/midi_control.c`,
`firmware/src/params.c` (names/ranges of the new settings), tests (new `tests/midi_route_test.c` registered in
`tests/run_tests.sh`, or the existing MIDI test file if one covers `midi_control.c`).

**Interfaces — Produces:**
```c
/* song state, saved in a new appended project block 'MR' (tag style as 'DM'/'DT'/'PX') */
typedef struct { uint8_t in, out, ch; } trk_midi_t;          /* in/out 0|1, ch 0..15 */
extern trk_midi_t trk_midi[NTRK];
extern uint8_t midi_kbd_sel;                                 /* KEYBOARD PLAYS SELECTED 0|1 */
uint32_t midi_in_mask(uint32_t ch);      /* bit i set: track i receives channel ch (IN on and CHANNEL ch), plus TSEL when
                                            midi_kbd_sel and no track claims ch */
uint32_t trk_midi_ch(uint32_t i);        /* now trk_midi[i].ch (was i % NTRK) */
int      trk_midi_out(uint32_t i);       /* trk_midi[i].out && NOTES OUT (midi_out_mode keys bit) */
void     trk_midi_defaults(void);        /* T1..T6: in 1, out 1, ch 0..5; midi_kbd_sel 0 */
```
If the settings are better held as new `G_*` song globals appended after `G_PFXRES` (one per track per field + one for
KEYBOARD PLAYS SELECTED) so `edit_param`, undo and the editor see them, use that instead and expose the same functions;
state the choice in the report. Either way they are saved in an appended block and load with defaults when absent.

Spec §4.2–4.3.

- [ ] Step 1 — failing tests:
  - defaults: T1–T6 receive channels 1–6 and send on 1–6; nothing else receives;
  - T2 set to channel 10: a note on channel 10 starts a voice on T2 only; a note on channel 2 starts nothing (IN by
    channel); two tracks on the same channel both play;
  - IN off on T3: channel 3 notes are ignored; OUT off on T4: T4's played and sequenced notes send nothing over USB
    (`mo_send` captured); NOTES OUT off: no track sends; CLOCK OUT (clock bit of `midi_out_mode`) sends 24 ppq only
    with CLOCK INT (as `mo_clock_on`);
  - KEYBOARD PLAYS SELECTED on: a note on an unclaimed channel plays the selected track; on a claimed channel it plays
    the claiming track only;
  - Program Change: channel 16 still selects a section / project even when a track is on channel 16; on channel c it
    loads a sound on every track receiving c;
  - storage: a project saved before this change (no 'MR' block; `G_ROUTE` 0) loads to the defaults; `G_ROUTE` 1 (SEL)
    loads with KEYBOARD PLAYS SELECTED on; a project saved with changed routing round-trips; NEW PROJECT keeps the
    routing; a 0.4-format project still loads byte-identical in every other field.
  RED (the functions don't exist).
- [ ] Step 2 — implement: `midi_track(ch)` callers in `midi_control.c` move to `midi_in_mask(ch)` (notes, CCs,
  expression, Program Change on a track channel); `seq.c` out paths use `trk_midi_ch` / `trk_midi_out`; `G_ROUTE` read
  once in `project_load` when the block is absent, then no longer read at run time; `KEEP` extended for NEW PROJECT.
  Keep anything the audio interrupt reaches pinned (the build guard tells you). GREEN; suite; target budgets checked;
  commit `feat(midi): per-track MIDI in, out and channel; old ROUTE read on load`.

---

### Task 2: SEQ STEPS (roll over strip; DRUM grid)

**Files:** create `firmware/src/ux_seq.c` (+ header if shared), `firmware/src/ux_section.c` (SEQ owned),
`firmware/src/params.c` / `ui_lens.c` (SEQ chain STEPS → STEP EXTRAS → PATTERN → MORE; Task 2 adds STEPS and the
chain with placeholders for the later pages hidden until their tasks), `firmware/src/ui_input.c` (step paging),
tests.

**Interfaces — Produces:**
```c
uint32_t uxq_step(void);                 /* the picked step 0..len-1, shared by every SEQ page and MORE */
void     uxq_strip(int y, uint32_t first);   /* the 16-cell strip at y for steps first..first+15 (note/accent/slide/rest, picked outlined, playing lit) */
void     uxq_roll(int y0, int h);        /* the roll of the current 16 */
void     uxq_grid(int y0, int h);        /* DRUM: 8 lanes x 16 steps */
```
Spec §2.1 (and UI spec §4.4).

- [ ] Step 1 — failing tests: SEQ opens on STEPS; footer `1/3 · NEXT STEP EXTRAS`; knobs STEP · NOTE · ACC/SLD · GATE
  turn the same values the inherited STEP page did (STEP n/LEN, NOTE, FLAG, TIME) for synth tracks; DRUM: STEP · LANE ·
  HIT · ACC as the inherited grid; status `STEP 5 · G2` / `SLIDE ON` / `GATE TIE`; 32 / 64 steps: SELECT on the last cell
  pages to the next 16 and the header shows `2/4`; the strip spells `5 · G2 · ACC`; renders S_SEQ_STEPS (synth 16, 64
  steps page 3, DRUM) lint-clean; the roll is paced (no redraw while nothing changes, ≤ one redraw per played step).
  RED.
- [ ] Step 2 — implement; delete the inherited STEP / NOTE drawing once no page uses it; GREEN; suite; commit
  `feat(ui): SEQ STEPS: the roll over the strip`.

---

### Task 3: STEP EXTRAS and PATTERN

**Files:** `firmware/src/ux_seq.c`, `params.c`, `ui_lens.c`, `ui_input.c`, tests.

**Interfaces — Consumes:** `uxq_step`, `uxq_strip` (Task 2).

Spec §2.2–2.3: STEP EXTRAS knobs RATCHET · CHANCE · CONDITION · MICRO (the inherited RATCH, CHANCE, COND, MICRO for the
picked step); picture: strip with chance as fill height, ratchets as ticks, the condition spelled out (`EVERY 2ND`,
`1ST OF 4`), micro as an offset mark. PATTERN knobs LENGTH · DIVISION · SWING · GATE (`P_SLEN P_SDIV P_SSWING
P_SGATE`); picture: length as cells, swing as late offbeats, gate as width.

- [ ] Step 1 — failing tests: each knob writes exactly the step / track value the inherited page wrote (compare
  against the inherited page's handler on the same state); the picked step is shared with STEPS; condition names are
  plain words and fit; renders S_SEQ_EXTRAS (ratchets, conditions, micro), S_SEQ_PATTERN (swing on/off, 32 steps)
  lint-clean. RED.
- [ ] Step 2 — implement; GREEN; suite; commit `feat(ui): SEQ STEP EXTRAS and PATTERN`.

---

### Task 4: SEQ MORE, lock cards, roll fixes, SCL drops the duplicates

**Files:** `firmware/src/ux_more.c` (SEQ rows), `params.c`, `ui_lens.c`, `ui_input.c` (`lock_turn`, `ent_knob`),
`ux_section.c` / `ux_seq.c` (lock card), SCL page tables (drop ROTATE and SEQ PITCH), `tests/ux_cover_test.c`, tests.

Spec §2.4–2.5: MORE groups TRACK, MUTATE, SHAPE (STRETCH REPEAT NUDGE ROTATE, applied as you turn), PLAYHEADS, STEP
(COMP options for the picked step, NOTE FOCUS, MOVE), AUTOMATION (PLAY, EVENTS, LOCKS, CLEAR action + dialog, the event
list with OCT+ add/toggle). Lock card in the ACID look (`LOCK · <SETTING> · <VALUE>`). Roll with a step held: KNOB 1
picks the MORE row, chord LEN on KNOB 4. ARP chord entry: KNOB 1–3 chord, KNOB 4 picks the row.

- [ ] Step 1 — failing tests: every setting / action the inherited SEQ pages reached is a SEQ page knob or a MORE row
  (coverage list extended); SHAPE rows apply as you turn like `shp_*`; AUTOMATION CLEAR opens the same dialog; the event
  list adds/toggles with OCT+; lock card render S_SEQ_LOCK; on the roll with a step held KNOB 1 picks the MORE row and
  KNOB 4 sets the chord LEN; during ARP chord entry KNOB 4 picks the row and a lock lands; held hit survives (the 3b
  rule); SCL no longer shows ROTATE or SEQ PITCH and SEQ does; renders of MORE scrolled to its end. RED.
- [ ] Step 2 — implement; delete the inherited COMP / CHANCE / AUTO / STRETCH / PLAYHEADS / MUTATE drawing and the
  inherited lock card; GREEN; suite; commit `feat(ui): SEQ MORE, the lock card, roll fixes`.

---

### Task 5: SEQ TOOLS and REC layers

**Files:** `firmware/src/ux_layer.c`, `ui_layer.c`, `ui_tools.c`, tests.

Spec §2.6: draw SEQ TOOLS and REC with `ux_layer.c` (key map, knob row, status line naming the layer); every key keeps
its job (list in the codebase map).

- [ ] Step 1 — failing tests: each key and knob of both layers does exactly what it did (existing layer tests green);
  renders S_LAYER_TOOLS (synth and DRUM), S_LAYER_REC lint-clean; per-layer view bits (`ui.uxlv`) cover the new layers;
  the layer overlay uses ux colours (pixel samples). RED.
- [ ] Step 2 — implement; delete the inherited drawing of both; GREEN; suite; commit
  `feat(ui): SEQ TOOLS and REC layers in the ACID look`.

---

### Task 6: GLO LIVE

**Files:** create `firmware/src/ux_glo.c`, `firmware/src/ux_section.c` (GLO owned), `params.c` / `ui_lens.c` (GLO chain
LIVE → MIX → MIDI → MORE; MIX / MIDI hidden until their tasks), `ui_live.c` (drawing removed; behaviour kept), tests.

Spec §3.1: keys as before (white tricks, black tracks, G#4 latch, C#5/D#5 TURN, F#5 RANDOM), OCT− tap RETURN / hold
BYPASS, OCT+ KEEP; knobs RATE · LOOP · THIN · DJ FILTER; picture: one-bar timeline with active tricks as blocks, picked
tracks lit; RIELES → RAILS rows (TRACKS, RATE, CUT, DENSITY) in GLO › MORE under `LIVE`. No Spanish names remain.

- [ ] Step 1 — failing tests: every LIVE key / knob / OCT gesture does what MANO did (state compared on the same input
  sequence); RAILS rows write what RIELES wrote; renders S_GLO_LIVE (idle, three tricks active, latched) lint-clean;
  the timeline paces (≤ 1 redraw per step while tricks run, 0 idle). RED.
- [ ] Step 2 — implement; GREEN; suite; commit `feat(ui): GLO LIVE`.

---

### Task 7: MIX and GLO MORE

**Files:** `firmware/src/ux_glo.c`, `ux_more.c`, `params.c`, `ui_lens.c`, `ux_home.c` (level bar in dB), tests.

Spec §3.2–3.3: MIX knobs LEVEL · PAN (selected track) · SWING · TUNE (`G_SWING`, `G_TUNE`); picture: six columns with
level bar + dB number + pan mark, selected lit; bar and number agree in dB on MIX and HOME TRACKS. GLO › MORE: LIVE
(RAILS), SYSTEM (TRACKS 4/6, LOAD, CPU readout), TEMPO (BPM, same rules as HOME), UNDO action, and every other setting
the inherited GLO pages reached (SENDS, REV, DJ FILTER are on FX: the coverage list says so).

- [ ] Step 1 — failing tests: knobs write the same params as the inherited MIXER / GLOBAL; the bar height follows the
  dB value (bar for −6 dB is the same on MIX and HOME); coverage of GLO; renders S_GLO_MIX, GLO MORE scrolled to end.
  RED.
- [ ] Step 2 — implement; delete the inherited MIXER / SENDS / GLOBAL / SYSTEM drawing (`graph_system`) once unused;
  GREEN; suite; commit `feat(ui): GLO MIX and MORE; level bars in dB`.

---

### Task 8: The MIDI page; tempo writers under external clock

**Files:** `firmware/src/ux_glo.c`, `ux_more.c`, `params.c`, `ui_lens.c`, `ui_input.c` (GLO+SELECT ~2512),
`ui_layer.c` (TAP ~338), `ux_widgets.c` (header sync mark), `menu_items.c` (MENU › MIDI → pointer row "MIDI IS ON
GLO"), tests.

**Interfaces — Consumes:** `trk_midi[]`, `midi_kbd_sel`, `trk_midi_defaults` (Task 1).

Spec §4.1–4.2: table T1–T6 `T3  IN ●  OUT ●  CH 3`, selected row lit, clock line `CLOCK INT` / `CLOCK USB ◆` (◆ on each
received beat while following); knobs IN · OUT · CHANNEL · CLOCK (`G_CLOCK`); MIDI MORE rows KEYBOARD PLAYS SELECTED,
CLOCK OUT (hint: USB only, TRS is input only), NOTES OUT, LEARN, LEARN CLEAR; CHANNEL 16 hint about Program Change;
header sync mark on every page when CLOCK is USB / TRS. Under USB / TRS clock, GLO+SELECT and TAP refuse with "CLOCK:
USB" / "CLOCK: TRS" like `param_knob`.

- [ ] Step 1 — failing tests: knobs set the selected track's IN / OUT / CHANNEL and `G_CLOCK`; ALGORITHM moves the lit
  row; MORE rows set `midi_kbd_sel` and the two `midi_out_mode` bits (round-trip through settings); MENU › MIDI shows
  the pointer row and LEARN still works from GLO; the header mark appears with CLOCK USB/TRS; GLO+SELECT and TAP write
  nothing under external clock and show the message, and still work with INT; renders S_GLO_MIDI (mixed settings,
  USB clock, channel 16 hint) lint-clean; glyphs ● ○ ◆ present in the fonts used (add if missing). RED.
- [ ] Step 2 — implement; GREEN; suite; commit `feat(ui): GLO MIDI page; every tempo writer follows the clock rule`.

---

### Task 9: GLO held layer

**Files:** `firmware/src/ux_layer.c`, `ui_layer.c`, tests.

Spec §3.4: mutes (black 1–6), DRUM-X group mutes (black 7–10), solos (F3–D4), E4 UNMUTE ALL, F4 TAP, G4 UNDO, A4 MUTATE
FREEZE, B4 REVERT ALL, K1–K4 levels (window as HOME), PLAY RESTART (message under external clock) drawn by
`ux_layer.c`; also correct the stale header comment at `ui_layer.c:9-13`.

- [ ] Step 1 — failing tests: every key and knob as before; renders S_LAYER_GLO (mutes, a solo held, external clock)
  lint-clean. RED.
- [ ] Step 2 — implement; delete the inherited drawing; GREEN; suite; commit `feat(ui): GLO layer in the ACID look`.

---

### Task 10: Backlog items and the "built on" wording

**Files:** `ux_widgets.c` (automation / lock mark on the knob row), `ux_section.c` (paced redraw on automation and
locks), `ux_layer.c` (MACRO status "SELECT DICES LFO"), `ui_input.c` / `ux_section.c` (engine change on EDIT page 3
stays on page 3), `ux_theme.c` (glow ring banding, only if a 3c page uses a big glow), `ui_menu.c` (ABOUT "Built on
JIANT FM and Felucca"), web pages listed in the spec §5, tests.

Spec §5 and §6.

- [ ] Step 1 — failing tests: a knob whose setting has automation or a lock on the current step shows the mark
  (pixel sample) and one without does not; automation playing on a section page redraws the picture at most at the
  pace interval (count redraws over a played bar); MACRO status text; engine change on EDIT page 3 stays on page 3;
  ABOUT text contains "Built on JIANT FM and Felucca" and no "fork of"; a web check (extend `web/test_web.mjs` or a
  grep test in `tests/`) that the installer, 404, manual and editor visible text no longer says "fork of JIANT" or
  "JIANT FM under a new name" while LICENSING.md and copyright lines are unchanged. RED.
- [ ] Step 2 — implement; GREEN; suite; commit `fix(ui): 3c-1 backlog; ACID FM built on JIANT FM and Felucca`.

---

### Task 11: Version 0.5, docs, device

**Files:** `firmware/src/felucca.c` (`v0.5`), README Status, `web/landing/index.html`, `web/manual/data.js` (0.5 news
entry ES + EN; SEQ / GLO / MIDI manual entries corrected briefly — full rewrite is 3d), regenerate what tests pin
(manual shots, nav maps, cheat sheets where the generator runs), `docs/acid-fm/device-log.md`,
`docs/superpowers/plans/2026-10-26-acid-fm-piece3c1-deferred.txt` (everything flagged for later in the task reports).

- [ ] Step 1 (implementer) — version + docs; full suite; commit `release prep: ACID FM 0.5 (SEQ, GLO, MIDI page)`;
  tree clean.
- [ ] Step 2 (controller + user) — fresh backup (`sudo .venv/bin/python fm1_unbrick.py backup DIR` in
  `~/MvaveFM1Unbricker` after `softkey --port "ACID FM" --yes`); `./build.sh --release 0.5` on the clean HEAD; install
  (`tools/fm1_install.py build/acid-fm-0.5.fwsc --port "ACID FM" --yes`); serial `status`; no-manual check ("make step 5
  a ratchet", "make T5's notes play only every other bar", "send T2 to MIDI channel 10", "follow the clock from USB",
  "loop the last bar live and let it go"); CPU TEST late blocks/s on SEQ STEPS, LIVE with tricks, MIDI; a DAW or second
  device on USB sending clock and notes; log; merge ff to main and push on the user's go-ahead.
