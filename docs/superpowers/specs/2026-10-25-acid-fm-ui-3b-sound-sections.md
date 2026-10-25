# ACID FM 3b — the sound sections, genres, power-on demo

Date: 2026-10-25. Status: approved section by section in brainstorming; awaiting spec review. Parent specs:
`docs/superpowers/specs/2026-10-24-acid-fm-ui-design.md` (the interface: §0 intuitive first, §3 look, §4.1 the seven
rules, §5 implementation, §6 tests) and `docs/superpowers/specs/2026-10-24-acid-fm-design.md`. Inventory of JIANT's
sections: taken 2026-10-25 from `ui_lens.c` CHAINS, `params.c` PAGES, the engines' `.edit[]` tables, `ui_layer.c`,
`gen_tables.c`, `engines.c` (identifiers below). Release: ACID FM **0.4**.

## 1. Scope

1. EDIT, ENV, LFO, FX, ARP, SCL rebuilt on the 3a widgets (`ux_header`, `ux_knobs`, `ux_panel_*`, `ux_status`,
   `ux_footer`) and rules (§4.1); JIANT's page code for them and the EDIT creatures (`ui_engvis.c` `vx_*` pictures,
   `graph_being`, `graph_colony`) deleted; shared helpers kept while used.
2. Those sections' held layers (EDIT, LFO MACRO, FX performance, SCL) restyled; behaviour unchanged except §2.6.
3. Generator: ten genres shown (§3).
4. Power-on demo (§4).
5. TRACKS calmer under load; the [3b] backlog items (§5).

Setting ids, stored formats, the editor protocol and MIDI CCs do not change. Goldens (sound) unchanged.

## 2. Page tables

Knob colours: K1 ACID, K2 SLIDE, K3 VIOLET, K4 ACCENT (3a). Labels in plain words (§0 rule 4); the MORE page lists
everything else in the section, one setting per row (SELECT scrolls, KNOB 1 picks a row, KNOB 2 sets it). Every
setting JIANT exposed in these sections is on a page or a MORE row (tested, §6).

### 2.1 EDIT

| Engine | Page 1 · SOUND | Page 2 · TONE | Page 3 | MORE |
|---|---|---|---|---|
| ANALOG | WAVE · DETUNE · MIX · NOISE (`P_E0..E3`) | CUTOFF · RESO · DRIVE · INTERVAL (`P_E4..E7`) | VOICE: MODE `P_VOICE` · GLIDE `P_GLIDE` · PRIORITY `P_PRIO` · DETUNE `P_DETUNE` | FILTER TYPE `P_FTYPE`, MORPH `P_MORPH`, DRIFT `P_DRIFT`, GLIDE MODE `P_GLMODE`, ALLOC `P_ALLOC` |
| PHASE, LOFI, NOISE, VOICE | the engine's E1–E4 (`.edit[0..3]`, mode-dependent relabels kept) | E5–E8 | FILTER: TYPE `P_FTYPE` · CUTOFF `P_FCUT` · RESO `P_FRES` · DRIVE `P_DIST` | VOICE MODE, GLIDE, GLIDE MODE, PRIORITY, ALLOC, DETUNE, MORPH, DRIFT (where the engine shows them), RETRIG (LOFI BYTE), VOICE: LANGUAGE `P_VLANG`, LYRIC `P_VLYR`, POSITION, and the action TYPE LYRIC (opens the lyric editor) |
| FM6 | ALGORITHM · FEEDBACK · MOD · RATIO (E1–E4) | OPERATOR: OP · RATIO · FINE · LEVEL | OP ENV: OP · STAGE · RATE · LEVEL | MEG, VMOD, DETUNE, SLOT (E5–E8); for the picked OP: MODE (RATIO/FIXED), DETUNE, VELOCITY; FILTER TYPE/CUTOFF/RESO/DRIVE; VOICE MODE, GLIDE, PRIORITY, ALLOC, DETUNE |
| DRUM-X | KIT: MORPH · TUNE · TONE · DECAY (E1–E4) | SOUND: LANE · LEVEL `P_LN*` · PITCH · DECAY | SOUND 2: LANE · NOISE · COLOR · MODE | KIT 2 (NOISE, FM ENV, FOLD, DRIVE), KIT 3 (BP, FEEDBACK, HANDS), per LANE: SIDE, PITCH MOD, PM TYPE, DRIVE, LAYER, FEEDBACK, SEND; X-MOD (RATE, DEPTH, SHAPE, RAND); FILTER (TYPE, CUT, RES, DRV); STEREO IMAGE `P_DXIMG`; VOICE |

- The OP / LANE picked on one page is the same OP / LANE on the others and in MORE (one selection per track).
- FM6 operator values remain patch bytes (not automatable), as in JIANT.
- **FOCUS is dropped**: it was a view into the LFO MOD matrix for one setting; the matrix keeps every function, so no
  setting is lost.
- Panels: synths draw their waveform (cheap, from the engine's own shape or a short oscillator render already used by
  JIANT's scope); FM6 its algorithm diagram (operators as dots and lines in the blend colours); DRUM-X the eight lanes
  as a row of cells with the picked lane lit and its level bar.

### 2.2 ENV

| Page 1 · AMP | Page 2 · MOD ENV | MORE |
|---|---|---|
| ATTACK `P_ATK` · DECAY `P_DEC` · SUSTAIN `P_SUS` · RELEASE `P_REL` | FILTER `P_ED_FLT` · PITCH `P_ED_PIT` · SHAPE `P_ED_SHP` · LOOP `P_ELOOP` | none (JIANT's STAGE view is dropped: it edited the same four values) |

Panel: the envelope's shape, redrawn as a knob turns.

### 2.3 LFO

| Page 1 · LFO | Page 2 · DEST | Page 3 · MOD MATRIX | MORE |
|---|---|---|---|
| RATE `P_LRATE` · WAVE `P_LWAVE` · MODE (TRIG/FREE) · FADE `P_LFADE` | PITCH `P_LD_PIT` · FILTER `P_LD_FLT` · SHAPE `P_LD_SHP` · AMP `P_LD_AMP` | SLOT 1–4 · SOURCE · DEST · AMOUNT (`P_M{k}SRC/DST/AMT`) | PHASE; MSEQ LENGTH `P_MSLEN`, SLEW `P_MSSLW`, DIVISION `P_MSDIV`, STEP 1–16 levels `P_MS0..` |

Panel: the LFO wave scrolling at its rate (page 1), the four destinations' amounts (page 2), the four slots as
source → destination lines (page 3). The panel animation follows the 3a pace rules (no faster than the ridgelines).

### 2.4 FX

| Page 1 · SENDS | Page 2 · SPACE | Page 3 · MASTER | MORE |
|---|---|---|---|
| DRIVE `P_DIST` · CHORUS `P_CHOR` · DELAY `P_DLY` · REVERB `P_REV` (per track) | DLY TIME `G_DTIME` · DLY FEEDBACK `G_DFDBK` · REV TYPE `G_RTYPE` · REV SIZE `G_RSIZE` | CLIP `G_CLIP` · PUNCH `G_PUNCH` · DUCK `G_DUCK` · DUCK REL `G_DREL` | DRIVE TYPE/TONE; AUTO PAN (DEPTH, RATE, SYNC, WAVE); DELAY: TONE, MIX, WIDE, PITCH, SPRAY, MOD, TYPE, PING; REVERB: DAMP, PRE/AIR/PITCH (label per type), MOD, RATE, FILT, WIDE; CHORUS: RATE, DEPTH, STEREO; SLICER: MODE, PATTERN, RATE, DEPTH, T1–T6 + FX bus, PITCH LENGTH, QUANT, PITCH STEP 1–16; BUS amounts; PUNCH: LENGTH, RESOLUTION, CLEAR (action, asks first) |

- CHORUS and SLICER rows hidden under LOAD ECO, as JIANT.
- The DJ FILTER (`G_DJF`) is GLO's (3c); the FX performance layer's FILTER macro keeps driving it.
- Panels: SENDS four send bars; SPACE the delay taps and reverb tail as a decaying line; MASTER a level meter with
  the clip point marked.

### 2.5 ARP and SCL

| Section | Page 1 | Page 2 | MORE |
|---|---|---|---|
| ARP | MODE `P_AMODE` · RATE `P_ARATE` · OCTAVES `P_AOCT` · GATE `P_AGATE` | SWING `P_ASWING` · CHANCE `P_APROB` · HOLD `P_AHOLD` · ORDER `P_AORDER` | none |
| SCL | ROOT `P_ROOT` · SCALE `P_SCALE` · SNAP `P_QUANT` · ALL TRACKS `G_STRN` | CHORD `P_CHRD` · VOICING `P_CVOI` · STRUM `P_CSTR` · ROTATE `P_SOFS` | PITCH OFFSET `P_POFS`, TRANSPOSE `P_TRANS` |

Panels: ARP the note pattern as dots; SCL the scale's notes on a one-octave keyboard (root highlighted).

### 2.6 Held layers

Restyled to the 3a look (map overlay in ux colours and fonts); functions unchanged: EDIT (F3 INIT, G3 RECALL, both
asking first; DRUM black keys mute lanes; K1–K4 = EDIT page 1; +SELECT dice), LFO MACRO (K1–K4 macros, latch, F3
DICE, G3 CLEAR), FX performance (all keys, mutes, freeze, macros STEPS / TRANSP / MUTATE / FILTER, glitcher knobs,
latch), SCL (keys set ROOT; K1–K4 ROOT, SCALE, ROTATE, PITCH OFFSET). ROTATE and PITCH OFFSET become reachable
without the held layer (§2.5), per §0 rule 2.

## 3. Generator genres (interim until piece 6)

Shown, in this order: **IDM, BROKEN BEAT** (BROKEN), **BREAKS, DRILL'N'BASS** (DRUM & BASS), **ACID** (ACID HOUSE),
**ELECTRO, MICRO** (MICRO HOUSE), **RETROBOC** (BOOM BAP), **AMBIENT, DRIFT** (IMPRESSION); plus EXPLORE and RANDOM.
The other 33 are hidden. Rules: the stored genre value `gp.genre` (0 RANDOM, 1 + index, EXPLORE) and seed hashing
are unchanged; only the displayed list (`gen_knob` walk, names, gauge) and every random genre draw (`gr() % NGENRE`
sites: RANDOM, mutants) are filtered to the ten; a song or setting stored with a hidden genre still opens and
regenerates as before (its genre name shown as JIANT's). Names are display names only.

## 4. Power-on demo

When power-on has nothing to restore (RESTORE LAST off, or no autosave), the parts load (`power_on_parts`,
`TRK_DEF`): T1 DRUM-X with PATTERNS BEAT (12); T5 ANALOG with its **ACID** preset and PATTERNS ACID (1); T6 FM6 with its
factory preset whose sound type is PAD (the first one in the list; if none, the first KEYS one) and PATTERNS PAD (5); T2–T4 keep their sounds with empty patterns. Tempo 124. The status line on HOME
says "DEMO: EDIT ANYTHING, OR HOME → GENERATE" until the first edit or PLAY+STOP cycle. NEW PROJECT (`system_init`)
still starts empty. RESTORE LAST behaviour unchanged.

## 5. Other 3b items

- TRACKS under load: while `load_tight`, the ridge panel paces at 100 ms → 200 ms (half rate); re-measured on the
  device with the CPU TEST song (MENU / TRACKS / VISUAL as in 0.3's device log).
- `ux_knob_touch` from every 3b page's knob handler (600 ms highlight).
- Font coverage: the 3a message-literal scan extended to every 3b label and MORE row; add glyphs as needed.
- RAM: no new `.pool` data (224 B margin); new tables in `.rodata` / `.bss`; DC cell budget per page ≤ 1920.
- [3b] backlog items: creature reds (gone with the creatures), HC font slot thrash on EDIT (profile; keep EDIT to two
  Huffman faces at most), knob-value width test in UF_VAL.

## 6. Testing

- Renders: every page of every section and engine (incl. DRUM-X lanes, FM6 ops), every MORE list scrolled to its end,
  each held layer, lint clean.
- Coverage test: every `P_*` / `G_*` / id that JIANT's EDIT, ENV, LFO, FX, ARP, SCL pages and layers reached is on a
  3b page or MORE row (generated from the old PAGES/CHAINS tables at test-build time or kept as a reviewed list).
- Navigation: the seven rules on every section (press again cycles, SELECT steps and stops at ends, MORE scroll,
  page memory per section and per engine kind).
- Genres: ten shown in order with display names; RANDOM and mutants never draw a hidden genre (many seeds); a stored
  hidden genre loads and regenerates identically.
- Power-on demo: fresh boot with no autosave → the three patterns and presets, PLAY sounds (host: steps present and
  the voices start); NEW PROJECT empty; RESTORE LAST unchanged.
- Device (0.4): no-manual check ("make T5's acid line brighter", "add delay to T3", "change the scale to minor",
  "press PLAY on a fresh boot"); CPU TEST late blocks/s on MENU, TRACKS, an EDIT page, the LFO page.

## 7. Order

1. Genres; 2. power-on demo; 3. MORE list widget + section page-table engine (`ux_nav` tables for sections); 4. EDIT
synths; 5. EDIT FM6 + DRUM-X; 6. ENV + ARP + SCL; 7. LFO; 8. FX; 9. layers restyle; 10. TRACKS load + backlog; 11.
version 0.4, docs, device. The plan may merge or split steps.
