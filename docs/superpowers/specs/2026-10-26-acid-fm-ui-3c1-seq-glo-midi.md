# ACID FM 3c-1: SEQ, GLO and the MIDI page

Date: 2026-10-26. Status: approved section by section in brainstorming; awaiting spec review. Parent specs:
`docs/superpowers/specs/2026-10-24-acid-fm-ui-design.md` (the interface: §0 intuitive first, §3 look, §4.1 the seven
rules, §5 implementation, §6 tests), `docs/superpowers/specs/2026-10-25-acid-fm-ui-3b-sound-sections.md` (the
section/MORE engine this piece reuses) and `docs/superpowers/specs/2026-10-24-acid-fm-design.md`. Inventory of the
inherited SEQ / GLO / MIDI code: taken 2026-10-26 from `ui_lens.c` CHAINS, `params.c` PAGES, `ui_draw.c` knob labels,
`ui_live.c`, `ui_tools.c`, `ui_layer.c`, `menu_items.c`, `midi_control.c`, `midi_clock.c`, `seq.c` (identifiers
below). Release: ACID FM **0.5**.

User decisions (2026-10-26): piece 3c ships in two releases, **0.5** = SEQ + GLO with a new MIDI page (this spec),
**0.6** = SAVE, MENU, ABOUT's layout and the dialogs (own spec after 0.5 is on the device); SEQ's third page is STEP
EXTRAS; GLO keeps the LIVE performance page, first; the MIDI page lives in GLO; the "any channel plays the selected
track" mode is kept as a MIDI MORE row; ACID FM is presented as its own firmware "built on" JIANT FM and Felucca
(credits only, licence notices kept).

## 1. Scope

1. SEQ rebuilt on the 3b section engine (`ux_section.c`, `ux_more.c`, `ux_nav.c` tables): STEPS, STEP EXTRAS,
   PATTERN, MORE (§2). The step lock cards and the SEQ TOOLS and REC held layers in the ACID look.
2. GLO rebuilt: LIVE, MIX, MIDI, MORE (§3). The GLO held layer in the ACID look.
3. The MIDI page: per-track MIDI IN / OUT / CHANNEL, clock source and clock out made visible (§4). New stored
   settings are **appended** (old projects and settings load and behave as before).
4. The "built on" wording on the website and the device's ABOUT text (§5).
5. The [3c] backlog items that belong to SEQ / GLO (§6).

The inherited page code for SEQ and GLO (the lens pages in `ui_lzpage.c`, the SEQ/GLO parts of `ui_draw.c`,
`ui_live.c`'s drawing) is deleted once its pages are replaced; shared helpers stay while used. Setting ids, the
editor protocol and MIDI CCs do not change. Goldens (sound) unchanged.

## 2. SEQ = STEPS → STEP EXTRAS → PATTERN → MORE

Knob colours K1 ACID, K2 SLIDE, K3 VIOLET, K4 ACCENT. The step picked on any SEQ page is the same step on the others.

### 2.1 STEPS (the UI spec §4.4)

- Top ≈ 96 px: a roll of the current 16 steps (pitch up, time across): notes ACID, accents ACCENT, slides as joined
  bars in SLIDE, the edited step outlined white, the playing step a light column.
- Below: a strip of 16 cells, 13 px wide, coloured note / accent / slide / rest; the edited step spelled out
  (`5 · G2 · ACC`).
- Knobs: **STEP · NOTE · ACC/SLD · GATE** (from the inherited STEP page: STEP, NOTE, FLAG, TIME). GATE shows
  NOTE / TIE / REST / a length, as before.
- 32 / 64 steps: SELECT on the strip's last cell pages to the next 16, shown as `1/4` in the header.
- DRUM tracks: the roll is the lane grid (eight lanes × 16 steps, hits in the lane's colour, accents brighter);
  knobs **STEP · LANE · HIT · ACC** (the inherited DRUM grid).
- Status line on a turn: the value in words (`STEP 5 · G2`, `SLIDE ON`, `GATE TIE`).

### 2.2 STEP EXTRAS

For the step picked on the strip (the strip is drawn on this page too):

- Knobs: **RATCHET · CHANCE · CONDITION · MICRO** (from the inherited CHANCE, COMP and NOTE pages: RATCH, CHANCE,
  COND, MICRO).
- Picture: the strip with each cell's chance as its fill height and ratchets as ticks inside the cell; the picked
  step's condition spelled out (`EVERY 2ND`, `1ST OF 4`, …) and its micro timing as a small offset mark.
- The other step options of the inherited COMP pages (TYPE, MODE, RANGE, SHIFT, BEND, EVERY, TO STEP, RATE, LAP) are
  MORE rows (§2.4), applying to the picked step.

### 2.3 PATTERN

- Knobs: **LENGTH · DIVISION · SWING · GATE** (`P_SLEN`, `P_SDIV`, `P_SSWING`, `P_SGATE`).
- Picture: the pattern's length as cells, swing as the offbeat cells shifted late, the gate as the cells' width.

### 2.4 SEQ › MORE

Rows, in groups, plain names:

- TRACK: PITCH `P_POFS`, CHANCE `P_TCHAN`, HUMAN `P_HVEL`, REC QUANTISE `P_HRQ`.
- MUTATE: SOUND `P_MSND`, PATTERN `P_MPAT`, AMOUNT `P_MAMT`, MODE `P_MMODE` (ORBIT / DRIFT).
- SHAPE: STRETCH, REPEAT, NUDGE, ROTATE (applied as you turn, as before).
- PLAYHEADS: HEAD, RATE, DIRECTION, INTERVAL.
- STEP (the picked step): the COMP options listed in §2.2, NOTE FOCUS, MOVE.
- AUTOMATION: PLAY, EVENTS (count), LOCKS (count), CLEAR (action, dialog), and the event list (ROW, STEP, SETTING,
  VALUE; OCT+ adds or toggles, as before).

ROTATE and SEQ PITCH were on SCL as well (SCL › CHORD ROTATE, SCL › MORE SEQ PITCH): they now live in SEQ only and
SCL drops them (one place per setting).

### 2.5 Step locks and the roll fixes

- Holding a step and turning a knob shows the lock card in the ACID look (`LOCK · <SETTING> · <VALUE>` in the knob
  row and the status line), replacing the inherited card.
- Roll with a step held: KNOB 1 picks the MORE row to lock (as on the DRUM grid); the chord LEN moves to KNOB 4
  while a step is held.
- ARP chord entry on the roll: the MORE row can still be picked and locked (`ent_knob` keeps KNOB 1–3 for the chord,
  KNOB 4 picks the row).

### 2.6 Held layers

SEQ TOOLS (F3 CLEAR, G3 REVERSE, A3/B3 SHIFT, C4 RANDOM, D4 COOK, E4 BEAT, F4 CLEAR AUTO, G4–C5 lane tools, D5
REVERT, G5 CLEAR STEP, black keys lanes, K1–K4 PATTERN, REC = CLEAR ALL) and REC (F3 CLEAR, G3 CLICK, K1 CLICK) are
drawn by `ux_layer.c` like the 3b layers: key map, knob row, status line naming the layer. Every key keeps its job.

## 3. GLO = LIVE → MIX → MIDI → MORE

### 3.1 LIVE (the inherited MANO page, renamed)

- The white keys are the tricks: LOOP 8 / 4 / 2 / 1, ½×, 2×, REVERSE, PING-PONG, RATCHET, THIN, OCT±, RETURN; the black
  keys pick the tracks they act on; G#4 latches; C#5 / D#5 TURN; F#5 RANDOM (as before).
- OCT− tapped RETURN, OCT− held BYPASS, OCT+ KEEP (the inherited COMMIT).
- Knobs: **RATE · LOOP · THIN · DJ FILTER**.
- Picture: one bar as a timeline, the active tricks as coloured blocks over it (loop length, speed, reverse arrows),
  the picked tracks lit along the left.
- LIVE › MORE (LIVE is a page, so its extra settings are GLO › MORE rows under `LIVE`): the inherited RIELES page as
  RAILS: TRACKS (ALL, T1–T6, SYNTHS, DRUMS), RATE, CUT, DENSITY.

### 3.2 MIX

- Knobs: **LEVEL · PAN · SWING · TUNE** (`P_LEVEL`, `P_PAN` for the selected track; `G_SWING`, `G_TUNE` for the song).
- Picture: six columns, T1–T6, each a level bar with its dB number and a pan mark; the selected track lit.
- The level bar and the dB number agree (the bar follows the mixer's dB, the 3a backlog item; HOME TRACKS gets the
  same rule).
- The inherited SENDS page is not repeated (FX › SENDS has it); the inherited MIXER's REV and DJ FILTER are on FX.
- BPM lives on HOME (KNOB 4) and GLO+SELECT. CLOCK lives on MIDI (§4).

### 3.3 GLO › MORE

LIVE: the RAILS rows (§3.1). SYSTEM: TRACKS 4/6 `G_TRKS`, LOAD `G_LOADP`, CPU (readout). TEMPO: BPM `G_BPM` (same
rules as HOME). UNDO (action). Any other setting the inherited GLO pages reached (coverage test).

### 3.4 GLO held layer

Mutes (black 1–6), DRUM-X group mutes (black 7–10), solos (F3–D4), E4 UNMUTE ALL, F4 TAP, G4 UNDO, A4 MUTATE FREEZE, B4
REVERT ALL, K1–K4 levels, PLAY RESTART: drawn by `ux_layer.c`. Every key keeps its job.

## 4. The MIDI page (GLO page 3)

### 4.1 Page

- Picture: a table, one row per track T1–T6: `T3  IN ●  OUT ●  CH 3`, the selected track's row lit; above it the
  clock: `CLOCK INT` or `CLOCK USB ◆` (◆ blinks on each received beat while following).
- Knobs: **IN · OUT · CHANNEL · CLOCK** (IN on/off, OUT on/off, CHANNEL 1–16 for the selected track; CLOCK INT / USB /
  TRS = `G_CLOCK`). ALGORITHM picks the track, as everywhere.
- MIDI › MORE rows (GLO › MORE, group `MIDI`): **KEYBOARD PLAYS SELECTED** on/off; **CLOCK OUT** on/off (USB only: the
  FM-1's TRS socket is input only, said in the row's hint); **NOTES OUT** on/off (master for the per-track OUT);
  LEARN on/off, LEARN CLEAR (moved from MENU › MIDI, which keeps a pointer row "MIDI IS ON GLO").
- The header shows a small sync mark whenever CLOCK is USB or TRS, on every page.

### 4.2 Behaviour

- Incoming notes / CCs on channel c go to every track whose IN is on and whose CHANNEL is c. With **KEYBOARD PLAYS
  SELECTED** on, notes on any channel not claimed by a track go to the selected track (the inherited SEL mode,
  extended).
- Outgoing: a track's notes are sent on its CHANNEL when its OUT and NOTES OUT are on; clock and start/stop are sent
  when CLOCK OUT is on and CLOCK is INT (as `mo_clock_on` today).
- Channel 16 Program Change keeps selecting a section / project; Program Change on another channel loads a sound on
  the tracks listening on that channel. A track set to channel 16 receives notes and CCs on 16; Program Change on 16 always
  means section / project (the MIDI page's hint says so when CHANNEL is 16).
- Under USB / TRS clock every way of writing the tempo (HOME KNOB 4, GLO › MORE BPM, GLO+SELECT, TAP) changes nothing
  and says "CLOCK: USB" / "CLOCK: TRS" (closes the 3b backlog item).

### 4.3 Storage (append-only)

- Per track IN, OUT, CHANNEL: new song globals appended after the existing ones (saved with the project, like
  `G_CLOCK` and `G_ROUTE`; NEW PROJECT keeps them, like those). A project saved before 0.5 loads with T1–T6 on
  channels 1–6, IN and OUT on, so it behaves as before.
- `G_ROUTE` is read on load: CH1-6 → the defaults above; SEL → KEYBOARD PLAYS SELECTED on. It is no longer shown.
- CLOCK OUT and NOTES OUT stay in the device setting `midi_out_mode` (KEYS / CLOCK / ALL / OFF = the two bits); the
  MENU row is replaced by the two MORE rows.
- KEYBOARD PLAYS SELECTED: a new song global, appended.

## 5. "Built on" wording

- Device ABOUT text (`ui_menu.c`): "A fork of JIANT FM" → "Built on JIANT FM and Felucca"; the credits list keeps every
  author and licence. ABOUT's layout is 0.6.
- Website pages still naming JIANT in visible text (installer `index_pkg.html`, `404.html`, the manual's `data.js`
  and `index.html`, `editor.html`, `fm1backup.js` / `fm1diag.js` / `fm1ota.js` messages): ACID FM in headings and
  messages, "built on JIANT FM and Felucca" where provenance matters. Code comments and LICENSING.md are unchanged.
- The 0.5 news entry in English, plain words, no band names.
- The manual is English only and does not present ACID FM through JIANT: Spanish strings, the language switch and
  `MANUAL.es.md` go; JIANT appears only in one credits line (user, 2026-10-26).

## 6. Other 3c-1 items

- Automation mark on the knob row: a knob whose setting has recorded automation or a lock on the current step shows
  a small mark (3b backlog).
- Automation / step locks redraw a section's picture through the pace rules (no unpaced redraws; 3b backlog).
- Held MACRO's "SELECT DICE" reads "SELECT DICES LFO" (3b backlog).
- Changing the engine on EDIT page 3 stays on page 3 (3b backlog).
- The glow ring banding smoothed if the new pages use big glows (3a backlog).
- Coverage: every setting the inherited SEQ and GLO pages and layers reached is on a page or a MORE row (tested).
- RAM: no new `.pool` data; tables in `.rodata` / `.bss`; DC cell budget per page ≤ 1920; image stays under the slot
  with ≥ 16 KiB free for pieces 4–6 (32,244 B free at 0.4).

## 7. Testing

- Renders: every SEQ and GLO page (synth and DRUM tracks, 16 / 32 / 64 steps, STEP EXTRAS with ratchets and
  conditions, LIVE with tricks active, MIX, MIDI with mixed IN/OUT/channels and USB clock), every MORE list scrolled to
  its end, each held layer, the lock card; lint and alignment clean.
- Coverage test extended to SEQ and GLO (§6).
- Navigation: the seven rules on SEQ and GLO; step selection shared across SEQ pages; `1/4` paging.
- MIDI (host): routing per track and channel, KEYBOARD PLAYS SELECTED, OUT per track and NOTES OUT, CLOCK OUT only with
  INT, Program Change on 16 and on a track's channel; a pre-0.5 project with `G_ROUTE` CH1-6 and SEL loads to the
  matching settings; settings and projects round-trip.
- Tempo under external clock: all four writers refused with the status line.
- Device (0.5): no-manual check: "make step 5 a ratchet", "make T5's notes play only every other bar", "send T2 to
  MIDI channel 10", "follow the clock from USB", "loop the last bar live and let it go"; CPU TEST late blocks/s on
  SEQ STEPS, LIVE with tricks, MIDI; a DAW or second device on USB sending clock and notes.

## 8. Order

1. MIDI storage and routing (no UI); 2. SEQ STEPS; 3. STEP EXTRAS + PATTERN; 4. SEQ MORE, lock cards, roll fixes; 5.
SEQ TOOLS and REC layers; 6. GLO LIVE; 7. MIX + GLO MORE; 8. MIDI page; 9. GLO layer; 10. backlog (§6) and the
"built on" wording; 11. version 0.5, docs, device. The plan may merge or split steps.
