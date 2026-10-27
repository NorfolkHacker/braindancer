# ACID FM piece 3c-2 (SAVE, MENU, dialogs, PLAYS) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ACID FM 0.6: SAVE (SOUND, SONG, PROJECT, MORE) and MENU (AUDIO, CLICK, CONTROLS, MORE) on the section engine,
ABOUT / PROFILE restyled, one full-screen dialog for every confirmation, a track-wide PLAYS condition, every tempo writer
following the external clock, the web editor's per-track MIDI page and English example names.

**Architecture:** SAVE moves like 3b / 3c-1 sections: each page is a `PAGES` row (`params.c`) in the `CE_SAVE` chain
(`ui_lens.c`), drawn by `ux_section.c` with a per-page picture from a new `ux_save.c`; its knobs keep going through the
inherited write paths (`ui_input.c`: `up_ui`, `preset_step`, `project_load`, `name_open`, the `chain_config` rows). SAVE
› MORE is a `UXM_SAVE` list whose rows are views (`ID_SV_*`) and actions (`UXM_A_*`, asking through the same `CF_*`).
Dialogs: a new `ux_dialog.c` draws `ui.confirm` full screen (header kept, page area replaced); the answer path in
`ui_input.c` is untouched. MENU keeps its own state (`ui.menu`, MI_* through `menu_get` / `menu_put`) but is laid out as
three pages + a MORE list and drawn by a new `ux_menu.c` with the ux widgets and `ux_more_draw`. PLAYS is six appended song
globals `G_PLAYS1..6` saved in an appended project block `'P' 'L'`, gated in `seq.c seq_step`. Inherited SAVE / MENU /
dialog drawing is deleted as each part moves.

**Tech Stack:** C (JieLi clang / host gcc via `tools/host-cc`), Python generators and checks, Node for web tests.

**Spec:** `docs/superpowers/specs/2026-10-27-acid-fm-ui-3c2-save-menu-dialogs.md` (authority for every page table and
rule); parents `docs/superpowers/specs/2026-10-24-acid-fm-ui-design.md` (§0, §3, §4.1 the seven rules, §5, §6),
`docs/superpowers/specs/2026-10-25-acid-fm-ui-3b-sound-sections.md` (the section / MORE engine),
`docs/superpowers/specs/2026-10-26-acid-fm-ui-3c1-seq-glo-midi.md` (the engine on SEQ / GLO, held layers, MIDI page);
backlog `docs/superpowers/plans/2026-10-26-acid-fm-piece3c1-deferred.txt` ([3c-2] items).

## Global Constraints

- GPL-3.0-only; never remove JIANT/Felucca copyright lines; changed files gain `Portions Copyright (C) 2026
  NorfolkHacker`; new files `SPDX-License-Identifier: GPL-3.0-only` + `Copyright (C) 2026 NorfolkHacker`.
- Spec §1: "Setting ids, menu item ids (`MI_*`), the editor protocol's existing messages, MIDI CCs and the project /
  preset / settings storage formats do not change, except the one appended field of §5.1. Sound goldens unchanged."
- Spec §6: "Storage append-only: only PLAYS (§5.1) is new. Projects, user presets and settings from 0.5 load and behave
  as before; 0.5 reading a 0.6 project ignores PLAYS." Appended blocks follow the 0.5 `'M' 'R'` pattern (`project.c`
  `proj_mr_used` / `proj_packed_size` / `proj_pack` / `proj_unpack_c`); NEW PROJECT (`system_init`) keeps what `KEEP[]`
  and the MIDI routing keep and resets PLAYS.
- Spec §6: "No audio-interrupt work changes except the PLAYS gate in the sequencer (a compare per step) and the clock
  rule; variables an interrupt reaches stay pinned (`tools/build.py` guard); budgets (ISR 300, TIMER5 2170, slicer 821)
  unchanged." Target budgets (`tests/target_budget.py`, `tests/target_budget.txt`) unchanged unless a task explains why.
- Image at HEAD c42b523 (measured): 554,884 B of 584,336 (29,452 B free); spec §6: floor **16 KiB free** ("the deleted
  SAVE / MENU / dialog drawing pays for the new pages"); RAM .data+.bss 80,832 of 98,304; pool spare 9,920 B (**no new
  `.pool` data**); tables in `.rodata`; DC cells per page ≤ 1,920.
- Every new `.c` is added to `firmware/src/felucca.c` (after the file whose statics it uses), `tests/ui_test.c`'s include
  list, `web/emu/felucca_web.c` and `tools/size_fns.py` `SIZE_FILES`.
- Every status / label / MORE row / dialog literal passes the glyph scan (`tests/ux_msg_scan.py`, extended to each new
  literal table); values fit their knob column (`tests/ux_cover_test.c` width sweep).
- English only on the device and in the manual (the ES lyrics bank is a feature and stays). Plain words (UI spec §0 rule
  4), one short form per word. User-facing text says ACID FM, "built on JIANT FM and Felucca"; JIANT named only in the
  credits; no band names on the device (RETROBOC is the user's own genre name and stays).
- The section rules (UI spec §4.1): **3 pages + MORE**; the section's button opens the page last used and steps on;
  SELECT walks the pages and scrolls MORE; PRESETS loads presets, never a page; ALGORITHM picks the track; **every key
  keeps its job**; **OCT+ yes / OCT− no**; the footer `SECTION · PAGE   n / N · NEXT PAGE` counts **every shown page
  including MORE**.
- Knob colours K1 ACID, K2 SLIDE, K3 VIOLET, K4 ACCENT; `ux_knob_touch` on every knob turn; pictures paced with `ux_pace`
  / `ux_pace_at` (redrawn on a change, never every frame idle).
- Environment (venv required): `cd ~/synth/mvavefirmware && . .venv/bin/activate && export PATH=$PWD/.venv/bin:$PWD/build/node/bin:$PATH JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK`;
  build `./build.sh`; suite `CC=tools/host-cc sh tests/run_tests.sh`; renders `build/ui_new/ACID/`; a CHAINS change also
  needs `python3 tools/gen_nav_map.py` (the suite runs `--check`) and `python3 tools/gen_manual.py build/ui_new/ACID`.
- Process: **foreground commands only** (Bash timeout ≤ 600000 ms); never start background wait loops (no `until pgrep`
  / `while pgrep`); stop anything you start before reporting; never leave HEAD detached.
- Branch `piece3c2-ui`; commits end with a blank line + `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`;
  never on main, never push; no device or serial access (Task 10 step 2 is the controller's with the user).

## Pre-flight notes (spec points the code makes impossible or ambiguous; controller rules before Task 1)

Each with the plan's default, which the tasks below implement unless the controller rules otherwise.

1. **CF_DICE_ALL is GEN, not a dialog.** `ui_lens.c gen_on()` treats `ui.confirm == CF_DICE_ALL` as GEN's screen
   (`dlg_on()` excludes it). Default: GEN stays as it is (spec §9), so `ux_dialog` draws the other 21 kinds; the
   coverage test lists CF_DICE_ALL as "GEN's screen".
2. **CF_CLEAR_TRK is never opened** (only `ui_draw.c confirm_text` names it). Default: keep the enum value, give it a
   `uxd_text` entry and a render scene (set `ui.confirm` directly), no new opener.
3. **"Project" vs section.** In the code a project is one of four songs (`song_cur`, `NSONG` 4, `song_open`) holding four
   sections A–D (`proj_slot[4]`, picked by `G_SLOT`); LOAD / SAVE / `CF_OVR_PROJ` / NAME act on a **section**
   (`project_load(G_SLOT-1)`, `project_save`, `NK_PROJ_*` "PROJECT A"). Default: PROJECT's KNOB 1 = the project
   (`song_open`, the inherited KNOB 1), LOAD / SAVE = the inherited `G_LOAD` / `G_SAVE` on the section SONG's SECTION
   picks (named in PROJECT's status line: `PROJECT 2 · SECTION B · OCT+ SAVES`), NAME = `name_rename` of that section.
4. **No "load dialog" exists**: `act_do` `G_LOAD` loads at once (with an undo mark). Default: append `CF_LOAD_PROJ` after
   `CF_LIVE_COMMIT` (RAM-only enum) and ask only when the music has unsaved changes (`project_unsaved()`, Task 4).
5. **User slots**: `UP_SLOTS` is 32, the spec says 16. Default: SLOT 1–32, the list scrolls (storage unchanged).
6. **Red vs accent**: `UX_ACCENT` is the red (RGB FF 50 3C, the `STOP TO EDIT` colour). Default: something lost →
   `UX_ACCENT`; nothing lost → `UX_ACID`.
7. **Glyphs**: `UF_VAL` (the large face) has no `?` or `'`, `UF_HDR` no `?` (`tools/gen_aa_font.py UF_TEXT / UF_HDR`).
   Default: add `?'` to `UF_TEXT` and `?` to `UF_HDR` (Task 1); questions avoid `( ) ,`.
8. **SONG's SECTION knob**: the inherited ARRANGE's KNOB 2 sets the picked row's section, SECTIONS' KNOB 1 sets `G_SLOT`.
   Default: SECTION writes `G_SLOT` and, when ROW is a real row, that row's section too (one knob, both inherited
   writes); "left of A deletes the row" goes (DELETE ROW is a MORE row).
9. **OCT+ on SONG**: the code appends after the last row (`act_do` GR_SONG); the spec says "after the picked one".
   Default: insert after the picked row (its scene slots shift as `CF_DEL_ROW` shifts them); identical when the last is
   picked. ROW covers the real rows only; END moves to SAVE › MORE › SONG.
10. **SONG's "inherited section settings that had no page"**: none found besides END. Default: SONG group = END, DELETE
    ROW, CLEAR SONG.
11. **MENU is not a PAGES / CHAINS section**: MI_* rows are read and written by `menu_get` / `menu_put`, not
    `param_ref`. Default: keep `ui.menu` (HOME held opens, HOME closes), lay MENU out as three pages + a MORE list in
    `menu_items.c`, draw it with the ux widgets and `ux_more_draw` (new row kind `UXR_MENU`) in a new `ux_menu.c`.
12. **MENU_DESC's tab** (editor protocol: tab index + name): default: the item's new page (0 AUDIO, 1 CLICK, 2 CONTROLS,
    3 MORE; names ≤ 12 chars); message format unchanged, `web/EDITOR_PROTOCOL.md` says the grouping changed in 0.6.
13. **MENU duplicates**: SPEAKER EQ is `MI_LOWCUT` (AUDIO's "LOW CUT"); CALIBRATION is `MI_PANEL` (CONTROLS' "PANEL");
    "SERIAL" is a value of `MI_SERIAL` (named USB AUDIO). Default: one place each — AUDIO KNOB 1 SPEAKER EQ, CONTROLS
    KNOB 4 CALIBRATION, MORE › USB = USB AUDIO, USB LEVEL.
14. **MENU › SYSTEM › INIT** does not exist (INIT SYSTEM went in 0.9.18; SAVE › MORE › NEW PROJECT does it). Default:
    no INIT row (one place).
15. **PLAYS storage**: a new `P_*` would move user presets, the sound dice, MUTATE and the P map. Default: six song
    globals `G_PLAYS1..6` (ids 90–95, appended after `G_MRSEL`) in an appended `'P' 'L'` block written only when one is
    not ALWAYS; `G_COUNT` 90 → 96 (editor `knownLayout` and web tests follow); undo through the project snapshot.
16. **"As when muted"**: a muted track still applies its locks and automation (`seq.c` `motion_step` runs, `voice.c
    trk_note_on` drops the note). Default: the PLAYS gate is in `seq_step` (no notes, hits, ratchets, components) and
    the tick's `motion_step` keeps its roll, so locks / automation apply as when muted; keys still play the track.
17. **CPU TEST and CLOCK**: UNDO already restores `G_CLOCK` (it is in the snapshot); NEW PROJECT keeps `G_CLOCK`
    (`KEEP[]`), so a replaced test would stay on INT. Default: `cpt_clk` (previous CLOCK + 1) restored by
    `cpt_clock_back()` in `system_init` (before `KEEP`), `gen_go`, `project_load`, `section_load`, `undo_do`.
18. **Editor BPM refusal**: `ED_SET` always replies with the value. Default: `tempo_ext()` (the device's message) and
    the reply carries the unchanged BPM; documented in `web/EDITOR_PROTOCOL.md`.
19. **NAME screen footer says "LENTE"** (`ui_name.c:433`, Spanish, on the device). Default: fixed in Task 9 with the
    English check (`NAME · OCT- CANCEL`).
20. **Task race** (`tests/ui_test.c` ~11980): the SAVE chain and the new SEQ › MORE row change action counts. Default:
    the tasks that change them update the race's expected counts with the reason in a comment.

## Codebase map (inventory 2026-10-27 at c42b523; verify line numbers)

- Chains `ui_lens.c:118-124` (`CE_SAVE` USER PRESETS / SECTIONS ARRANGE / PROJECTS TOOLS, groups SOUND PROJECT LIBRARY,
  levels `SV_LIB SV_PROJ SV_SOUND`); `save_level()` `:210`; `gen_on` / `dlg_on` `:137-138`; `SEC_SAVE` = 9.
- PAGES SAVE rows `params.c:860-864` (`PRESETS` GR_BROWSE, `USER` GR_USER, `PROJECT` GR_SLOTS {0xFF, G_SLOT, G_LOAD,
  G_SAVE}, `ARRANGE` GR_SONG, `TOOLS` GR_TOOLS); SEQ's `SONG` GR_SONG `:885`; GR enum `params.c:533`; views
  `ID_SQ_*` 0xA0..0xBA `:615`, `ID_FO_*` 0xC0, `ID_DX_*` 0xD0..0xE6 (`ID_DX_END` 0xE7), `ID_MO_*` 0xF2, `ID_LV_*` 0xF5;
  `param_ref` / `page_id` `:944-1030`; `UXM_SEQ_ROWS` `:786` (TRACK group `P_POFS P_TCHAN P_HVEL P_HRQ`).
- SAVE drawing: `ui_lzsave.c` (whole file: `graph_lib graph_lz_tools graph_proj lz_track_card graph_sound
  graph_lz_presets graph_save_lz`, included by `ui_graph.c:703`), `ui_graph.c:539-600` (GR_* sigs), `ui_draw.c:1241`
  (SECTIONS knob row), `ui.c:520,808,1820-1880` (`act_cols act_col act_name act_ready` for GR_TOOLS GR_SONG GR_USER).
- SAVE input: `ui_input.c` knob path `:1225-1325` (GR_SONG rows / scenes / END, GR_BROWSE, GR_TOOLS, GR_SLOTS, GR_USER),
  `act_do` `:1340-1430` (TOOLS' six: CLEAR SEQ, INIT SOUND, DELETE ROW, CLEAR SONG, NEW PROJECT, CPU TEST; ADD ROW;
  USER LOAD ERASE SAVE; G_LOAD G_SAVE); `preset_step` / `preset_mark` `ui.c:1763,1647`; `list_get` / `list_put`
  `ui.c:74`; `up_ui` `upreset.c:565`, `up_name`, `up_used`, `up_rec(k)->engine`, `UP_SLOTS` 32 `core.h:51`; songs
  `song_main.c` (`song_open`, `song_select`, `section_load`, `section_store`, `song_idx`), `song_chain.c song_end_step`,
  `ui.c:726 song_end_row`; NAME `ui_name.c` (`name_open`, `name_rename` `:121` keyed on GR_USER / GR_SLOTS).
- SAVE layer `ui_layer.c:399-445` (`song_key`: F3–B3 play, C4–F4 store, C5 NAME, D5 LOOP/SONG, E5 SONG REC, G5 the first
  GR_SONG page), KNOB 1 the song `:679`, SAVE + OCT± store / recall `:741-748`, overlay `:969-1050`, state `:1147`;
  `ux_layer.c ux_layer_owns` `:14`.
- Dialogs: enum `ui.c:350-353`; `confirm_open` `ui_input.c:545`; answers `ui_input.c:2330-2460` (undo mark except OVR
  PROJ / OVR USER / ERASE USER / DICE / DEL ROW / CLEAR SONG); drawing `ui_draw.c:1390-1600` (`confirm_text`,
  `fill_around`, `dlg_tabs`, `dlg_crumb`, `dlg_card`, `dlg_button`, `draw_confirm`) and routing `ui_draw.c:1648-1700`
  (`dlg_notice`, `scr_holes`, `dlg_behind`); dice `dice_panel` `ui_input.c:1811` (240x148 at `LZ_CARD_Y`), `dk_back`
  `:1847`, `part_dice` `:1904`, `dice_kind` `:1930`. Openers: CLEAR SEQ / INIT SOUND / DEL ROW / CLEAR SONG / NEW
  PROJECT / CPU TEST (TOOLS), DEL ROW (ARRANGE left of A), LOAD PAT (PHRASES), ERASE / OVR USER (USER), OVR PROJ
  (G_SAVE), CLEAR ALL (SEQ + REC `:2491`), CLEAR PFX / CLEAR MOTION (`ux_more.c UXM_ACT`), INIT / RECALL SOUND (EDIT
  layer `ui_layer.c:579`), LIVE COMMIT (`ui_live.c:261`), MUT BACK (`project.c:2153`), DICE * (held + SELECT `:1922`).
- MENU: `menu_items.c` (`MI_*`, `MI_NAME`, `MTAB_*`, `MPAGE[8]`, `menu_page_of`, `mi_tab`, `menu_get/put/step/n/vname`,
  `menu_hidden`, `menu_valued`); `ui_menu.c` (`menu_document` ABOUT `:229`, `prof_document` PROFILE `:193`, `cpt_*`,
  `MI_HELP`, `menu_short`, `menu_cards`, `menu_panel`, `draw_menu` `:441`, `menu_input` `:543`); `ui_draw.c:250,896,965`
  (MENU header / tabs / footer); `editor_menu.c:72` (MENU_DESC tab); HOME held opens `ui_input.c:2288-2296`.
- PLAYS: `seq.c` `t->loops` (`core.h:645`; `++` at `seq.c:2216`), `cp_trig_cond` `:1337`, `CP_TRIGS` `core.h:546`,
  `step_rolls` `:1369`, `seq_step` first gate `:1920`, tick `motion_step` `:2229-2231`; words `ux_seq.c UXQ_COND`
  `:55`; SEQ › MORE views `ui_input.c seq_view_turn` `:1054`, `ux_seq.c uxq_vtext` `:565`.
- Clock: `ui_input.c tempo_ext` `:1119`; GEN `gen.c:2480-2484` (`gen_go` `:2538`); CPU TEST `song_dice.c:89-94`
  (`cpu_test_song`, `cpt_arm`); demo `ui.c:1564` (`power_on_parts`); editor `editor.c:438-462` (`ED_SET`); header BPM /
  sync mark `ui_draw.c:360-390`.
- Editor: `web/editor.html` globals table `:1540-1566` (G 71–89), `knownLayout` `:2754`, `LAYOUT` `:2758-2780`, `G_SKIP`
  `:2782`; `web/test_web.mjs` (gcount 90 at `:373,393`; mock tables from `tests/descdump.c` / `tests/editor_test.c`).
- Names: `tests/ui_render.c:1817-1872` (NOCHE LARGA, NOCHE B / C, PAD NUBE, lyric LA NOCHE); `tests/built_on_test.py`.
- Tests: `tests/run_tests.sh`; `tests/ui_test.c` (helpers `ui_power_on frame frames press turn check uxq_t_is
  uxq_t_say msg_is go_title nav_put lay_combo white key_up btn_up ux_px host_screen`; MENU tests `test_menu_tabs`
  `:2501`, `test_save` `:1565`, `test_save_lens` `:6873`, `test_project_page` `:7893`, `test_t38_menu` `:19479`, the race
  `:11980`); `tests/ui_render.c` scenes `S_*` `:620-661`, names `:671`; `tests/ux_cover_test.c`; `tests/ux_msg_scan.py`;
  `tests/ratchet_test.c` (sequencer test pattern); `tools/gen_manual.py PAGE_SHOT` `:31`.

## File map

| File | Change |
|---|---|
| `firmware/src/ux_dialog.c` (new) | every confirmation drawn full screen (Tasks 1, 1b) |
| `firmware/src/ux_save.c`, `ux_save.h` (new) | SAVE's knobs, pictures, status (Tasks 2–4) |
| `firmware/src/ux_menu.c` (new) | MENU's pages, MORE, ABOUT / PROFILE frame (Task 5) |
| `firmware/src/params.c`, `ui_lens.c`, `ux_more.c`, `ux_more.h`, `ux_section.c`, `ux_section.h`, `ui_input.c`, `ui.c` | pages, chain, MORE rows, actions, input |
| `firmware/src/ui_draw.c`, `ui_graph.c`, `ui_lzsave.c` (deleted), `ui_menu.c`, `menu_items.c`, `editor_menu.c` | inherited drawing removed; MENU tables |
| `firmware/src/ui_layer.c`, `ux_layer.c` | SAVE layer (Task 4) |
| `firmware/src/core.h`, `project.c`, `seq.c`, `ux_seq.c` | PLAYS (Task 6) |
| `firmware/src/gen.c`, `song_dice.c`, `ui.c`, `editor.c` | clock rule (Task 7) |
| `tools/gen_aa_font.py`, `tools/gen_manual.py`, `docs/navigation-map.svg` (regenerated) | glyphs; manual map |
| `web/editor.html`, `web/EDITOR_PROTOCOL.md`, `web/test_web.mjs` | PLAYS globals (6), MIDI page (8) |
| tests: `tests/ui_test.c`, `tests/ui_render.c`, `tests/ux_cover_test.c`, `tests/ux_msg_scan.py`, `tests/built_on_test.py`, new `tests/plays_test.c`, `tests/editor_test.c`, `tests/run_tests.sh` | per task |
| `firmware/src/felucca.c`, `README.md`, `web/landing/index.html`, `web/manual/data.js`, `docs/acid-fm/device-log.md`, new `docs/superpowers/plans/2026-10-27-acid-fm-piece3c2-deferred.txt` | Task 10 |

---

### Task 1: `ux_dialog` and the NOTICE dialogs

**Files:** create `firmware/src/ux_dialog.c`; modify `firmware/src/felucca.c` (include after `ux_layer.c`),
`tests/ui_test.c` (include), `web/emu/felucca_web.c`, `tools/size_fns.py`, `firmware/src/ui_draw.c` (routing),
`tools/gen_aa_font.py` (`UF_TEXT += "?'"`, `UF_HDR += "?"`), `tests/ux_msg_scan.py` (the `UXD_*` tables),
`tests/ui_render.c` (scenes), `tests/ui_test.c` (tests).

**Interfaces — Produces:**
```c
/* one confirmation's words: action word, question (UF_VAL, up to 2 lines of 224 px), what is lost and how to get it back
 * (UF_HDR, up to 2 lines), OCT+'s verb, OCT-'s word ("NO"; MUT BACK "KEEP"), loses: red (UX_ACCENT) else UX_ACID */
typedef struct { const char *word, *verb, *no; char q[40], lost[72]; uint8_t loses; } uxd_text_t;
static void uxd_text(uint32_t kind, uint32_t k, uxd_text_t *t);     /* kind a CF_*, k ui.confirm_trk */
static int ux_dialog_owns(uint32_t kind);                           /* every CF_* but CF_NONE and CF_DICE_ALL */
static void ux_dialog_draw(void);                                   /* ui.confirm full screen: header, page area, status, footer */
static void uxd_foot(char *left, uint32_t nl, char *right, uint32_t nr);  /* "SEQ · CLEAR", "OCT+ YES · OCT- NO" */
static uint32_t uxd_notes(const track_t *t);                        /* notes + hits on the track's P_SLEN steps */
static uint32_t uxd_draws;                                          /* frames ux_dialog_draw drew (tests) */
```

Spec §4. Layout A: `ux_header` (track, transport) kept; page area (`UX_Y_KNOBS`..`UX_Y_STATUS`) BG with: the action word
(UF_LBL, accent / red), the question (UF_VAL), the lost line (UF_HDR, UX_MID), two key cells `OCT−` + no-word left,
`OCT+` + verb right; footer `<SECTION> · <WORD>` / `OCT+ YES · OCT- NO`. Words per kind (k is `ui.confirm_trk`; counts
from `uxd_notes`, `motion_count`, `motion_lock_count`, `chain_config`):

| kind | word | question | lost line | verb | lost |
|---|---|---|---|---|---|
| CLEAR_SEQ | CLEAR | `CLEAR T3'S SEQUENCE?` | `Loses 14 notes and 3 locks. GLO + G4 brings it back.` | CLEAR | y |
| CLEAR_TRK | CLEAR | `CLEAR TRACK 3?` | as CLEAR_SEQ | CLEAR | y |
| CLEAR_ALL | CLEAR | `CLEAR EVERY SEQUENCE?` | `Loses 120 notes on 6 tracks and the punch lane. GLO + G4 brings it back.` | CLEAR | y |
| CLEAR_MOTION | CLEAR | `CLEAR T3'S AUTOMATION?` | `Loses 9 records and 3 locks. GLO + G4 brings them back.` | CLEAR | y |
| CLEAR_PFX | CLEAR | `CLEAR THE PUNCH LANE?` | `Loses the lane's punch-ins. GLO + G4 brings it back.` | CLEAR | y |
| CLEAR_SONG | CLEAR | `CLEAR THE SONG?` | `Loses 6 rows, 120 bars. There is no undo for this.` | CLEAR | y |
| DEL_ROW | DELETE | `DELETE ROW 3 · B16?` | `Its scene goes with it. There is no undo for this.` | DELETE | y |
| INIT_SOUND | INIT | `INIT T3'S SOUND?` (DRUM: `INIT T3'S SOUND AND KIT?`) | `The steps stay. GLO + G4 brings the sound back.` | INIT | y |
| INIT_SYS | NEW | `START A NEW PROJECT?` | `Clears the music in memory; stored sections stay. GLO + G4 brings it back.` | NEW | y |
| CPU_TEST | RUN | `BUILD THE CPU TEST SONG?` | `Plays at 160 BPM on the internal clock. GLO + G4 brings the music back.` | RUN | y |
| LIVE_COMMIT | KEEP | `KEEP THE LIVE TRICKS?` | `Writes them into the steps. GLO + G4 takes them out.` | KEEP | n |
| MUT_BACK | BACK | `T3 BACK TO BEFORE MUTATE?` | `Loses what MUTATE changed in its sound and pattern.` | BACK (no-word KEEP) | y |
| LOAD_PAT | LOAD | `REPLACE T3'S SEQUENCE?` | `Loads <pattern>. Loses 14 notes. GLO + G4 brings them back.` | LOAD | y |
| OVR_PROJ | SAVE OVER | `SAVE OVER SECTION B?` | `Replaces <its name>. There is no undo for this.` | SAVE OVER | y |
| OVR_USER | SAVE OVER | `SAVE OVER U07?` | `Replaces <its name>. There is no undo for this.` | SAVE OVER | y |
| ERASE_USER | ERASE | `ERASE U07?` | `Loses <its name>. There is no undo for this.` | ERASE | y |
| RECALL_SOUND | RECALL | `RECALL T3'S STORED SOUND?` (DRUM: `.. SOUND AND KIT?`) | `Loses the changes since the section was stored. GLO + G4 brings them back.` | RECALL | y |

Where a count is 0 or not cheap, the line names the thing without a number (`Loses its notes.`). Names in `<..>` come from
`project_name`, `up_name`, `pat_label` (text_fit to the line).

- [ ] Step 1 — failing tests (`tests/ui_test.c`, registered after `test_c10_echo_left`):
  - `test_u1_kinds`: for every kind of the table: `ui.confirm = kind; ui.confirm_trk = 2; ui.force = 1; frame();` →
    `ux_dialog_owns(kind)`, `uxd_draws` grew, `uxd_foot` right is `"OCT+ YES \xB7 OCT- NO"` and left
    `"<nav_sec name> \xB7 <word>"`, the question splits into ≤ 2 lines of ≤ 224 px in `UF_VAL`, the lost line ≤ 2 lines in
    `UF_HDR`, every glyph present in its face (`text_w` of each char > 0); the word's pixels are `UX_ACCENT` when `loses`,
    `UX_ACID` otherwise (`ux_px` sample inside the word's ink box). `ux_dialog_owns(CF_DICE_ALL) == 0`.
  - `test_u1_counts`: T3 with 14 notes over 16 steps (`step_t` n / hits) and 3 locks (`motion_set_lock`) → CLEAR_SEQ's
    lost line is exactly `"Loses 14 notes and 3 locks. GLO + G4 brings it back."`; with 6 rows of 120 bars CLEAR_SONG's
    `"Loses 6 rows, 120 bars. There is no undo for this."`.
  - `test_u1_full`: SEQ STEPS open with notes, `confirm_open(CF_CLEAR_SEQ, 2)` → no roll / strip colour (`UX_ACID`
    step cells) remains in rows `UX_Y_KNOBS`..`UX_Y_STATUS` outside the dialog's own ink; the header rows equal the page's
    header (same `ux_header` call); `frames(50)` idle → `uxd_draws` grows by ≤ 1 (paced: drawn on a change only).
  - `test_u1_answers` (behaviour unchanged; each from a fresh `ui_power_on()`): CLEAR_SEQ OCT+ → T3's steps default and
    `undo_ok`, OCT− → steps `memcmp`-equal to before; CLEAR_ALL OCT+ → every track cleared, pfx lane zero; CLEAR_MOTION
    OCT+ → `motion_count(T3) == 0`; CLEAR_PFX; DEL_ROW (row 1 of 3 gone, scenes shifted); CLEAR_SONG; INIT_SOUND;
    INIT_SYS (`NEW PROJECT` message); CPU_TEST (`cpt_arm`, BPM 160); LIVE_COMMIT; MUT_BACK OCT− → `KEPT AS IT IS`;
    LOAD_PAT; OVR_PROJ / OVR_USER OCT+ → `name_on()`; ERASE_USER → `!up_used(7)`; RECALL_SOUND → `SOUND RECALLED`.
  - `tests/ui_render.c`: scenes `S_DLG_CLEAR_SEQ S_DLG_CLEAR_TRK S_DLG_CLEAR_ALL S_DLG_CLEAR_MOTION S_DLG_CLEAR_PFX
    S_DLG_CLEAR_SONG S_DLG_DEL_ROW S_DLG_INIT_SOUND S_DLG_INIT_SOUND_DRUM S_DLG_INIT_SYS S_DLG_CPU_TEST S_DLG_LIVE_COMMIT
    S_DLG_LOAD_PAT S_DLG_OVR_PROJ S_DLG_OVR_USER S_DLG_ERASE_USER S_DLG_RECALL_SOUND` (names `dlg_clear_seq` …; the
    existing `S_CONFIRM_*` and `S_DLG_MUT_BACK` become these), lint-clean, longest names (`A VERY LONG SOUND NAME`).
  RED (no `ux_dialog.c`).
- [ ] Step 2 — implement `ux_dialog.c`; in `ui_draw.c` route `dlg_on() && ux_dialog_owns(ui.confirm)` to
  `ux_dialog_draw()` before the layer / page paths in `ui_draw_body` and drop the NOTICE-over-page path (`dlg_notice`,
  `scr_holes(R, 3)`, `dlg_behind`) from `ui_draw_page`; the dice kinds still use `draw_confirm` until Task 1b.
  Regenerate fonts (`./build.sh`), extend `ux_msg_scan.py` to `UXD_*`. GREEN; suite; commit
  `feat(ui): one full-screen dialog for every confirmation`.

---

### Task 1b: the dice dialogs; the inherited dialog drawing deleted

**Files:** `firmware/src/ux_dialog.c`, `firmware/src/ui_input.c` (`dice_panel` → `dice_panel_body`), `firmware/src/ui_draw.c`
(delete `confirm_text`, `dlg_tabs`, `dlg_crumb`, `dlg_card`, `dlg_button`, `draw_confirm`, `dlg_notice`, `DLG_*`, and
`fill_around` if no caller remains), `tests/ui_render.c`, `tests/ui_test.c`.

**Interfaces — Consumes:** `uxd_text`, `ux_dialog_draw`, `uxd_foot` (Task 1). **Produces:**
```c
static void dice_panel_body(void);   /* BEFORE / NOW into the current canvas (ux_panel_begin .. ux_panel_end, 240 x 150) */
```

Spec §4 "The dice dialogs keep their BEFORE / NOW comparison in the page area and use the same action word, footer and
keys": DICE_SOUND / SEQ / LFO / FX: word DICE, question `KEEP THE DICED SOUND?` (`KIT`, `SEQUENCE`, `LFO`, `EFFECTS`),
lost line `OCT- puts it back. SELECT dices again.`, verb KEEP, not lost (accent).

- [ ] Step 1 — failing tests: `test_u1_dice`: each dice kind via `part_dice(kind)` from its held + SELECT (EDIT, SEQ,
  LFO, FX) → `ux_dialog_owns`, the panel area shows BEFORE / NOW (pixel probe of both column captions), footer as Task 1;
  SELECT again → `part_dice` ran (state differs), OCT− → `dk_back` message (`SOUND BACK` …), OCT+ → `dk.msg`;
  `test_u1_all`: every CF_* in `CF_CLEAR_SEQ..CF_LIVE_COMMIT` but CF_DICE_ALL increments `uxd_draws` when shown and no
  inherited dialog function is linked (`ui_draw.c` has no `draw_confirm`: compile-level, the test file stops referencing
  it); renders `S_DLG_DICE_SOUND S_DLG_DICE_KIT S_DLG_DICE_SEQ S_DLG_DICE_LFO S_DLG_DICE_FX` lint-clean. RED.
- [ ] Step 2 — implement; delete the inherited dialog drawing; GREEN; suite; size noted in the report; commit
  `feat(ui): dice dialogs in the full-screen layout; inherited dialog drawing gone`.

---

### Task 2: SAVE SOUND (and SAVE MORE's SOUND group)

**Files:** create `firmware/src/ux_save.c`, `ux_save.h` (included by `ux_section.h` like `ux_glo.h`); modify
`params.c` (GR enum `GR_SVSND, GR_SVSONG, GR_SVPROJ` after `GR_MIDI`; PAGES rows `{"SAVE SOUND", FAM_SAVE, SC_GLOBAL,
GR_SVSND, {0xFF, 0xFF, 0xFF, 0xFF}}`, `{"SAVE MORE", FAM_SAVE, SC_GLOBAL, GR_MORE, {0xFF..}, &UXM_SAVE}`; views
`ID_SV_*`; `UXM_SAVE_ROWS`), `ui_lens.c` (`CE_SAVE`), `ux_section.h` (`UXS_SEC_SAVE 9u` in `UXS_OWN`, `ux_page_owns`
SAVE branch), `ux_section.c` (`uxs_knobs`, `uxs_picture`, sig, status via `uxv_*`), `ux_more.h/.c` (headings
`UXM_H_SOUND`, `UXM_H_TOOLS`, actions `UXM_A_ULOAD`, `UXM_A_ERASE`, `uxm_act_t.arg`), `ui_input.c` (knob + OCT+ for GR_SVSND; `param_knob` views),
`ui.c` (`act_cols`, `act_col`, `act_name`, `act_ready` for GR_SVSND), `ui_name.c` (`name_rename` on GR_SVSND),
`ui_layer.c` (C5 NAME on GR_SVSND), `tests/ui_test.c`, `tests/ui_render.c`, `tests/ux_cover_test.c`,
`docs/navigation-map.svg` (regenerated).

**Interfaces — Produces:**
```c
/* ux_save.h */
static int uxv_owns(void);                               /* the SAVE page shown is drawn by ux_section.c (SOUND, SONG, PROJECT, MORE) */
static int uxv_knobs(const page_t *pg, ux_knob_t k[4]);  /* SOUND: SLOT PRESET NAME SAVE (1: filled) */
static void uxv_picture(const page_t *pg);
static uint32_t uxv_sig(const page_t *pg);               /* what the picture is drawn from */
static int uxv_say(const page_t *pg, char *b, uint32_t nb);   /* the status line ("U07 · DEEP SUB · OCT+ SAVES OVER IT") */
static uint16_t uxv_eng_col(uint32_t e);                 /* an engine's colour: ux_knob_col(engine's place in ENGINE_ORDER % 4) */
/* params.c: SAVE > MORE's views (no value of their own; param_ref reads into sv_ui, ui_input.c sv_view_turn writes) */
enum { ID_SV_ENGINE = 0xE7, ID_SV_FAV, ID_SV_LIST, ID_SV_END, ID_SV_N };   /* (ID_DX_END is 0xE7) */
typedef char svv_ids_ok[(uint32_t)ID_SV_N <= (uint32_t)ID_MO_NOTES ? 1 : -1];
static int svv_is(uint32_t id);
/* ux_more.h */
enum { UXA_SEL, UXA_USLOT, UXA_ROW };                    /* an action's target: song.sel, ui.uslot, ui.song_row */
/* uxm_act_t becomes { const char *name; uint8_t ask, arg; } */
/* ui_input.c */
static void save_knob(const page_t *pg, uint32_t slot, int32_t steps);   /* SAVE's pages' knobs */
static void save_oct(const page_t *pg);                                    /* OCT+ on SAVE's pages (act_do) */
static void sv_view_turn(uint32_t id, int32_t steps);
```
`CE_SAVE` after this task: `NV("SAVE SOUND", "SOUND", 0, 0)`, the inherited `NV("PROJECT", "SECTIONS", 1, SV_PROJ)`,
`NV("ARRANGE", 0, 1, SV_PROJ)`, `NV("PROJECT", "PROJECTS", 2, SV_LIB)`, `NV("TOOLS", 0, 2, SV_LIB)` (until Tasks 3–4),
`NV("SAVE MORE", "MORE", 3, 0)`; `CG_SAVE` SOUND, PROJECT, LIBRARY, MORE. `UXM_SAVE_ROWS` now: `{UXM_H_TOOLS, UXR_HEAD}`,
`{UXM_A_ERASE, UXR_ACTION}` ("ERASE USER SLOT": CF_ERASE_USER, arg UXA_USLOT: the inherited USER column 2; Task 4 adds
the other tools above it), `{UXM_H_SOUND, UXR_HEAD}`, `{UXM_A_ULOAD, UXR_ACTION}` ("LOAD SLOT": `up_ui(0, ui.uslot)`),
`{ID_SV_ENGINE, UXR_GLOBAL}` (`select_engine(eng_step(..))`, the inherited PRESETS KNOB 2), `{ID_SV_FAV, UXR_GLOBAL}`
(`preset_mark`, KNOB 3), `{ID_SV_LIST, UXR_GLOBAL}` (`list_put`, KNOB 4). Existing `UXM_ACT` entries get `arg` UXA_SEL.

Spec §2.1: KNOB 1 SLOT (`ui.uslot` 0..31), 2 PRESET (`preset_step`, the inherited PRESETS KNOB 1), 3 NAME (a turn right:
`name_rename()` of the slot; an empty slot: `EMPTY SLOT`), 4 SAVE (action knob: right arms `ui.act`, left drops); OCT+
with SAVE armed: playing → `STOP TO SAVE`; used → `confirm_open(CF_OVR_USER, ui.uslot)`; else `name_open(NK_USER_SAVE,
ui.uslot)` (exactly the inherited GR_USER column 3). Picture: the track's sound named on top (`sound_name`), the 32
slots as a scrolling list (8 rows), each with its engine colour (`uxv_eng_col(up_rec(k)->engine)`) and name (empty: slot
label `U07` dim), the picked slot lit.

- [ ] Step 1 — failing tests:
  - `test_u2_nav`: SAVE opens on SOUND; footer `1 / 6 · NEXT SECTIONS` (the inherited pages counted until Task 4);
    `uxv_owns()` on SOUND and MORE, not on SECTIONS.
  - `test_u2_knobs`: KNOB 1 moves `ui.uslot` (clamped 0..31); KNOB 2 changes the preset exactly as `preset_step(+1)`
    does on a copy of the state (compare `TSEL->preset`, `TSEL->eng_req`); KNOB 3 on a used slot opens NAME
    (`NK_USER_RENAME`), on an empty one says `EMPTY SLOT`; KNOB 4 right then OCT+: empty slot → `name_on()` with
    `NK_USER_SAVE`; used slot → `ui.confirm == CF_OVR_USER`; while playing → `STOP TO SAVE`; SAVE + C5 on SOUND renames.
  - `test_u2_more`: SAVE › MORE lists SOUND: LOAD SLOT (loads `ui.uslot` like `up_ui(0, ..)`), ENGINE, FAVORITE, LIST
    writing what the inherited PRESETS KNOB 2–4 wrote, and TOOLS: ERASE USER SLOT (`CF_ERASE_USER` with
    `ui.confirm_trk == ui.uslot`; an empty slot: `EMPTY SLOT`); every GR_USER / GR_BROWSE reach (SLOT, LOAD, ERASE, SAVE,
    PRESET, ENGINE, FAVORITE, LIST) is a SOUND knob or a MORE row (the first rows of `tests/ux_cover_test.c UXC_SAVE`,
    Task 4 completes it).
  - renders `S_SAVE_SOUND` (7 used slots, the picked one used), `S_SAVE_SOUND_EMPTY`, `S_SAVE_MORE` lint-clean; the
    picture redraws on a change only (`uxs_panels`).
  RED.
- [ ] Step 2 — implement; remove the `USER` and `PRESETS` PAGES rows and their drawing (`graph_sound`,
  `graph_lz_presets`, `lz_track_card`, the GR_USER / GR_BROWSE branches in `ui_graph.c`, `ui.c`, `ui_input.c`) once
  nothing reaches them; `python3 tools/gen_nav_map.py`; GREEN; suite; commit `feat(ui): SAVE SOUND`.

---

### Task 3: SAVE SONG

**Files:** `ux_save.c`, `params.c` (PAGES `{"SAVE SONG", FAM_SAVE, SC_GLOBAL, GR_SVSONG, {0xFF..}}`; `UXM_SAVE_ROWS` SONG
group), `ui_lens.c` (`CE_SAVE`: SOUND, SONG, PROJECTS, TOOLS, MORE), `ux_more.h/.c` (`UXM_H_SONG`, `UXM_A_DELROW`,
`UXM_A_CLRSONG`), `ui_input.c` (`save_knob` / `save_oct` GR_SVSONG; `song_row_insert`), `ui.c`, `ui_layer.c` (G5 opens
SAVE › SONG: `nav_put(SEC_SAVE, "SONG")`), tests, `docs/navigation-map.svg`.

**Interfaces — Consumes:** `uxv_*`, `save_knob`, `save_oct`, `uxm_act_t.arg` (Task 2). **Produces:**
```c
static void song_row_insert(uint32_t after);   /* a row after row `after` (its section G_SLOT-1, bars as `after`'s; scenes shifted) */
```

Spec §2.2 (layout A): KNOB 1 SECTION (A–D: `song.g[G_SLOT]`; with ROW on a real row also
`chain_config.row[ui.song_row].slot`, pre-flight 8), 2 ROW (`ui.song_row`, real rows only, `n / N`), 3 BARS
(`row.bars` 1..`CHAIN_BARS`), 4 SCENE (right stores `scene_capture`, left clears: the inherited ARRANGE KNOB 4). Picture:
four section blocks (letter, `project_name`, EMPTY dim, `live.cur` marked, picked lit, `sec_col`), `ARRANGE` and the rows
as chips (`A16`, picked lit, `chain.row` marked, scene dot), `72 BARS · LOOP` under them. Status: `ROW 3 · B · 16 BARS ·
OCT+ ADDS A ROW`. OCT+ → `song_row_insert(ui.song_row)` (`16 ROWS` when full, `STOP TO EDIT` while a song plays).
`UXM_SAVE_ROWS` SONG: `{UXM_H_SONG}`, `{ID_SV_END, UXR_GLOBAL}` (`song_end_step`, `song_idx_touch`), `{UXM_A_DELROW,
UXR_ACTION}` (CF_DEL_ROW, arg UXA_ROW; no rows: `NO ROW`), `{UXM_A_CLRSONG, UXR_ACTION}` (CF_CLEAR_SONG).

- [ ] Step 1 — failing tests: `test_u3_knobs` (SECTION writes `G_SLOT` and the picked row's slot; ROW clamps to
  `count-1`; BARS 1..CHAIN_BARS; SCENE stores / clears exactly as the inherited ARRANGE KNOB 4: `song_idx.scene` bytes
  compared); `test_u3_add` (3 rows, ROW 2, OCT+ → 4 rows, the new one at index 2 with section `G_SLOT-1`, scenes of rows
  2.. shifted; picked = last → identical to the inherited append); `test_u3_more` (END steps STOP x2..x8 LOOP; DELETE ROW
  → CF_DEL_ROW with `ui.confirm_trk == ui.song_row`; CLEAR SONG → CF_CLEAR_SONG); `test_u3_layer` (SAVE + G5 opens SAVE
  › SONG; F3–B3, C4–F4, C5, D5, E5 as before: existing layer tests green); renders `S_SAVE_SONG` (the 6-row song, scenes
  on 2 and 4, END LOOP, section B picked, A playing), `S_SAVE_SONG_EMPTY` lint-clean; the race's counts updated
  (pre-flight 20). RED.
- [ ] Step 2 — implement; delete the inherited SECTIONS / ARRANGE drawing (`graph_proj`, GR_SLOTS-at-SV_PROJ and GR_SONG
  branches; the FAM_SAVE `ARRANGE` row) — keep SEQ's hidden `SONG` row only if `page_visible` can still show it, else
  delete it too; `gen_nav_map.py`; GREEN; suite; commit `feat(ui): SAVE SONG: sections and the song's rows`.

---

### Task 4: SAVE PROJECT, SAVE MORE, the SAVE layer

**Files:** `ux_save.c`, `params.c` (PAGES `{"SAVE PROJECT", FAM_SAVE, SC_GLOBAL, GR_SVPROJ, {0xFF..}}`, `UXM_SAVE_ROWS`
TOOLS group; delete `PROJECT` / `TOOLS` rows), `ui_lens.c` (`CE_SAVE` final: `NV("SAVE SOUND","SOUND",0,0)`,
`NV("SAVE SONG","SONG",1,0)`, `NV("SAVE PROJECT","PROJECT",2,0)`, `NV("SAVE MORE","MORE",3,0)`; `CG_SAVE` SOUND SONG
PROJECT MORE; `SV_*` and `save_level` gone), `ui.c` (append `CF_LOAD_PROJ`), `project.c` (unsaved state),
`ux_dialog.c` (CF_LOAD_PROJ words), `ux_more.h/.c` (`UXM_A_CLRSEQ UXM_A_INITSND UXM_A_NEWPROJ
UXM_A_CPUTEST`), `ui_input.c`, `ui_layer.c`, `ux_layer.c`, delete `firmware/src/ui_lzsave.c` (and its
include in `ui_graph.c:703`), `tools/gen_manual.py` (`PAGE_SHOT['SAVE'] = {'SOUND': 'save_sound', 'SONG': 'save_song',
'PROJECT': 'save_project', 'MORE': 'save_more'}`), `tests/ux_cover_test.c`, tests, `docs/navigation-map.svg`.

**Interfaces — Consumes:** Tasks 1–3. **Produces:**
```c
/* project.c */
static uint32_t proj_clean_sig;        /* autosave_sig() when the music was last loaded, stored or made new */
static uint8_t proj_unsaved;           /* autosave_sig() != proj_clean_sig, refreshed by autosave_poll's 250 ms tick */
static void proj_clean_mark(void);     /* called by project_save_as, project_load, section_load, section_store, system_init */
static int project_unsaved(void);
/* ui.c: enum ... CF_LIVE_COMMIT, CF_LOAD_PROJ };  (RAM only) */
/* ux_layer.c: ux_layer_owns(LAYER_SAVE) == 1 */
```

Spec §2.3–2.5. PROJECT: KNOB 1 PROJECT (`song_open`, `STOP TO CHANGE PROJECT` while playing), 2 LOAD and 3 SAVE (action
knobs, OCT+ does the armed one), 4 NAME (`name_rename` of section `G_SLOT`). LOAD: `chain_busy` → `STOP TO EDIT`;
`project_unsaved()` → `confirm_open(CF_LOAD_PROJ, G_SLOT-1)`; else `undo_mark(); project_load(G_SLOT-1)`. OCT+ on
CF_LOAD_PROJ (undo marked by the generic path) → `project_load(ui.confirm_trk & 3)`; dialog words: LOAD, `LOAD SECTION
B?`, `Loses the changes not stored. GLO + G4 brings them back.`, verb LOAD, lost. SAVE: as `G_SAVE` (OVR PROJ / NAME).
Picture: four project cards (`lib_scan` names = section A's, total bars of `song_idx.rows[n]`, a dot when `n == song_cur
&& project_unsaved()`, `song_cur` marked, the picked one lit). Status: `PROJECT 2 · SECTION B · OCT+ LOADS` (or SAVES).
MORE TOOLS group: CLEAR SEQ (CF_CLEAR_SEQ, `NOTHING TO CLEAR` when empty), INIT SOUND (CF_INIT_SOUND), ERASE USER SLOT
(CF_ERASE_USER, arg UXA_USLOT, empty: `EMPTY SLOT`), NEW PROJECT (CF_INIT_SYS), CPU TEST (CF_CPU_TEST); all refuse with
`STOP TO EDIT` while a song plays, as `act_do` GR_TOOLS did. MORE order: SONG, TOOLS, SOUND (spec §2.4).
SAVE layer (spec §2.5) drawn by `ux_layer.c`: key map (F3–B3 PLAY A–D, C4–F4 STORE A–D, C5 NAME, D5 LOOP / SONG, E5 SONG
REC, G5 SONG page), knob row (KNOB 1 SONG), status `SAVE: KEYS PLAY / STORE · OCT+ STORE · OCT- RECALL`, footer `SAVE ·
HELD`; state chips via `uxl_chip` (the inherited `layer_state` SAVE chips); C5 NAME on SOUND / PROJECT.

- [ ] Step 1 — failing tests: `test_u4_nav` (SAVE: SOUND → SONG → PROJECT → MORE → SOUND, footers `1 / 4 · NEXT SONG` ..
  `4 / 4`; SELECT walks and scrolls MORE; ALGORITHM picks the track; PRESETS never changes page); `test_u4_project`
  (KNOB 1 = `song_open` on a copy; LOAD clean → loaded at once with `undo_ok`; after a knob turn `project_unsaved()`
  within 300 ms → CF_LOAD_PROJ, OCT− nothing, OCT+ loads and GLO + G4 brings the music back; SAVE used → CF_OVR_PROJ,
  empty → NAME; NAME renames section `G_SLOT`); `test_u4_unsaved` (load → clean; turn a knob → unsaved; store via SAVE
  + C4 → clean; NEW PROJECT → clean); `test_u4_tools` (each TOOLS row opens its dialog with the right `confirm_trk`;
  refusals); `test_u4_cover` (`tests/ux_cover_test.c` new list `UXC_SAVE`: every reach of the inherited USER, PRESETS,
  SECTIONS, ARRANGE, PROJECTS, TOOLS pages — SLOT, LOAD, ERASE, SAVE, PRESET, ENGINE, FAVORITE, LIST, SECTION, LOAD,
  SAVE, NAME, ROW, ROW SECTION, BARS, SCENE, END, ADD, PROJECT, CLEAR SEQ, INIT SOUND, DELETE ROW, CLEAR SONG, NEW
  PROJECT, CPU TEST — is a SAVE knob or MORE row); `test_u4_layer` (every SAVE layer key / knob / OCT as before:
  `section_jump`, `section_store`, `live.mode`, `song_rec_toggle`, `name_rename`, KNOB 1 `lys.song`; renders
  `S_LAYER_SAVE` with a section playing and SONG REC armed); renders `S_SAVE_PROJECT` (4 projects, 2 used, current
  unsaved), `S_SAVE_MORE_END` (MORE scrolled to its end), `S_DLG_LOAD_PROJ` lint-clean. RED.
- [ ] Step 2 — implement; delete `ui_lzsave.c`, the GR_SLOTS / GR_TOOLS / G_LOAD / G_SAVE / G_CLRSEQ / G_INITSND page
  branches in `ui_input.c act_do` and the knob path, `act_cols` / `act_name` / `act_ready` / `go_id` cases, `ui_graph.c`
  SAVE sigs, `ui_draw.c:1241`, the SAVE overlay branches of `ui_layer.c` (draw path only; key / knob handlers stay);
  `gen_nav_map.py`, `gen_manual.py build/ui_new/ACID`; GREEN; suite; size in the report; commit
  `feat(ui): SAVE PROJECT, SAVE MORE and the SAVE layer; inherited SAVE drawing gone`.

---

### Task 5: MENU (AUDIO, CLICK, CONTROLS, MORE) and ABOUT / PROFILE

**Files:** create `firmware/src/ux_menu.c` (included after `ux_more.c`); modify `menu_items.c` (MTAB / MPAGE replaced),
`ui_menu.c` (`menu_cards`, `menu_panel`, `draw_menu`, `menu_input`, `menu_page`, `menu_tab`, `menu_short` deleted;
`menu_document` first screen restyled; `MI_HELP` kept as the hints), `ui.c` (ui fields), `ui_draw.c` (MENU header /
tabs / footer branches at `:250,896,965` deleted), `ui_input.c` (calls `ux_menu_input`), `ux_more.h/.c` (`UXR_MENU`,
headings `UXM_H_SCREEN UXM_H_LEDS UXM_H_USB UXM_H_ABOUT`), `editor_menu.c` (tab), `web/EDITOR_PROTOCOL.md` (tab note),
`tests/ui_test.c` (`test_menu_tabs`, `test_t38_menu` and the MENU tests rewritten), `tests/editor_test.c` (tab names),
`tests/ui_render.c`, `tests/built_on_test.py` (only if ABOUT literals move).

**Interfaces — Produces:**
```c
/* menu_items.c */
enum { MN_AUDIO, MN_CLICK, MN_CONTROLS, MN_MORE, MN_N };
static const char *const MN_NAME[MN_N] = {"AUDIO", "CLICK", "CONTROLS", "MORE"};
static const uint8_t MN_ROW[MN_MORE][4] = {{MI_LOWCUT, MI_TRACKS, MI_LOAD, MI_BPMLOCK},
                                           {MI_CLICK, MI_CLKLVL, MI_COUNTIN, MI_NONE},
                                           {MI_HOLD, MI_ACCEL, MI_LATCH, MI_PANEL}};
static const ux_row_t UXM_MENU_ROWS[];   /* SCREEN: LARGE ANIMATION SCREEN SLEEP; LEDS: LED GLOW SCALE LEDS; USB: USB AUDIO
                                          * USB LEVEL; SYSTEM: CPU CORES CPU CLOCK RESTORE LAST; MIDI: MIDI IS ON GLO; ABOUT: ABOUT
                                          * (rows {MI_*, UXR_MENU}) */
static const ux_more_t UXM_MENU;
static uint32_t mi_tab(uint32_t row);    /* now the row's MN_* page (MI_MIDIOUT, on no page: MN_MORE) */
/* ui.c ui struct: uint8_t mn_page, mn_sel, mn_top;  (the page, MORE's pick and scroll) */
/* ux_menu.c */
static void ux_menu_draw(void);          /* ui.menu 1: the page / MORE; 2 ABOUT; 3 PROFILE */
static void ux_menu_input(uint32_t oct); /* replaces ui_menu.c menu_input */
static void uxn_knobs(ux_knob_t k[4]);
static const char *uxn_where(void);      /* "MENU · AUDIO" */
```

Spec §3. Pages: KNOB n sets row n (`menu_put(r, menu_step(r, s))`, `ux_knob_touch`); an action knob (MI_PANEL) arms,
OCT+ does it (`panel_setup`); OCT± step the last turned setting as before. SELECT walks AUDIO → CLICK → CONTROLS → MORE
(no wrap) and scrolls MORE (`ux_more_turn(&UXM_MENU, &ui.mn_sel, &ui.mn_top, UXM_SELECT, s)`; before the first row: back
to CONTROLS). ALGORITHM does nothing. PRESETS does nothing. HOME closes (from ABOUT too). MORE: KNOB 1 picks, KNOB 2 sets
(UXR_MENU: `menu_put(menu_step)`), action rows (MI_GLOMIDI → `menu_close(); nav_put(SEC_GLO, "MIDI")`; MI_ABOUT → `ui.menu
= 2`) run on KNOB 2 right or OCT+. Footer `MENU · AUDIO   1 / 4 · NEXT CLICK`; status = `menu_help(row)` of the last
turned / picked row (CPU CORES, CPU CLOCK, RESTORE LAST keep "(RESTART)" / "AT POWER-ON"). Panel picture on the three
pages: the four rows, name, value, hint (the inherited `menu_panel`'s content in ux colours). ABOUT (spec §3.1): first
screen = `UF_MARK` "ACID FM" wordmark (as `ux_splash.c`), `FELUCCA_VERSION`, `"Built on JIANT FM and Felucca"`, the QR
(`draw_about_qr`), AUDIO LOAD NOW / PEAK / LATE / SHED, `cpt_line`; SELECT scrolls the rest of `menu_document` (every
credit and licence line kept); OCT+ PROFILE (`prof_document`, same figures), OCT− back; footer `MENU · ABOUT   SELECT
SCROLLS · OCT+ PROFILE`.

- [ ] Step 1 — failing tests: `test_u5_cover` (every `MI_*` but `menu_hidden` and `MI_MIDIOUT` is on exactly one of
  `MN_ROW` / `UXM_MENU_ROWS`); `test_u5_nav` (HOME held opens AUDIO, SELECT ×3 → MORE, footers `1 / 4 · NEXT CLICK` ..
  `4 / 4`, ALGORITHM changes nothing, HOME closes); `test_u5_knobs` (each page's knobs write `menu_get` exactly as the
  inherited `menu_input` did for the same detents; OCT± on the last turned; CALIBRATION armed + OCT+ opens
  `panel_setup`); `test_u5_more` (MORE: LARGE … RESTORE LAST set; MIDI IS ON GLO opens GLO › MIDI; ABOUT opens ABOUT);
  `test_u5_about` (first screen: wordmark pixels in the top area, version, built-on line, QR; SELECT scrolls to
  `menu_scroll_max()`; OCT+ PROFILE; OCT− back; HOME closes); `editor_test` MENU_DESC tabs (0..3, names AUDIO CLICK
  CONTROLS MORE) and every id still answered; renders `S_MENU` (AUDIO), `S_MENU_CLICK`, `S_MENU_CONTROLS`, `S_MENU_MORE`,
  `S_MENU_MORE_END`, `S_ABOUT`, `S_ABOUT_CREDITS`, `S_ABOUT_END`, `S_ABOUT_PROFILE`, `S_ABOUT_PROFILE2` lint-clean (the
  old `S_MENU_*` scenes mapped onto these). RED.
- [ ] Step 2 — implement; delete the inherited MENU drawing and tabs; `gen_manual.py`; GREEN; suite; size in the report;
  commit `feat(ui): MENU on the section engine; ABOUT and PROFILE restyled`.

---

### Task 6: PLAYS, a track-wide loop condition

**Files:** `firmware/src/core.h` (`G_PLAYS1, G_PLAYS6 = G_PLAYS1 + 5,` before `G_COUNT`), `params.c` (`N_PLAYS[CP_NTRIG +
1]` = `UXQ_COND`'s words, moved here; `GP[G_PLAYS1 + k] = PE("PLAYS", N_PLAYS, 0)`; `ID_SQ_PLAYS` before `ID_SQ_END`;
`UXM_SEQ_ROWS` TRACK group gets `{ID_SQ_PLAYS, UXR_TRACK, 0}` after `P_HRQ`), `seq.c`, `project.c`, `ux_seq.c`
(`UXQ_COND` → `N_PLAYS`; `UXQ_VNAME` += "PLAYS"; `uxq_vtext`), `ui_input.c` (`seq_view_turn`), `web/editor.html`
(globals table + 6 `E("PLAYS", …)`, `knownLayout` 96), `web/test_web.mjs` (gcount 96), `web/EDITOR_PROTOCOL.md` (G 90–95),
create `tests/plays_test.c`, `tests/run_tests.sh`, `tests/ui_test.c`.

**Interfaces — Produces:**
```c
/* core.h: G_PLAYS1 .. G_PLAYS6 (ids 90..95): 0 ALWAYS, k = CP_TRIGS[k - 1] (1ST OF 2 .. NOT 1ST), counted in t->loops */
static int trk_plays_on(const track_t *t)   /* seq.c: this loop of the track's pattern is one PLAYS lets through */
{
    uint32_t c = (uint32_t)song.g[G_PLAYS1 + trk_index(t) % NTRK];
    return !c || cp_trig_cond(t, c - 1u);
}
/* project.c: FUNC's 'P' 'L' block after 'M' 'R' (the very last), written when any track's PLAYS is not ALWAYS:
 * 'P' 'L' then one byte a track; absent: every PLAYS 0 (unpack's G_NSTORE..G_COUNT loop gives GP[].def) */
#define PROJ_PL_N NTRK
static int proj_pl_used(const project_t *q);
```

Spec §5.1 and pre-flight 15–16. In `seq_step` the first gate becomes `if ((skip & SEQ_MISS) || !trk_plays_on(t) ||
(!(skip & SEQ_ROLLED) && !step_rolls(t, s)))`; the tick's `motion_step` call is unchanged. `system_init` resets PLAYS
(not in `KEEP`). Status line `T5 PLAYS · 1ST OF 2` (MORE row readout with the track prefix, as GLO › MIDI's `T3 IN ·
OFF`).

- [ ] Step 1 — failing tests (`tests/plays_test.c`, ratchet_test.c's pattern: `#define UI_TEST_NO_MAIN 1`, `#include
  "ui_test.c"`, a 1-step loop NOTE 60 on T1, `seq_start`, note-ons counted per pass through `vage` as `run()` there):
  ```c
  static const uint8_t WANT_1OF2[5] = {1, 0, 1, 0, 1};      /* PLAYS 1ST OF 2 (1): loops 0, 2, 4 */
  static const uint8_t WANT_MIX[5] = {0, 0, 1, 0, 1};       /* PLAYS NOT 1ST (12) + the step's CONDITION 1ST OF 2 */
  ```
  `test_plays_loops` (ALWAYS: 1 each pass; 1ST OF 2: `WANT_1OF2`; EVERY 2ND: 0 1 0 1 0; 1ST ONLY: 1 0 0 0 0);
  `test_plays_cond` (`WANT_MIX`, the step's TRIG via `uxq_cond_turn`); `test_plays_drum` (a DRUM-X hit step silent on the
  excluded loops); `test_plays_locks` (a P_FCUT lock on step 0 still applied on an excluded loop, as with P_MUTE);
  `test_plays_midi_out` (no `mo_send` note-on for T1 on an excluded loop); `test_plays_store` (PLAYS 0 everywhere →
  packed bytes identical to before this change, no `'P' 'L'`, `proj_packed_size` unchanged; T5 = 1ST OF 2 → round trip
  `memcmp`-equal and the block the last bytes after `'M' 'R'`; an import without the block gives 0 even if `song.g` held
  values; NEW PROJECT resets; UNDO restores; `part_dice(CF_DICE_SEQ)` and its OCT− leave `G_PLAYS1..6` alone).
  `tests/ui_test.c` `test_u6_row` (SEQ › MORE TRACK group has PLAYS after REC Q; KNOB 2 on it steps 0..12 with `N_PLAYS`
  words; status `T5 PLAYS \xB7 1ST OF 2`; render `S_SEQ_MORE_PLAYS` lint-clean; race counts per pre-flight 20).
  `web/test_web.mjs`: gcount 96 known; DUMP 96 globals. RED.
- [ ] Step 2 — implement; target budgets checked (`tests/target_budget.py`), the pin guard passes (`./build.sh`); GREEN;
  suite; commit `feat(seq): PLAYS, a track plays only on some loops; stored in an appended block`.

---

### Task 7: The clock rule for every tempo writer

**Files:** `firmware/src/gen.c` (`gen_go` ~2538 / BPM write ~2480), `song_dice.c` (`cpu_test_song`, `cpt_clk`,
`cpt_clock_back`), `ui.c` (`power_on_parts` ~1564), `editor.c` (`ED_SET` ~440), `project.c` (`system_init`,
`project_load`, `undo_do`), `song_main.c` (`section_load`), `ui_draw.c` (`head_bpm_text`), `ux_dialog.c` (CPU TEST
line), `web/EDITOR_PROTOCOL.md`, `tests/ui_test.c`, `tests/editor_test.c`.

**Interfaces — Consumes:** `tempo_ext()` (`ui_input.c`, 3c-1). **Produces:**
```c
static uint8_t cpt_clk;                       /* song_dice.c: the CLOCK before CPU TEST + 1 (0: none) */
static void cpt_clock_back(void);             /* G_CLOCK = cpt_clk - 1 when set; cpt_clk = 0 */
static void head_bpm_text(char *b, uint32_t n);   /* ui_draw.c: the inherited header's tempo: "CLOCK: USB" / "CLOCK: TRS" on
                                                   * GEN under an outside clock, else the BPM */
```
Spec §5.2: GEN under USB / TRS writes no BPM (`if (!(ui_prefs & PREF_BPM_LOCK) && !song.g[G_CLOCK])`) and its header says
`CLOCK: USB`; the demo writes no BPM under USB / TRS; `ED_SET` scope 1 id `G_BPM` under USB / TRS: `tempo_ext()`, no write,
reply with the BPM as it is; CPU TEST: `cpt_clk = G_CLOCK + 1; G_CLOCK = 0` before 160 BPM; `cpt_clock_back()` at the top
of `system_init` (before `KEEP` is read), `gen_go`, `project_load`, `section_load`, and after `undo_do`'s restore; the
CPU TEST dialog's line already says "on the internal clock" (Task 1; add "CLOCK comes back after").

- [ ] Step 1 — failing tests: `test_u7_gen` (CLOCK USB, BPM 97, GEN OCT+ → BPM 97, message `CLOCK: USB`,
  `head_bpm_text` = `CLOCK: USB`; INT → BPM in the genre's range); `test_u7_demo` (CLOCK TRS → `power_on_parts()` leaves
  BPM; INT → `DEMO_BPM`); `editor_test` `ed_bpm_clock` (USB: `ED_SET 1 G_BPM 150` → BPM unchanged, reply value = old,
  `ui.msg` `CLOCK: USB`; INT → 150); `test_u7_cpu_test` (CLOCK USB → CPU TEST OCT+ → `G_CLOCK 0`, BPM 160; GLO + G4 →
  `G_CLOCK 1`; again, then NEW PROJECT → `G_CLOCK 1`; again, then `project_load` of a section saved with CLOCK INT →
  INT (the project's own) and `cpt_clk == 0`). RED.
- [ ] Step 2 — implement; GREEN; suite; commit `fix(clock): GEN, the demo, the editor and CPU TEST follow the clock rule`.

---

### Task 8: The web editor's per-track MIDI

**Files:** `web/editor.html` (`LAYOUT` a `{ t: "MIDI", tab: "setgroups", pages: [...] }` entry when `dev.info.gcount >=
90`: `["T1", 1, [71, 77, 83]]` … `["T6", 1, [76, 82, 88]]`, `["KEYBOARD", 1, [89]]`; `G_SKIP` keeps `ROUTE`),
`web/EDITOR_PROTOCOL.md` (a section listing G 71–76 IN, 77–82 OUT, 83–88 CHANNEL stored 1–16, 89 KEYBOARD PLAYS SELECTED,
90–95 PLAYS; written with the existing PARAM_SET), `web/test_web.mjs`.

**Interfaces — Consumes:** firmware globals `G_MRIN1..G_MRSEL` (71–89), `G_PLAYS1..6` (Task 6).

- [ ] Step 1 — failing tests (`web/test_web.mjs`): `editor: MIDI page` — with the mock device (gcount 96) the MIDI tab
  exists, its pages' ids are `[71+k, 77+k, 83+k]` and `[89]`, the mock's `gdesc` labels at them are IN / OUT / CHANNEL /
  SELECTED (so they match the firmware's `desc.json`); turning T3's CHANNEL sends `ED_SET` `[1, 85, v]` and the mock's
  value follows; ROUTE (14) is shown nowhere; with gcount 71 (0.4 firmware) no MIDI tab. RED.
- [ ] Step 2 — implement; GREEN (`node web/test_web.mjs`); suite; commit `feat(editor): per-track MIDI page (IN, OUT,
  CHANNEL, KEYBOARD PLAYS SELECTED)`.

---

### Task 9: English example names; the [3c-2] leftovers

**Files:** `tests/ui_render.c` (`:1817-1872`), `firmware/src/ui_name.c` (`:433` footer), `ui_draw.c` / `ui_layer.c`
comments naming LENTE where they describe shown text, `tests/built_on_test.py` (Spanish check), `ux_section.c` /
`ux_glo.c` / `ux_more.c` (marks), `ux_layer.c` (GLO NO RESTART re-check), the stale comments (`tests/ui_render.c` ~997
and `tests/ui_test.c` ~11693, hint_tick; `web/EDITOR_PROTOCOL.md` lines that present GLO › MIXER as current), tests.

Spec §5.4: render data English (`NOCHE LARGA` → `LONG NIGHT`, `NOCHE B` / `NOCHE C` → `NIGHT B` / `NIGHT C`, `PAD NUBE`
→ `CLOUD PAD`, the VOICE scene's lyric `LA NOCHE` with `LG_ES` → an English lyric with the English language — the ES bank
stays and is still tested where it was); NAME footer `NAME · OCT- CANCEL` / `NAME · STOP TO SAVE` / `LYRIC · K3 DICE ·
OCT- CANCEL`. Marks (deferred "Mark rule"): MIX's LEVEL / PAN knobs and a MORE row's KNOB 2 (and the row in the list) get
`ux_knob_t.mark` from `uxs_marks` when the selected track's setting has automation or a lock on the step.

- [ ] Step 1 — failing tests: `built_on_test.py` gains a Spanish check: the C string literals of `firmware/src/*.c`
  (`c_literals`, the ES lyric bank file(s) and `LG_ES` tables excluded by name) and the render scene literals of
  `tests/ui_render.c` contain none of `LENTE NOCHE NUBE BOSQUE MANO RIELES PISTAS CORTE CARRERA` (word-bounded); `test_u9_marks`
  (an automation record on T2's P_LEVEL → MIX KNOB 1 marked, none without; a lock on P_FCUT at the step → FX › MORE's
  CUTOFF row marked on its KNOB 2 and in the list; pixel samples as `c10_marked`); `test_u9_glo_restart` (GLO layer
  under CLOCK USB says NO RESTART only while playing); manual shots regenerated show the English names (`gen_manual.py
  --check`). RED.
- [ ] Step 2 — implement; regenerate renders / shots; GREEN; suite; commit `fix(ui): English example names; 3c-1 leftovers`.

---

### Task 10: Version 0.6, docs, device

**Files:** `firmware/src/felucca.c` (`FELUCCA_VERSION "v0.6"`), `README.md` (Status 0.6: SAVE, MENU, dialogs, PLAYS,
clock rule, editor MIDI; roadmap line "3c-2 in 0.6"), `web/landing/index.html`, `web/manual/data.js` (a 0.6 news entry in
English; SAVE / MENU / dialog entries rewritten for the new pages and shots `save_sound save_song save_project save_more
menu menu_more about dlg_*`; PLAYS under SEQ), regenerated manual shots / nav (`tools/gen_manual.py`), nav map, cheat
sheets where their generators run, `docs/acid-fm/device-log.md` (a section `## 2026-10-xx — piece 3c-2, ACID FM 0.6`
with every device line **pending**), `docs/superpowers/plans/2026-10-27-acid-fm-piece3c2-deferred.txt` (everything
flagged for later in the task reports, tagged as the 3c-1 list).

- [ ] Step 1 (implementer) — version + docs; full suite; commit `release prep: ACID FM 0.6 (SAVE, MENU, dialogs,
  PLAYS)`; tree clean.
- [ ] Step 2 (controller + user) — fresh backup: in `~/MvaveFM1Unbricker` run `softkey --port "ACID FM" --yes`, then the
  user runs `sudo .venv/bin/python fm1_unbrick.py backup DIR`; `./build.sh --release 0.6` on the clean HEAD; install
  `tools/fm1_install.py build/acid-fm-0.6.fwsc --port "ACID FM" --yes`; serial `status`; the spec §7 no-manual tasks:
  "save T3's sound to a user slot", "make a song A-B-B-C", "start a new project and get the old one back", "make T5 play
  every other bar", "turn the click on with a count-in"; CPU TEST (late blocks / s on SAVE SONG, MENU MORE, a dialog over
  a playing song; CLOCK back after); fill the device log; merge ff to main and push only on the user's go-ahead.
