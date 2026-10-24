# ACID FM piece 3a (UI foundations, HOME, splash) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ACID FM 0.3: the new look's foundations (ACID palette and fonts, gradient text, glow, the shared widgets,
the navigation tables), a new HOME (TRACKS, and VISUAL on a second HOME press), the wordmark splash; JIANT's 11
palettes and 3 skins become one, HOME's creatures and the splash signature are gone. The other sections keep JIANT's
pages, drawn in ACID colours, until 3b / 3c.

**Architecture:** New `ux_*.c` files beside JIANT's UI, included in the unity build (`firmware/src/felucca.c`) and
the UI tests (`tests/ui_test.c`), drawing through `gfx.c`'s canvas (`cv_*`) and `lcd.c`. HOME's navigation entries in
`ui_lens.c` (`CE_HOME`, `CG_HOME`) are rewritten to TRACKS / VISUAL + GEN; HOME drawing moves from `ui_lzhome.c`
(deleted) to `ux_home.c`. The palette generator emits one palette; the COLOR / SKIN / STYLE menu rows keep their editor
ids but do nothing.

**Tech Stack:** C (JieLi clang for the device, host gcc via `tools/host-cc` for tests), Python build generators
(`tools/gen_aa_font.py`, `tools/gen_ui_palettes.py`), JS tests (Node in `build/node`).

**Spec:** `docs/superpowers/specs/2026-10-24-acid-fm-ui-design.md` (§1–§6; this plan is §5.2's **3a**). Mockups:
`docs/acid-fm/ui-mockups/` (`home.html` option 1 and 2, `splash.html` option 1, `blend-amount.html` option 3).

## Global Constraints

- GPL-3.0-only. Never remove a JIANT or Felucca copyright line; changed files gain
  `Portions Copyright (C) 2026 NorfolkHacker` (C in the header comment; Python / shell `#`; JS `//`); new files carry
  `SPDX-License-Identifier: GPL-3.0-only` and `Copyright (C) 2026 NorfolkHacker`.
- Setting ids, stored formats, the editor protocol and MIDI CCs do not change (append-only: MI_COLOR id 0, MI_STYLE 1,
  MI_SKIN 21, MI_SCOPE 22 keep their ids; their values become inert).
- No sound change: `tests/golden.txt` unchanged. The UI never runs in the audio interrupt.
- Palette (spec §3.1): BG `#141518`, BG2 `#1A1C20`, LINE `#303338`, EMPTY `#26282D`, TEXT `#E8EAED`, MID `#9AA0A6`,
  ACID `#B8F53A`, SLIDE `#4CB8FF`, VIOLET `#A64BFF`, ACCENT `#FF503C`, QUEUED `#FFC83C`, ACID LIGHT `#DCFA9D`,
  VIOLET LIGHT `#D3A5FF`. Knob colours: 1 ACID, 2 SLIDE, 3 VIOLET, 4 ACCENT. The blend: ACID → SLIDE → VIOLET →
  ACCENT → QUEUED.
- Every new `.c` is added to `firmware/src/felucca.c`'s include list, `tests/ui_test.c:127-138`'s include list and
  `tools/size_fns.py` `SIZE_FILES`.
- `cv_px` holds byte-swapped RGB565 (`swap16`, gfx.c:215): write it only through `cv_pset` / `cv_rect` or swap.
- Environment: `cd ~/synth/mvavefirmware && . .venv/bin/activate && export JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK PATH=$PWD/build/node/bin:$PATH`;
  build `./build.sh`; suite `CC=tools/host-cc sh tests/run_tests.sh`; screens land in `build/ui_new/`.
- Branch `piece3a-ui`. Commits end with a blank line and `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
  Never on main, never push. Device steps are the controller's with the user (the serial console `/dev/ttyACM0`
  `status` may be read freely).

## Codebase map (from the exploration; verify line numbers before editing)

- Unity build: `felucca.c:68-97` includes libc, lcd, gfx, panel, ui, menu_items, icons, ui_graph, ui_draw, ui_menu,
  ui_input, ui_layer; nested: `ui.c:653` ui_lens.c, `ui_graph.c:66-91` ui_organic / ui_fm6op / ui_being / ui_engvis,
  `ui_graph.c:1739-1744` ui_tech / ui_lzsave / ui_lzfx / ui_lzhome / ui_lzedit / ui_lzpage, `ui_draw.c:1788`
  ui_signature.h, `ui_input.c:2664` ui_dx.c.
- Canvas `cv_px[240*150]` (`gfx.c:81-83`), `cv_begin(w,h,bg)` 218, `cv_rect` 429, `cv_line` 463, `cv_rrect` 505,
  `cv_text` 923 / `cv_text_r` 928, `text_w`, `cv_blit(x,y)` 303, `cv_blit_cells` 373 with `dc_frame(all)` (changed
  8×4 cells only). `ux_mix(a,b,pct)` gfx.c:157. Theme struct `ux` gfx.c:104-117 set by `palette_set(i)` 176; T_*
  macros 135-152. Fonts `AF_S/AF_M/AF_L` from `build/gen/ui_fonts.h` (gfx.c:29), made by `tools/build.py generate()`
  with `--preset jiant-lab`.
- Palettes: `tools/gen_ui_palettes.py` → `build/gen/ui_palettes.h` (`UI_PALETTES[11]`, `UI_NPALETTES`,
  `UI_GREY_INDEX`, `UI_BW_INDEX`, `UI_JIANT_INDEX`, `UI_KNOBS`, `UI_RAISE_OF`, `UI_PALETTE_MIGRATE[20]`, `UI_REC_*`,
  `UI_CRASH_*`, `UI_QR_*`). `settings.palette` (panel.c:72), `palette_from_stored/to_stored/stored_ok` gfx.c:206-213,
  `settings_jiant_once` settings_persist.c:104-112. Skin `ui_skin` = `favorites.factory[15][20]` (ui.c:179),
  `SKIN_COLORS 0 / SKIN_BOTTOM_OLD 1 / SKIN_HUD 2 / SKIN_AIR 3` (gfx.c:122-126), `ux.skin` via `style_apply`
  (ui_draw.c:2247-2260), `skin_hud()` gfx.c:131; STYLE `factory[15][29]`, SCOPE `[19]`. Menu rows menu_items.c:16,
  :41-42, :83-85, :168-246; editor ids editor_menu.c:16-27; `ED_UI_PALETTES` editor_preferences.c:69,97-99.
- Tests pinning palettes / skins: theme_test.c:51-58, settings_test.c:31-63, persistence_test.c:572-676,
  editor_test.c:172-175, 815-837, 863, 890-922, startup_test.c:37-45, ui_render.c :586 `pal()`, loops 2063 / 2368 /
  2391 / 2422 / 2463, skins 2456-2615, ui_test.c mono / grey checks, web/test_web.mjs palette lists.
- HOME: `CE_HOME` (ECO, MACROS, BEING, ROUTES, GEN 1-3), `CG_HOME`, `nav_go` ui_lens.c:193, `nav_button` 240,
  `nav_select` 258, page memory `ui.nmem`, `ui.home`, `ui.hlv`, `ui.hset[2]`, `go_home()` ui.c:1068, `hud_home()`
  ui.c:683, `home_macros()`, `home_param(k,&vp)` ui.c:1731; input ui_input.c:2103 (HOME hold), 2122-2131 (HOME+SELECT
  → GEN), 2222-2236 (menu), 2395 (tap), 2540-2570 (knobs); GEN `gen_on()`, `gen_page()`. Drawing ui_lzhome.c (ECO /
  BEING), ui_draw.c draw_head / draw_columns / draw_graph, HUD HOME ~2411-2425.
- Data: `trk[NTRK]`, `TSEL`, `song.sel`, `step_t` (core.h:400-416: note[4], n, time ST_NOTE/ST_TIE/ST_REST, flags
  SF_ACCENT 1 / SF_SLIDE 2, vel), `p[P_SLEN]`, `seq_src(t,idx)` seq.c:585, `t->seq_idx`; `trk[c].peak` written by
  fx.c:1966-2020 and **cleared by its reader** (ui_lzhome.c:86); `bg_energy()` master level; `macro_v[4]` mod.c:62;
  `song.playing`, `song.g[G_BPM]`, `chain` (song_chain.c:81-93: row, remaining, running, bar_pos), `chain_config`,
  `song_cur`.
- Load: `load_tight` voice.c:325 (on > 65 %, off after 16 halves < 60 %), `gov_lite`, `load_diag`, `song.cpu_q8`,
  `sys_load` (LOAD LITE / ECO). Main loop main.c:~400-420, ≤ 1 frame / 15 ms; `ui_draw()` ui_draw.c:2286.
- Splash: ui_draw.c:1782-1936 (`SPL_WRITE 800`, `SPL_HOLD 900`, `SPLASH_LINES`, `splash_sig`, `draw_splash`,
  `splash_write(ms)`, `spl_anim`), boot main.c:244-245 and 296-305, `PREF_ANIM_OFF`; also used by
  `web/emu/felucca_web.c:259-260, 273, 317`; test ui_render.c:1879-1905 (S_SPLASH, `in_splash` text collector).
- Render tests: `tests/ui_render.c` `enum S_*` + parallel `S_NAME[]`, `setup()` / `draw()` cases, `lint()` (clipped /
  overlapping / ellipsised text via GFX_HOOK_*), PPM → PNG by `tests/ui_render.py`.

---

### Task 1: Inter Tight faces in a second font header

**Files:** `tools/gen_aa_font.py`, `tools/build.py` (`generate()`), `firmware/src/gfx.c` (include), `tests/text_ref_test.c`
or a new `tests/ux_font_test.c` (+ `tests/run_tests.sh`).

**Interfaces — Produces:** `build/gen/ux_fonts.h` with `aafont_t` faces `UF_LBL` (Inter Tight @600, 8 px, upper-case
labels: `" #%+-./0123456789:<>ABCDEFGHIJKLMNOPQRSTUVWXYZ·→/"`), `UF_HDR` (@500, 10 px, printable ASCII), `UF_VAL`
(@500, 14 px, printable ASCII), `UF_TTL` (@700 or the heaviest available, 32 px, `" -./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"`),
`UF_MARK` (@700, 52 px, `" ACDFIM"` — the wordmark's letters only), with `_CAP_Y` / `_CAP_H` metrics like AF_*;
`#include "ux_fonts.h"` next to `ui_fonts.h` in gfx.c.

- [ ] Step 1 — failing test: a host test that includes gfx.c and asserts each UF_* exists, `text_w(&UF_VAL, "128")`
  > 0, `UF_MARK` has glyphs for "ACID FM" and that their cap heights are in order (LBL < HDR < VAL < TTL < MARK).
  RED: header missing.
- [ ] Step 2 — generator: add an output mode to `gen_aa_font.py` (e.g. `--out ux_fonts.h --preset acid`) using the
  existing `--face NAME=FONT[@wght]:PX:CHARS` machinery; keep `PHASES_L` (sub-pixel phases) minimal for TTL / MARK to
  limit flash; `tools/build.py generate()` runs it after the jiant-lab run. JIANT's `ui_fonts.h` / AF_* stay untouched.
- [ ] Step 3 — GREEN; build; record the flash cost of the new faces (app size before / after) in the report and the
  commit message. If the cost is above 12 KiB, reduce character sets (do not drop a face) and say so.
- [ ] Step 4 — commit `feat(ui): Inter Tight faces for ACID FM's screens (ux_fonts.h)`.

---

### Task 2: One ACID palette and one skin; COLOR / SKIN / STYLE inert

**Files:** `tools/gen_ui_palettes.py`, `firmware/src/gfx.c`, `panel.c`, `settings_persist.c`, `ui.c`, `ui_draw.c`
(`style_apply`), `menu_items.c`, `editor_preferences.c`, every `UI_*_INDEX` / `skin_hud()` / `ux.skin` /
`settings.palette == UI_JIANT_INDEX` branch, and the tests listed in the codebase map (theme, settings, persistence,
editor, startup, ui_render, ui_test, web/test_web.mjs).

**Interfaces — Produces:** `UI_NPALETTES 1`, `UI_ACID_INDEX 0` (the only palette, named "ACID", five stored colours
chosen so JIANT's derived tokens land on the spec palette: BG `#141518`, text `#E8EAED`, theme `#B8F53A`, accent
`#FF503C`, surface `#1A1C20`; adjust `UI_*_PCT` derivations so MID ≈ `#9AA0A6` and LINE ≈ `#303338`); one skin
(AIR's look — the closest to spec §3.3 — with `ux.skin` constant); `palette_from_stored(any)` → 0; COLOR / SKIN /
STYLE menu rows hidden from MENU › SCREEN but their editor ids answer (a GET returns 0, a SET is accepted and ignored).

- [ ] Step 1 — failing tests: theme_test asserts `UI_NPALETTES == 1` and the palette's derived tokens equal the spec
  hex values (± 1 per RGB565 channel step); settings_test: any stored palette 0..19 (old and older firmware) loads as
  ACID; editor_test: GET MI_COLOR / MI_SKIN / MI_STYLE returns 0, SET returns rc ok and changes nothing;
  startup_test: fresh settings → ACID, the one skin. RED.
- [ ] Step 2 — implement. Remove the code paths only other palettes / skins reached (`mono`, `chamfer`, HUD HOME,
  COLORS cards, BOTTOM) where they are now dead; keep one path. `UI_KNOBS` becomes the four knob colours of the spec.
  `UI_REC_*`, `UI_CRASH_*`, `UI_QR_*` keep working (crash screen and QR stay readable).
- [ ] Step 3 — rewrite the pinned tests' expectations (not their intent): the palette loops in ui_render.c run once
  (ACID); `SHOW` is ACID; mono / grey-neutral checks that only made sense for other palettes go; editor / web tests
  expect one palette "ACID". The text lint must pass on every screen in ACID.
- [ ] Step 4 — build, suite (goldens unchanged), look at a sample of `build/ui_new/ACID/*.png` (the unmoved JIANT
  pages now in ACID colours) and list any unreadable combination in the report.
- [ ] Step 5 — commit `feat(ui): one ACID palette and one skin; COLOR / SKIN / STYLE kept as inert ids`.

---

### Task 3: `ux_theme` — gradient text and glow

**Files:** create `firmware/src/ux_theme.h`, `firmware/src/ux_theme.c`; add to felucca.c, ui_test.c, size_fns.py;
test `tests/ux_theme_test.c` (+ run_tests.sh).

**Interfaces — Produces:**
```c
/* the blend, 256 entries, ACID → SLIDE → VIOLET → ACCENT → QUEUED (native RGB565) */
extern const uint16_t UX_BLEND[256];            /* or built once by ux_theme_init() into RAM, 512 B */
uint16_t ux_knob_col(uint32_t k);               /* 0..3 → ACID, SLIDE, VIOLET, ACCENT */
void ux_text_grad(int32_t x, int32_t y, const aafont_t *f, const char *s, uint16_t bg); /* canvas text, colour from UX_BLEND by x across the text's width */
void ux_glow(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t level);  /* the panel glow; level 0..3 */
void ux_band(int32_t y, int32_t h);             /* the blend as a solid band across the canvas */
void ux_topline(void);                          /* the 2 px blend line at canvas y 0 */
```
- `ux_text_grad`: draw the text as JIANT does (anti-aliased) in a reference colour, then recolour each pixel in its
  bounding box: coverage a = how far the pixel moved from `bg` toward the reference, out = `ux_mix(bg, UX_BLEND[xi],
  a)` with xi = (x − x0)·255 / (w − 1). Pixel writes respect `cv_px`'s byte swap.
- `ux_glow`: no per-pixel maths per frame: concentric filled rounded rects (`cv_rrect`) from the outside in, colours
  `ux_mix(BG, VIOLET→SLIDE, pct)` from a small const table per level (4 levels × ≤ 8 rings); level 0 = no glow.
- [ ] Step 1 — failing test: UX_BLEND[0] == ACID, [255] == QUEUED, monotone hue order at 64/128/192 (SLIDE, VIOLET,
  ACCENT within one RGB565 step); `ux_text_grad` on a blank canvas: leftmost ink pixel's colour is near ACID, rightmost
  near QUEUED, background pixels untouched; `ux_glow(level 0)` leaves the canvas untouched, level 3 makes the centre
  brighter than the edge. RED.
- [ ] Step 2 — implement; GREEN; build (record flash); commit `feat(ui): ux_theme: the blend, gradient text, glow, band`.

---

### Task 4: `ux_widgets` — the shared pieces

**Files:** create `firmware/src/ux_widgets.h/.c`; register; tests in `tests/ui_render.c` (new `S_UX_DEMO` screen) and
`tests/ux_theme_test.c` or a new `tests/ux_widgets_test.c`.

**Interfaces — Produces:**
```c
typedef struct { const char *label; char value[8]; uint8_t bar; /* 0..255 */ } ux_knob_t;
void ux_header(uint32_t trk, const char *sound, const char *right);      /* top line + header, LCD y 0..15 */
void ux_knobs(const ux_knob_t k[4], int32_t hot);                         /* knob row, LCD y 16..47; hot = 0..3 or -1 */
void ux_panel_begin(uint32_t glow_level);                                 /* canvas 240x150 at LCD y 48, BG + glow */
void ux_panel_end(void);                                                  /* push the panel (changed cells only) */
void ux_footer(const char *where, const char *right);                     /* band footer, LCD y 226..239 */
void ux_meters(const uint8_t lvl[6], const char *const name[6], uint32_t sel, uint32_t knob_first); /* in the panel */
void ux_strip(int32_t y, const step_t *steps, uint32_t n16, int32_t edit, int32_t play);           /* 16 cells */
```
Layout per spec §3.3 (top line 2 px, header ≈ 14, knob row ≈ 28, panel 150, footer 14 = 240 − 2 − 14 − 28 − 150 −
14 + small gaps: settle exact y's so they sum to 240 and record them in `ux_widgets.h` as `UX_Y_*` constants).
Strip colours: note ACID, accent ACCENT (over note), slide SLIDE, tie / held = the previous cell's colour dimmed,
rest EMPTY, edit cell outlined TEXT, play cell lighter.
- [ ] Step 1 — failing tests: the `UX_Y_*` constants sum to 240 with no overlap; `ux_strip` colours for a known
  pattern (note, accent, slide, rest) at known pixels; `S_UX_DEMO` (a page assembled from every widget: header
  "T3 · ACID 303", knobs CUTOFF 42 / RESO 110 / ENV MOD 88 / DECAY 31, a glow panel with meters + strip, footer
  "EDIT · TONE" / "2 / 4") renders and passes the text lint. RED.
- [ ] Step 2 — implement; GREEN; look at `build/ui_new/ACID/ux_demo.png` next to `docs/acid-fm/ui-mockups/blend-amount.html`
  option 3 and note differences in the report; commit `feat(ui): ux_widgets: header, knob row, glow panel, footer, meters, step strip`.

---

### Task 5: HOME navigation: TRACKS ↔ VISUAL, GEN kept

**Files:** `firmware/src/ui_lens.c` (`CE_HOME`, `CG_HOME`, `NAV_DEF`, `nav_cur`, `nav_go`, `nv_ok`, groups / tabs),
`ui.c` (`ui.home`, `ui.hlv`, `ui.hset`, `go_home`, `hud_home`, `home_macros`, `home_level`, `lz_fam`),
`ui_input.c` (HOME tap / hold / +SELECT, knobs on HOME), create `firmware/src/ux_nav.h/.c` (HOME's two pages as a table
+ the page-memory rule); tests: a new `tests/ux_nav_test.c` (or extend ui_test.c's navigation checks).

**Interfaces — Produces:** `enum { UXH_TRACKS, UXH_VISUAL }`, `uint32_t ux_home_page(void)`, `void ux_home_flip(void)`;
HOME tap: from another section → HOME on the page last shown there; on HOME → flip TRACKS ↔ VISUAL. HOME held → MENU
(unchanged). HOME + SELECT → GEN (JIANT's GEN pages, unchanged until piece 6), OCT− leaves GEN back to HOME's last
page. MACROS / ROUTES / ECO / BEING pages are gone from HOME; JIANT's MACRO layer (LFO held) still edits `macro_v`.
- [ ] Step 1 — failing tests: from SEQ, HOME → TRACKS (fresh) ; HOME → VISUAL; HOME → TRACKS; SEQ then HOME →
  the page last shown on HOME; HOME+SELECT → GEN page 1; OCT− from GEN → HOME's last page; HOME held → MENU;
  `nav_groups`/tabs on HOME report 2 pages. RED.
- [ ] Step 2 — implement (remove the ECO / BEING / MACROS / ROUTES entries and their `ui.hlv` / `ui.hset` logic;
  keep stored page-memory bytes' layout, a stored HOME page that no longer exists → TRACKS); GREEN; full suite.
- [ ] Step 3 — commit `feat(ui): HOME is TRACKS and VISUAL; HOME again flips; GEN kept on HOME+SELECT`.

---

### Task 6: HOME TRACKS

**Files:** create `firmware/src/ux_home.c` (TRACKS here, VISUAL in Task 7); `ui_draw.c` (route HOME's drawing to
`ux_home_draw()` instead of draw_head / draw_columns / draw_graph and HUD HOME); `ui_input.c` (HOME knobs);
delete `firmware/src/ui_lzhome.c` and its include, and HUD HOME's code paths; tests in ui_render.c (`S_HOME`,
`S_HOME_PLAY`, `S_HOME_T5`) and ui_test.c (knobs).

**Behaviour (spec §4.3, revised: ridgelines):** header `ACID FM` (gradient) left, `SECTION · BAR` (`A·3/8`) +
tempo right. Six stacked ridgelines T1 (top) … T6 (bottom) in the panel: each is its track's level history (≈ 2 s:
one sample per UI frame or per 30 ms, a ring buffer of ~112 points per track = 672 B, scrolling right → left), drawn
as a polyline over a BG fill below it (so each line hides the one behind, pulsar-plot style), baseline per row, peak
height ≈ 24 px. The four tracks the knobs control are drawn in their knob colours (`ux_knob_col`) at full brightness;
the other two in UX_MID dimmed. Each row: name at the left (`T3 ACID`, UF_LBL) and the level 0–99 at the right end
(UF_LBL). Under the lines: `ux_strip` of the selected track's current 16 steps (`seq_src`), the playing step outlined
while playing. Knobs 1–4: track levels (the track's level parameter as the mixer sets it — find it in params / GLO
MIX) of the window of four holding the selected track (T1–T4 for T1–T4 selected, T3–T6 for T5–T6). Footer
`HOME · TRACKS` / `LVL · LVL · LVL · LVL`. Level source: `ux_home.c` becomes the single reader of `trk[c].peak` (read,
smooth with fall-off into its own history ring, then clear, as ui_lzhome.c did); no other reader remains (grep).
Mockup: `docs/acid-fm/ui-mockups/home-unique.html` option 1 (`ux_meters` from Task 4 may be replaced by a
`ux_ridges()` widget here; keep `ux_meters` only if something else uses it).
- [ ] Step 1 — failing tests: render `S_HOME` (stopped: flat lines), `S_HOME_PLAY` (playing, histories with peaks,
  step 5 of an acid pattern with an accent and a slide), `S_HOME_T5` (T5 selected: knobs control T3–T6, those four
  lines in knob colours, T1–T2 dimmed); text lint passes; a knob turn on HOME changes the right track's level; the
  history scrolls and a silent track's line goes flat. RED.
- [ ] Step 2 — implement; delete ui_lzhome.c and HUD HOME; GREEN; build; record flash change (deleting should more
  than pay for TRACKS).
- [ ] Step 3 — commit `feat(ui): HOME TRACKS: six ridgelines, the selected track's steps, levels on the knobs`.

---

### Task 7: HOME VISUAL and the load rules

**Files:** `firmware/src/ux_home.c` (VISUAL), `ui_draw.c` (redraw scheduling for VISUAL), tests in ui_render.c
(`S_HOME_VISUAL`) and a load-rule unit test.

**Behaviour (spec §4.3, §5.3):** full panel: concentric rings of the blend around the centre (e.g. 6–8 ring bands,
each a colour from UX_BLEND offset by a phase), the phase advancing with the bar position (`chain.bar_pos` / tempo)
and the rings' brightness pulsing with the master level (`bg_energy()`, sampled ≤ every 100 ms); the song's name
(`chain` / project name; "ACID FM" when none) in UF_TTL and `SECTION · BAR` over it. Knobs 1–4: macros M1–M4
(`macro_v`). Drawing by precomputed ring geometry (a const table of ring radii → filled spans per row, computed once
at init into RAM ≤ 2 KiB, or drawn with `cv_rrect` rings) — no trig per pixel per frame.
**Load rules:** VISUAL redraws at most every 66 ms; every 100 ms while `load_tight`; frozen (no redraw, last frame
kept) while the overload guard sheds (`load_diag.shed` rose since the last frame) or `gov_lite`; glow level on all
pages follows the same rule (level 0 while `load_tight`).
- [ ] Step 1 — failing tests: `S_HOME_VISUAL` renders (text lint passes); a host unit test of the redraw scheduler:
  given times and flags (load_tight, shed counter rising, gov_lite) it returns the right "draw now?" decisions;
  macros change with knobs on VISUAL. RED.
- [ ] Step 2 — implement; GREEN; build; record flash; commit `feat(ui): HOME VISUAL: the kaleidoscope, macros, and the load rules`.

---

### Task 8: The wordmark splash

**Files:** `firmware/src/ui_draw.c` (splash section 1782-1936), create `firmware/src/ux_splash.c` (included where
`ui_signature.h` was), delete `firmware/src/ui_signature.h` and `tools/gen_signature.py` (and any build reference),
`tests/ui_render.c` S_SPLASH (1879-1905), `web/emu/felucca_web.c` only if a symbol changes (keep `spl_anim`,
`draw_splash`, `splash_write`, `SPL_WRITE`, `SPL_HOLD` names).

**Behaviour (spec §4.5):** total ≈ 1.5 s (`SPL_WRITE` ≈ 900 ms of animation, `SPL_HOLD` ≈ 600 ms): the blend line
(3 px) sweeps left → right at y ≈ 96; "ACID FM" in UF_MARK gradient text types on letter by letter under it;
`FELUCCA_VERSION` and "BY NORFOLKHACKER" in UF_LBL (MID) appear under the wordmark; ANIMATION OFF draws the last frame
at once. Uses `ux_text_grad` and direct LCD strips (the canvas is free at boot).
- [ ] Step 1 — failing test: S_SPLASH's collector now expects `"ACID FM|" FELUCCA_VERSION "|BY NORFOLKHACKER"`
  (adapt the `in_splash` hook to gradient text), ink pixels in the wordmark band, the line present at y≈96. RED.
- [ ] Step 2 — implement; delete the signature; GREEN; check `startup_test` still steps past the splash; build;
  commit `feat(ui): the wordmark splash; JIANT's signature removed`.

---

### Task 9: Version 0.3, docs, device check

**Files:** `firmware/src/felucca.c` (`v0.3`), README Status, `web/landing/index.html` (step 3 in progress: mark 3a's
parts done in the text), `web/manual/data.js` (0.3 news entry, ES + EN: new look, HOME TRACKS / VISUAL, wordmark
splash, one palette; the other sections restyle in 0.4 / 0.5), regenerate what tests pin to the version, `docs/acid-fm/device-log.md`,
`docs/superpowers/plans/2026-10-24-acid-fm-piece3a-deferred.txt` (items for 3b / 3c / 3d).

- [ ] Step 1 — version + docs; full suite; commit `release prep: ACID FM 0.3 (UI foundations, HOME, splash)`; tree clean.
- [ ] Step 1b (controller + user) — the no-manual check (spec §0): after the push to a branch build is not available,
  build the emulator locally if emsdk is present, otherwise use the device: the user, without the manual, (a) finds
  which track is loudest on HOME, (b) turns T5's level down from HOME, (c) flips HOME to the visual and back, (d) sees
  the splash's version. Anything they hesitate over is noted and fixed (or filed for 3b/3c) before Step 2.
- [ ] Step 2 (controller + user) — `./build.sh --release 0.3` on the clean HEAD, sha256, install, `status` over
  serial (version, `audio_late` after a power cycle), then the CPU check: CPU TEST song playing 20 s with HOME
  TRACKS, read `audio_late` / `audio_max_us`; power-cycle; the same with HOME VISUAL: VISUAL must not add late
  blocks beyond TRACKS' run. The user checks: splash, HOME TRACKS (levels move, knobs set levels, steps show), HOME
  again → VISUAL (rings move with the music, macros), the other sections still work in ACID colours, save / load.
- [ ] Step 3 — log in device-log.md, commit `docs: piece 3a device log`.
