# ACID FM 3c-2: SAVE, MENU, ABOUT and the dialogs

Date: 2026-10-27. Status: approved section by section in brainstorming (with visual mockups); awaiting spec review.
Parent specs: `docs/superpowers/specs/2026-10-24-acid-fm-ui-design.md` (§0 intuitive first, §3 look, §4.1 the seven
rules, §5 implementation, §6 tests), `docs/superpowers/specs/2026-10-25-acid-fm-ui-3b-sound-sections.md` (the
section / MORE engine) and `docs/superpowers/specs/2026-10-26-acid-fm-ui-3c1-seq-glo-midi.md` (the same engine applied
to SEQ and GLO, the held layers, the MIDI page). Backlog: `docs/superpowers/plans/2026-10-26-acid-fm-piece3c1-deferred.txt`
([3c-2] items). Release: ACID FM **0.6**. Mockups: `.superpowers/brainstorm/*/content/save-song.html`, `dialogs.html`.

User decisions (2026-10-27):

- SAVE = **SOUND → SONG → PROJECT → MORE**.
- SONG layout **A**: the four sections on top, the song's rows as a strip of chips below.
- MENU = **AUDIO → CLICK → CONTROLS → MORE**; ABOUT is a MORE row (an action that opens it).
- Dialogs: **full screen** (layout A), one design for every confirmation.
- Extras in 0.6: the web editor's per-track MIDI; English example names in the manual's screens; every tempo writer
  follows the external clock; a track-wide **PLAYS** ("every other bar") setting.

## 1. Scope

1. SAVE rebuilt on the section engine (`ux_section.c`, `ux_more.c`, `ux_nav.c` tables) (§2); the SAVE held layer
   in the ACID look (§2.5).
2. MENU rebuilt on the same engine; ABOUT restyled (§3).
3. One full-screen dialog for all 22 confirmations (`CF_*`) (§4).
4. PLAYS, a track-wide loop condition (§5.1).
5. The clock rule for every tempo writer (§5.2).
6. The web editor's per-track MIDI (§5.3).
7. English example names in the manual's screens; the [3c-2] leftovers listed in §5.4.

The inherited SAVE, MENU, ABOUT and dialog drawing is deleted once replaced; shared helpers stay while used.
Setting ids, menu item ids (`MI_*`), the editor protocol's existing messages, MIDI CCs and the project / preset /
settings storage formats do not change, except the one appended field of §5.1. Sound goldens unchanged.

## 2. SAVE = SOUND → SONG → PROJECT → MORE

Same rules as every section (§4.1 of the UI spec): SAVE opens on the page last used and steps on when pressed again;
SELECT walks the pages and scrolls MORE; PRESETS loads the selected track's presets (never changes page); ALGORITHM
picks the track; OCT+ / OCT− act (yes / no) on SAVE's pages as they do today. Footer `SAVE · <PAGE>   n / 4 · NEXT <PAGE>`.
Every action that overwrites or loses something asks first (§4).

### 2.1 SOUND (the inherited USER and PRESETS pages)

- Knobs: **SLOT** (the 16 user slots), **PRESET** (the factory presets of the track's engine), **NAME** (opens the
  inherited NAME editor), **SAVE** (an action knob: a turn arms it, as the inherited SAVE knob does).
- Picture: the user slots as a list, each with its engine colour and name (empty slots dim), the selected slot lit;
  the track's current sound named on top.
- OCT+ saves the track's sound into SLOT (over a used slot: the overwrite dialog, `CF_OVR_USER`); loading a slot or
  a preset is what PRESETS / the inherited load path already do. ERASE (`CF_ERASE_USER`) is a MORE row.

### 2.2 SONG (the inherited SECTIONS and ARRANGE pages; layout A)

- Knobs: **SECTION** (A–D), **ROW** (the song's rows, n / N), **BARS** (the row's length), **SCENE** (the row's
  scene, or `--`).
- Picture: the four sections as four blocks (letter, name, EMPTY dim, the one playing marked, the picked one lit);
  under them `ARRANGE` and the song's rows as chips (section letter + bars, the picked row lit, the playing row
  marked); the song's length and end (`72 BARS · LOOP`) on a line under the chips.
- Status line: what the next press does (`ROW 3 · B · 16 BARS · OCT+ ADDS A ROW`).
- OCT+ adds a row after the picked one (as ARRANGE does today); deleting a row is a MORE row (`CF_DEL_ROW`).
  Storing and playing sections stay the SAVE layer's keys (§2.5) and SAVE + OCT± (kept).

### 2.3 PROJECT (the inherited PROJECTS page)

- Knobs: **PROJECT** (1–4), **LOAD** (action), **SAVE** (action), **NAME**.
- Picture: four cards, one a project: name, bars, saved / unsaved (a dot when the project in memory has changes
  not stored), the loaded one marked, the picked one lit.
- Loading over unsaved changes and saving over another project ask first (`CF_OVR_PROJ` and the load dialog).

### 2.4 MORE

Groups (MORE rows, KNOB 1 picks, KNOB 2 sets / OCT+ runs an action):

- **SONG**: END (STOP / x2–x8 / LOOP), DELETE ROW, CLEAR SONG, the inherited section settings that had no page.
- **TOOLS**: CLEAR SEQ, INIT SOUND, ERASE USER SLOT, NEW PROJECT, CPU TEST — each an action row that opens its dialog.
- **SOUND**: anything else the inherited USER / PRESETS pages reached.

Coverage: every setting and action the inherited SAVE pages reached is on a page knob or a MORE row (tested).

### 2.5 SAVE held layer

Drawn by `ux_layer.c` like the 3b / 3c-1 layers (key map, knob row, status line naming the layer). Every key keeps
its job: F3–B3 play sections A–D, C4–F4 store A–D, C5 NAME, D5 LOOP / SONG, E5 SONG REC, G5 ARRANGE, and the
inherited knobs.

## 3. MENU (HOME held) = AUDIO → CLICK → CONTROLS → MORE

MENU becomes a section on the same engine: SELECT walks its pages and scrolls MORE, KNOB 1–4 set the shown settings,
HOME closes it. There are no tabs any more, so ALGORITHM no longer jumps between them (it does nothing in MENU).
Settings keep their `MI_*` ids, ranges and storage.

- **AUDIO**: LOW CUT, TRACKS, LOAD, BPM LOCK.
- **CLICK**: CLICK, CLICK LEVEL, COUNT-IN, (empty).
- **CONTROLS**: HOLD TIME, KNOB SPEEDUP, FX LATCH, PANEL.
- **MORE**: SCREEN (LARGE, ANIMATION, SCREEN SLEEP), LEDS (LED GLOW, SCALE LEDS), USB (SERIAL, USB AUDIO, USB LEVEL),
  SYSTEM (CPU CORES, CPU CLOCK, RESTORE LAST, CALIBRATION, INIT), SPEAKER EQ, a pointer row **MIDI IS ON GLO** (opens
  GLO › MIDI), and **ABOUT** (an action row that opens it).
- Settings that act at power-on (CPU CORES, CPU CLOCK, RESTORE LAST) say so in their hint, as today.

Coverage: every menu item (`MI_*`) is on a page or a MORE row (tested).

### 3.1 ABOUT

- First screen: the ACID FM wordmark, the version, "Built on JIANT FM and Felucca", the QR code to the manual, the
  audio load (NOW, PEAK, LATE, SHED since power-on) and the CPU TEST result line.
- SELECT scrolls the credits (every name and licence line kept); OCT+ opens PROFILE (restyled, same figures);
  OCT− back.

## 4. Dialogs (layout A, full screen)

One widget (`ux_dialog`) draws every confirmation (`CF_*`: CLEAR ALL / MOTION / PFX / SEQ / SONG / TRK, CPU TEST,
DEL ROW, DICE ALL / FX / LFO / SEQ / SOUND, ERASE USER, INIT SOUND / SYS, LIVE COMMIT, LOAD PAT, MUT BACK,
OVR PROJ, OVR USER, RECALL SOUND).

- Header kept (track, transport). The page gives way to:
  - a small **action word** (CLEAR, LOAD, SAVE OVER, DICE, KEEP …) in the accent colour;
  - the **question** in the large face (`CLEAR T3'S SEQUENCE?`), the object named;
  - **one plain line on what is lost and how to get it back** (`Loses 14 notes and 3 locks. GLO + G4 brings it
    back.`; where nothing is lost, what happens);
  - the keys: `OCT−` **NO** on the left, `OCT+` **<VERB>** on the right (CLEAR, LOAD, SAVE OVER, KEEP, RUN).
- Colour: red (the existing danger token) only when something is lost; otherwise the accent.
- Footer `<SECTION> · <ACTION>   OCT+ YES · OCT− NO`.
- The dice dialogs keep their BEFORE / NOW comparison in the page area and use the same action word, footer and keys.
- Behaviour unchanged: the same keys answer, the same actions run, UNDO where it was.
- Counts in the "what is lost" line come from the data (notes, locks, rows, bars); where a count is not cheap to
  know, the line names the thing without a number.

## 5. Extras

### 5.1 PLAYS: a track-wide loop condition

- A SEQ › MORE row under TRACK: **PLAYS** = ALWAYS, then the same conditions STEP EXTRAS' CONDITION uses (1ST OF 2,
  EVERY 2ND, 1ST OF 3 … 1ST OF 8, 1ST ONLY, NOT 1ST), counted in loops of the track's pattern (`t->loops`).
- On a loop PLAYS excludes, the track's steps do not sound (notes and hits; its components, locks and automation
  as when muted). A step's own CONDITION still applies on the loops PLAYS lets through.
- Stored per track with the project: one new field, **appended** after the existing per-track data; a project saved
  before 0.6 loads with PLAYS = ALWAYS. Undoable like any track setting; the sequence dice leave it alone.
- The status line spells it (`T5 PLAYS · 1ST OF 2`).

### 5.2 The clock rule for every tempo writer

While CLOCK is USB or TRS:

- GEN does not write the BPM (the piece is made at the incoming tempo); its screen says `CLOCK: USB` where it shows
  the tempo.
- The power-on demo does not write the BPM.
- The web editor's BPM write (PARAM_SET on the BPM global) is refused with the same message path as the device's
  (`CLOCK: USB` / `CLOCK: TRS`).
- **CPU TEST** sets CLOCK to INT for the test (it always plays its 160 BPM song) and restores the previous CLOCK
  when the test is left (STOP + UNDO, or the test song replaced); the dialog says so.

### 5.3 The web editor's per-track MIDI

`web/editor.html` gets a MIDI page: per track IN, OUT, CHANNEL (song globals 71–88) and KEYBOARD PLAYS SELECTED
(89), written with the existing PARAM_SET messages; ROUTE (14) stays hidden. `web/EDITOR_PROTOCOL.md` lists them.

### 5.4 Names and leftovers

- English example names in the render scenes behind the manual's screens (`tests/ui_render.c`: NOCHE LARGA … →
  English); a check that no device string or default name is Spanish (the ES lyrics bank is a feature and stays).
- From the 3c-1 deferred list: the GLO layer's `NO RESTART` only while playing (fixed in 0.5, re-check); the lock /
  automation mark on MIX levels and MORE rows; the stale comments listed there.

## 6. Storage, CPU and size

- Storage append-only: only PLAYS (§5.1) is new. Projects, user presets and settings from 0.5 load and behave as
  before; 0.5 reading a 0.6 project ignores PLAYS.
- No audio-interrupt work changes except the PLAYS gate in the sequencer (a compare per step) and the clock rule;
  variables an interrupt reaches stay pinned (`tools/build.py` guard); budgets (ISR 300, TIMER5 2170, slicer 821)
  unchanged.
- Image: 29.8 KB free at 0.5.1 (floor 16 KiB); the deleted SAVE / MENU / dialog drawing pays for the new pages.

## 7. Testing

- Coverage: every setting and action the inherited SAVE pages and MENU tabs reached is on a page knob or a MORE row;
  every `CF_*` dialog draws through `ux_dialog`.
- Behaviour: each dialog answers with the same keys and runs the same action as before (state compared on the same
  input); SOUND / SONG / PROJECT knobs write what the inherited pages wrote; the SAVE layer's keys unchanged.
- PLAYS: in the sequencer, a track at 1ST OF 2 sounds on loop 0, not on loop 1, again on loop 2; a step CONDITION on
  top; old projects load as ALWAYS; save / load round-trip byte for byte.
- Clock rule: GEN, the demo and the editor's BPM write leave the BPM alone under USB / TRS and work under INT; CPU
  TEST runs at INT and restores CLOCK.
- Renders: every new page (SAVE SOUND / SONG / PROJECT / MORE scrolled, MENU AUDIO / CLICK / CONTROLS / MORE scrolled,
  ABOUT, PROFILE) and every dialog kind lint-clean; the manual's shots regenerated.
- Editor: the MIDI page's writes match the device's ids (web tests).
- Device (0.6): backup, install, status; no-manual tasks ("save T3's sound to a user slot", "make a song A-B-B-C",
  "start a new project and get the old one back", "make T5 play every other bar", "turn the click on with a
  count-in"); CPU TEST; log; merge and push on the user's go-ahead.

## 8. Order of work (for the plan)

1. `ux_dialog` and every `CF_*` dialog.
2. SAVE SOUND.
3. SAVE SONG.
4. SAVE PROJECT, SAVE MORE, the SAVE layer.
5. MENU (AUDIO, CLICK, CONTROLS, MORE) and ABOUT / PROFILE.
6. PLAYS.
7. The clock rule.
8. The editor's per-track MIDI.
9. English example names; leftovers.
10. 0.6: version, docs, device.

## 9. Out of scope

The BREAK sampler (piece 4), TAPE and the acid voice (piece 5), the generator rewrite (piece 6). GEN's pages stay as
they are (piece 6 rebuilds them). The MIDI minors on the [later] list stay there.
