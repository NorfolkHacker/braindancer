# ACID FM — interface redesign (piece 3)

Date: 2026-10-24. Status: approved in brainstorming (visual choices made in the browser companion), awaiting spec
review. Parent spec: `docs/superpowers/specs/2026-10-24-acid-fm-design.md` §8 (complete redesign) and §10 item 3.
Mockups: `docs/acid-fm/ui-mockups/` (the browser companion's screens, kept for reference).

## 0. The first rule: intuitive and easy to use

The user's top requirement (2026-10-24): "the whole app is intuitive and easy to use, as most firmwares like this
aren't". Every screen and every later piece is judged against it before its looks:

1. **The screen says what the controls do.** Every page names what its four knobs do, and the footer says where you
   are and what the next press goes to. No page needs the manual to operate.
2. **The core jobs never need a held-button combo.** Making a beat, changing a sound, writing steps, saving, loading
   a project, generating a song and (piece 4) loading and chopping a break are all reachable by pressing labelled
   buttons and turning knobs. Combos and held layers are shortcuts only, never the only way.
3. **One rule everywhere.** The seven rules of §4.1 have no exceptions; a section never invents its own navigation.
4. **Plain words.** Labels say what a thing does (`CUTOFF`, `DECAY`, `SLIDE`), not internal names; abbreviations only
   where space forces them, and the same abbreviation everywhere.
5. **Nothing destructive by surprise.** Anything that loses work asks first (OCT+ yes / OCT− no) and is undoable
   (GLO + G4 UNDO, kept); the dialog says what will be lost.
6. **Feedback at once.** A knob turn shows its new value on screen immediately; a press that changes page shows the
   new page within one frame; a refused action says why in a short message.
7. **Discoverable.** The first time a section is opened, its footer shows a one-line hint (JIANT's once-only hints,
   restyled); MORE lists everything else so nothing is hidden.

**Checked every sub-piece:** besides the automated tests, the user does a short task list in the browser emulator
**without the manual** (e.g. "make a 4-bar acid line with two slides and an accent", "change T3's sound and save it",
"make the HOME screen show T5's level") and anything confusing is fixed before the device install. A page that needs
a sentence of explanation gets that sentence on screen or gets redesigned.

## 1. Decisions (user, 2026-10-24)

| Question | Decision |
|---|---|
| Depth | New look **and** new navigation; every screen changes, in stages |
| Visual direction | **C, braindance minimal** (Designers Republic / Autechre sleeve: Swiss grid, big type, thin lines, dark space) with **B's colour blending at strength 3, "glow"**: the acid → slide → violet → red → amber blend on a 2 px top line and titles, each knob owning one colour as a value bar, a soft colour glow behind the picture panel that breathes with the music, the blend as a solid footer band |
| Buttons | **Labels keep their meaning**, every section flatter (≤ 3 pages + MORE) |
| HOME | **Six tracks** (levels, what each track is, the selected track's 16 steps, section / bar); **HOME pressed again flips to the visual** (full-screen music-reactive kaleidoscope with the song's name), and back |
| SEQ | **Roll over a step strip**: the melody's shape on top, a 16-cell strip below (note / accent / slide / rest), the edited step spelled out |
| Splash | **Wordmark**: a colour line sweeps across, the gradient "ACID FM" types on, version and "BY NORFOLKHACKER" under it; JIANT's signature is gone |

## 2. Hardware and budgets (fixed)

- Screen: ST7789-class **240 × 240, RGB565**, SPI at 12 MHz (a full frame ≈ 77 ms; only changed areas are sent).
  JIANT's canvas strip (`gfx.c`, 240 × 150, `CV_MAX`) and LCD DMA path are reused.
- Controls: KNOB 1–4; encoders SELECT, ALGORITHM, PRESETS; MASTER; buttons HOME ENV LFO FX SCL EDIT GLO SAVE ARP SEQ
  PLAY REC OCT− OCT+; the keyboard (its keys also act inside held layers, as in JIANT).
- Flash: about 30 KiB free in the app slot (piece 2: app 553 708 B of 584 336). The new UI is paid for by deleting
  JIANT's skins (COLORS, HUD, AIR, MONO, PAPER, GREY palettes and their code), creature drawings (`ui_being.c`,
  `ui_engvis.c`, `ui_organic.c`, the per-engine creatures), the LENTE lens pages and the splash signature
  (`ui_signature.h`) as each section moves over. The build's flash and RAM-code checks stay.
- CPU: the audio already saturates the CPU in the worst case (`docs/acid-fm/voices.md`). The UI runs in the main
  loop, never in the audio interrupt; anything animated must back off under load (§5.3).

## 3. Look

### 3.1 Palette (SidAcid `theme.rs`)

| Token | Hex | Use |
|---|---|---|
| BG | `#141518` | screen |
| BG2 | `#1A1C20` | cells, cards |
| LINE | `#303338` | rules, empty bars, empty steps (`#26282D`) |
| TEXT | `#E8EAED` | values |
| MID | `#9AA0A6` | labels |
| ACID | `#B8F53A` | primary; knob 1; notes |
| SLIDE | `#4CB8FF` | knob 2; slides |
| VIOLET | `#A64BFF` | knob 3; glow centre |
| ACCENT | `#FF503C` | knob 4; accents; warnings |
| QUEUED | `#FFC83C` | queued / pending; the blend's end |
| ACID LIGHT `#DCFA9D`, VIOLET LIGHT `#D3A5FF` | | highlights |

The **blend** is ACID → SLIDE → VIOLET → ACCENT → QUEUED, precomputed once as a 256-entry RGB565 table.

### 3.2 Type

Inter Tight (already in `assets/fonts`, SIL OFL 1.1), rasterised at build time by `tools/gen_aa_font.py` into a
second header beside JIANT's `ui_fonts.h` (whose faces the unmoved sections keep using until they move), at the sizes the layout needs: about 8 px labels (tracked, upper case), 10 px header, 13–14 px
values, 30–34 px titles, 52 px splash wordmark. Gradient text = glyph coverage × the blend table along x.

### 3.3 Page anatomy (every page)

1. **Top line**: the blend, 2 px.
2. **Header** (≈ 14 px): left `T3 · ` + the sound's name in gradient text; right the tempo and play state / position
   (`A · 1/4`, `128 BPM ▶`).
3. **Knob row** (≈ 28 px): four columns; label (MID, 8 px), value (TEXT, 13–14 px), a 2–3 px bar in the knob's colour
   (KNOB 1 ACID, 2 SLIDE, 3 VIOLET, 4 ACCENT) showing the value's position; the column a knob just moved is brighter
   for a moment.
4. **Panel** (≈ 150 px, the canvas strip): the page's picture over the glow.
5. **Band footer** (14 px): the blend as a solid band; dark text: `SECTION · PAGE` left, `n / N` and the knobs' short
   names right.

The **glow**: a soft radial violet → slide fade behind the panel, drawn from a precomputed band table (no per-pixel
maths), its brightness breathing with the audio level (§5.3).

## 4. Navigation

### 4.1 Rules (one implementation, every section)

1. A section button opens its section on the page last used there; pressed again it steps to the next page, round.
   At most **3 pages + MORE** per section.
2. SELECT steps through the section's pages (a detent a page) and scrolls inside MORE.
3. ALGORITHM picks the track T1–T6; PRESETS picks presets (the selected track's sounds; on SEQ, patterns), everywhere.
4. KNOB 1–4 set the four values shown; each knob keeps its colour on every page.
5. **MORE** is a section's last page: a list of everything else in the section; SELECT scrolls, KNOB 1 picks a row,
   KNOB 2 sets it. Nothing JIANT had is removed: a setting not on a page is a MORE row.
6. A button held is its layer, as in JIANT (GLO: mutes / solos / levels / tap / UNDO; HOME held: MENU; SAVE, SEQ
   TOOLS, EDIT, REC, LFO MACRO, FX punch, SCL layers), restyled.
7. OCT+ / OCT− are yes / no in dialogs, the octave otherwise. JIANT's combos (HOME+SELECT GEN, EDIT/SEQ/LFO/FX+SELECT
   dice, GLO+SELECT tempo, SEQ+REC clear all, REC held + section: clear automation, SAVE+OCT± sections, OCT−+OCT+ at
   power-on calibration) keep working.

### 4.2 Sections and pages

| Button | Pages (→ = pressing the button again / SELECT) | Piece |
|---|---|---|
| HOME | TRACKS → VISUAL; HOME + SELECT: GEN (JIANT's until piece 6) | 3a |
| EDIT | SOUND → TONE → FOCUS (FM6: OPERATOR; DRUM-X: KIT) → MORE | 3b |
| ENV | AMP → MOD ENV → MORE | 3b |
| LFO | LFO → MOD MATRIX → MORE (MSEQ, macros) | 3b |
| FX | TRACK (sends, drive, pan) → SPACE (delay, reverb) → MASTER (DJ filter; TAPE in piece 5) → MORE (chorus, slicer, punch-in) | 3b |
| SCL | KEY & SCALE → CHORD | 3b |
| ARP | ARP → MORE | 3b |
| SEQ | STEPS (roll over strip) → PATTERN (length, swing, chance, ratchet) → MORE (mutate, playheads, comp, automation) | 3c |
| GLO | MIX (levels, pans) → GLOBAL (tempo, swing, master) → MORE (live, system) | 3c |
| SAVE | SOUND (user presets) → SONG (sections, arrange, projects) → TOOLS | 3c |
| MENU (HOME held) | SCREEN · AUDIO · MIDI · USB · SYSTEM as the same page/MORE model; ABOUT restyled | 3c |

Pages are **data tables** (`ux_nav.c`): each page names its title, its four setting ids (`P_*` / `G_*` / menu items),
its picture and its MORE rows. Each sub-piece's plan writes the full tables for its sections from JIANT's parameter
lists (`params.c`, `menu_items.c`, the engines' `E1..E8` pages); a test (§6) proves every setting JIANT exposed is on
a page or a MORE row. Setting ids, stored formats, the editor protocol and MIDI CCs do not change.

### 4.3 HOME (piece 3a)

- **TRACKS** (revised with the user, 2026-10-24: "ridgelines", replacing level columns): six stacked lines T1 (top)
  … T6 (bottom), like a pulsar plot; each line is its track's level history over the last ≈ 2 s, scrolling right to
  left (from the per-track peak the audio code already measures — no extra audio-path work), drawn in the blend's
  colour along x, each line filled below in BG so it hides the line behind; a silent track is flat. The four tracks
  the knobs control are drawn in their knob colours (ACID, SLIDE, VIOLET, ACCENT) and full brightness, the other two
  dimmed; each line's name at its left (`T3 ACID`) and its level as a small number at its right end. Under the
  lines the selected track's 16 steps of the current page (notes ACID, accents ACCENT, slides SLIDE, rest LINE, the
  playing step outlined); header right: section and bar. Mockup: `docs/acid-fm/ui-mockups/home-unique.html` option 1.
  Knobs: the levels of four tracks, the window of four that holds the selected track (T1–T4 while T1–T4 is selected,
  T3–T6 for T5–T6; ALGORITHM keeps picking the track, rule 3), their columns marked; glow behind.
- **VISUAL** (HOME pressed on TRACKS): the kaleidoscope (concentric rings of the blend, rotating with the bar,
  pulsing with the kick / the master level), the song's name and `SECTION · BAR` over it. Knobs: macros M1–M4 (JIANT's
  macros). HOME again → TRACKS.

### 4.4 SEQ STEPS (piece 3c)

Top ≈ 96 px: a roll of the current 16 steps (pitch up, time across; slides as joined bars in SLIDE, accents ACCENT,
notes ACID, the edited step outlined white, the playing step a light column). Below: a strip of 16 cells, 13 px wide,
coloured note / accent / slide / rest, and the edited step spelled out (`5 · G2 · ACC`). Knobs: STEP, NOTE,
ACC/SLD, GATE. 32 / 64 steps: SELECT on the strip's last cell pages to the next 16 (shown as `1/4` in the header).
BREAK tracks (piece 4) reuse the strip with slices instead of notes.

### 4.5 Splash (piece 3a)

≈ 1.5 s: the blend line sweeps left to right at y ≈ 96; "ACID FM" in 52 px gradient text types on under it; the
version and "BY NORFOLKHACKER" (8 px, tracked) appear; then HOME. MENU › SCREEN › ANIMATION OFF shows the last frame
at once (JIANT's setting kept).

## 5. Implementation

### 5.1 Files (new, focused)

| File | Responsibility |
|---|---|
| `firmware/src/ux_theme.c/.h` | palette tokens, knob colours, the 256-entry blend table, gradient text |
| `firmware/src/ux_widgets.c/.h` | header, knob row (+ bars), panel + glow, band footer, step strip, roll, track meters, MORE list, dialog |
| `firmware/src/ux_nav.c/.h` | the rules of §4.1; the section / page tables; page memory per section |
| `firmware/src/ux_home.c` | HOME TRACKS and VISUAL |
| `firmware/src/ux_splash.c` | the wordmark splash |

They draw through JIANT's `gfx.c` / `lcd.c`; JIANT's `ui_*.c` keep driving a section until its sub-piece moves it.
A section moved to `ux_` loses its old page code in the same sub-piece.

### 5.2 Order

- **3a**: theme, widgets, nav (rules + tables for HOME), HOME, splash, fonts; one palette (ACID) replaces JIANT's 11
  and one skin replaces COLORS / HUD / AIR, so the sections still on JIANT's code are drawn in ACID colours until they
  move; HOME's creatures (`ui_lzhome.c` ECO / BEING) and the splash signature (`ui_signature.h`) deleted. MENU ›
  SCREEN's COLOR / SKIN / STYLE rows keep their editor ids (append-only protocol) but do nothing and are hidden.
- The EDIT pages' creatures (`ui_engvis.c` `vx_draw`, `graph_being`, `graph_colony`) go in **3b** with EDIT; the FM6
  OPERATOR / DRUM-X KIT pictures in `ui_engvis.c` are real pages and are redrawn there, not deleted. Shared helpers
  (`og_*` lines, `bg_energy`, `heat_col`) stay while any caller does.
- **3b**: EDIT (all engines), ENV, LFO, FX, ARP, SCL.
- **3c**: SEQ, SAVE (songs, arrange, projects, tools), GLO, MENU and ABOUT; the LENTE lens code removed.
- **3d**: manual (`docs/MANUAL.md`, `web/manual`), cheatsheets, website screenshots, emulator labels; piece 1/2
  backlog doc items tagged [piece 3].

Each sub-piece: its own plan, builds, passes the suite, installs on the FM-1 with the user's check, merges.

### 5.3 CPU rules

- The glow and VISUAL read the audio level at most every 100 ms; drawing is table lookups.
- VISUAL redraws at most ≈ 15 frames/s, only when the main loop has time (JIANT's "screen held under load" rule:
  above 60 % render load it slows to one redraw per 50 ms or less), and freezes while the overload guard is shedding.
- Only changed rectangles go to the LCD.

## 6. Testing

- **Renders**: `tests/ui_render.c` / `ui_test.c` render every new page and state to PNG (`build/ui_new/`); JIANT's
  clipped / overlapping-text lint runs on all of them; a hash per new screen guards against unintended changes
  (updated only for intended ones, as goldens).
- **Navigation**: button / encoder sequences → expected page and section page memory, for all seven rules.
- **Coverage**: every setting JIANT exposed (the `P_*`, `G_*` and menu item lists) is on a page or a MORE row of its
  section (per sub-piece for its sections; complete after 3c).
- **CPU on the device**: the CPU TEST song with HOME VISUAL showing adds no late audio blocks compared with TRACKS
  (`status` over the serial console: `audio_late`, `audio_max_us`); main-loop draw time per page measured with JIANT's
  PROFILE.
- **Emulator**: the browser emulator runs the same code; each sub-piece is played there before the device.
- Sound: goldens unchanged throughout piece 3.

## 7. Out of scope

- New features (BREAK pages arrive in piece 4, TAPE in piece 5, the new generator in piece 6; their pages follow this
  spec's rules and widgets).
- Changing setting ids, stored formats, MIDI CCs or the web editor's protocol.
- Touch, external displays, new fonts beyond Inter Tight.
