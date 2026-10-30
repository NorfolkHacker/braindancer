# Braindancer piece 5 (TAPE, the 303 acid response, IDM presets, DRUM-X kits) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Braindancer 0.8: a TAPE master effect (WOW, FLUTTER, SAT, HISS, TONE and the AGE macro) that is bit for bit
out of the chain at 0 and the first rung of the overload safety net, an ANALOG 303 acid response (accent sweep, fixed
~60 ms slide without retrigger in mono / legato), 20 appended IDM factory presets and DRUM-X kits re-voiced towards
808 / 909 / crunchy machines.

**Architecture:** TAPE lives in `fx.c` next to CLIP and the DJ FILTER: a `noinline tape_block()` run by `mix_block` on
core 0 after `clip_block` (the mix, dry + the buses' return, folded as CLIP folds it), never called while TAPE is out of
the chain (every control 0 and its fade done), so the master stays bit for bit 0.7's. Its six controls are appended
song globals `G_TAGE .. G_TTONE` (ids 96–101, `G_COUNT` 96 → 102) stored in an appended FUNC block `'T' 'P'` after
`'P' 'L'`; AGE is a macro that writes the five details when it is turned (knob, editor, automation), so a detail turned
later stays as set until AGE moves again (no extra state). AGE is automated like the DJ FILTER (a record id `MOT_AGE`
126, its records kept in the `'T' 'P'` block). The safety ladder (`audio.c fx_safety_tick`) gains `SAFE_TAPE` as its
first rung; GEN's model (`gen.c gc_cost`) counts TAPE's measured cost. The acid response is a per-track accent
accumulator (`track_t.acc_sw`) and a fixed-time slide in `voice.c`, ANALOG mono / legato only. Presets and kits are
table rows (`eng_*.c`, `drumx_voice.c`), appended / values only.

**Tech Stack:** C (JieLi clang 4 for pi32v2 / host gcc via `tools/host-cc`), Python checks and generators, Node for the
web editor's tests, valgrind (`tests/cpu_vg.py`).

**Spec:** `docs/superpowers/specs/2026-10-30-braindancer-piece5-sound.md` (binding); parents
`docs/superpowers/specs/2026-10-24-acid-fm-design.md` (§4.1 acid response, §4.2 TAPE, §5 voices),
`docs/superpowers/specs/2026-10-24-acid-fm-ui-design.md` (§0, §4.1 the section rules); context: the 0.5.2 overload fix
(`audio.c fx_safety_tick`, `gen.c` GC_* budget), device log `docs/braindancer/device-log.md`.

## Global Constraints

- GPL-3.0-only; never remove JIANT / Felucca copyright lines; changed files gain `Portions Copyright (C) 2026
  NorfolkHacker` if they lack it; a new file `SPDX-License-Identifier: GPL-3.0-only` + `Copyright (C) 2026 NorfolkHacker`.
- Spec §1: "Off by default (AGE 0 and every detail 0 = bypassed, output bit-identical to 0.7). Saved with the project as
  new song globals (appended, append-only storage; older projects load with TAPE off). Automatable and lockable like DJ
  FILTER." (lock: pre-flight 3.)
- Spec §4: "Storage append-only (new song globals for TAPE); presets appended; kits change values only." Setting ids,
  `MI_*`, MIDI CCs, the editor protocol's messages and every existing stored byte keep their meaning; a 0.7 project, user
  preset and settings load and sound as in 0.7 (TAPE off); 0.7 reading a 0.8 project ignores `'T' 'P'` (it stops reading
  at a block it does not know).
- Spec §4 CPU: "TAPE is a fixed per-sample cost while on; measured on the device with CPU TEST (TAPE on) and covered by
  the target budget tests (`tests/target_budget.py`): the ISR / TIMER5 / slicer budgets may rise only by TAPE's own cost,
  which is recorded." Plan rule: `tape_block` is `noinline` with its own budget line; `fm1_alnk0_irq 300`, `fm1_timer5_irq
  2170`, `slicer_bus 821` unchanged; every other line of `tests/target_budget.txt` and `tests/cpu_baseline_vg.txt`
  unchanged (a new line may be added, an old one never rewritten without a reason in the commit message).
- Variables an interrupt reaches stay pinned (`tools/build.py audio_sections` / `unpinned_vars`): new audio statics are
  plain `static` definitions in `fx.c` / `voice.c` / `audio.c` (no `common`, `weak`, `__attribute__((section))` other than
  an existing pattern); the build's `layout:` line must not report unpinned variables.
- Image at HEAD c7b9897 (measured): 555,944 B of 584,336 (28,392 B free); floor **16 KiB free**. RAM .data+.bss 80,976 B
  of 98,304 (build error past 96 KiB); pool spare 9,888 B (guard 8,192: **no new `.pool` data**); RAM code 19,672 of
  25,600. Each task reports the image and RAM lines of `./build.sh`.
- English only on the device, in presets and in the manual; plain words (UI spec §0 rule 4). Naming rule: Braindancer,
  "built on JIANT FM and Felucca"; no band names (RETROBOC is the user's own genre word and may appear in docs, not as a
  preset name). The FX layer's existing "TAPE STOP" keeps its name.
- The section rules (UI spec §4.1): **3 pages + MORE**; **every key keeps its job**; OCT+ yes / OCT− no; the footer counts
  every shown page including MORE. FX stays SENDS · SPACE · MASTER · MORE; MASTER becomes CLIP · PUNCH · DUCK · AGE.
- Every status / label / MORE row literal passes the glyph scan (`tests/ux_msg_scan.py`: ux tables named `UX*_*` are
  scanned) and fits its column (`tests/ux_cover_test.c` width sweep).
- Environment (venv required): `cd ~/synth/mvavefirmware && . .venv/bin/activate && export PATH=$PWD/.venv/bin:$PWD/build/node/bin:$PATH JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK`;
  build `./build.sh`; suite `CC=tools/host-cc sh tests/run_tests.sh` (green at c7b9897, `ALL HOST TESTS PASSED`, ≈ 8 min; its log is binary-ish: read it with
  `grep -a`); renders in `build/ui_new/ACID/`; a CHAINS change also needs `python3 tools/gen_nav_map.py` (the suite runs
  `--check`). Golden renders: `GOLDEN_UPDATE=1` only for the renders a task names; budgets: `BUDGET_UPDATE=1` only to add
  a named new line.
- Process: **foreground commands only** (Bash timeout ≤ 600000 ms; if the suite exceeds it, run its parts by hand, never
  in the background); never start wait loops (no `until pgrep` / `while pgrep`); stop anything you start before
  reporting; never leave HEAD detached.
- Branch `piece5-sound`; commits end with a blank line + `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`;
  never on main, never push; no device or serial access (Task 10 step 2 is the controller's with the user).

## Pre-flight notes (spec points the code makes hard or ambiguous; controller rules before Task 1)

Each with the plan's default, which the tasks below implement unless the controller rules otherwise.

1. **Where TAPE runs vs the core split.** `mix_block` (`fx.c:2449`) runs the parts on both cores and the buses on core 1 a
   block behind; the master (DJ FILTER, CLIP, leveler, `master_out`) runs on core 0 after `mc_wait`. Default: TAPE on
   core 0 right after `clip_block` (counted in `PRF_POST`), before `master_rest` / `lev_block`, so the FX layer's
   `perf_master` path gets it too; it lengthens core 0's critical path by its cost (GEN counts it on both core counts,
   Task 5). Not on core 1: it would need the master's input a block early.
2. **Stereo vs mono master.** The master is stereo (`mix_l`, `mix_r`) plus the mono bus return `wet`. Default: TAPE folds
   `wet` into both sides (as `clip_block` does), one shared WOW (one transport), FLUTTER's right side at 1.07 × the
   left's rate (spec "a little different left and right"), HISS decorrelated (two noise states), TONE per side.
3. **"Lockable like DJ FILTER".** The DJ FILTER is automation only (`core.h MOT_DJF`: "automation only (no lock)",
   `motion.c motion_put` refuses a lock on it) and no global takes a step lock (`ux_more.h UXM_NL_GLOBAL`). Default: AGE
   automated exactly like the DJ FILTER (record id `MOT_AGE` 126, recorded from FX › MASTER KNOB 4 while REC is armed on
   the selected track, AUTO LIST shows them, stop puts the knob's values back), **no locks**; the five details are not
   automated. A lock would need a new record kind on globals (deferred list).
4. **How the AGE override is stored.** Default: no override state. AGE writes `G_TWOW .. G_TTONE` from its map whenever
   AGE itself changes (knob, editor, automation, undo restores all six); a detail row writes only its own global, so it
   "overrides until AGE is turned again" by construction. The AGE status line names how many details differ from AGE's
   map (`tape_age_custom()`: `AGE · 50%: OLD TAPE, 2 BY HAND`).
5. **AGE records' storage.** The `'M' 'G'` loader of 0.8 and of 0.7 rejects any record that is not `MOT_DJF`
   (`project.c:1316-1325`: `return 0`, the whole project refused), and 0.7's main motion block refuses an unknown id.
   Default: AGE records live only in the new `'T' 'P'` block after `'P' 'L'` (0.7 stops reading before it); the main
   motion block and `'M' 'G'` skip them.
6. **FX › MORE is full.** `UXM_FX_ROWS` holds 63 rows and `ux_more.c` keeps a list's shown rows in one `uint64_t` (64 at
   most, `params.c:817 uxm_fx_fits`). MASTER (DUCK TIME) + TAPE need 7 more (70). Default: Task 2 widens the MORE mask to
   128 rows (`uxm_bits_t`, two `uint64_t` words) before the rows are added; MASTER and TAPE groups are appended after the
   PUNCH LANE group (no existing row moves).
7. **Latency of the wow line.** A read behind the write is the only way to move pitch: TAPE in the chain delays the
   master by its centre read, `TAPE_D0` 480 samples (10.9 ms), line 1024 samples a side (23.2 ms, spec "≈ 20 ms").
   Into / out of the chain is a 256-sample (5.8 ms) crossfade with the undelayed mix (a brief doubling, no click).
   Default as stated; the line is `int16_t` holding the input `>> 2` saturated (4 KiB of .bss; ±4 × full scale headroom,
   −78 dBFS resolution, under HISS) rather than `int32_t` (8 KiB).
8. **Silence path.** `master_rest` (`fx.c:2400`) declares the master at rest only when every state is at 0. Default:
   TAPE counts as at rest only after `TAPE_LEN / CTL + 2` silent blocks in a row with its follower and TONE states at
   exactly 0 (`tape_idle()`); otherwise `master_rest` returns 0. TAPE's states are written so that silence drives them to
   exactly 0 (tested).
9. **Accent sweep and golden renders.** `tests/regress.c` renders ACID lines with accents and slides on mono ANALOG
   (`song/4track_mix` T1 with `ACIDF`, `slicer/song_gate_stut`, possibly `slicer/gate/ANALOG_ACID`, `mode/ANALOG/MONO`,
   `mode/ANALOG/LEGATO`). Default: Task 6 may change only those golden lines (`GOLDEN_UPDATE=1`, the diff limited to
   them, listed in the commit message); any other changed render is a bug. Spec §2 "No switch" accepted.
10. **Which engines / modes get the acid response.** Spec §2 is titled ANALOG. Default: ANALOG only (`ENG_ANALOG`), voice
    modes `V_MONO` and `V_LEGATO`; `V_UNISON` and `V_POLY` keep today's accent (vel 127 + the `v->vel > 110` filter
    lift, `voice.c:1112`) and slide. A slide in those modes is a fixed 58 ms (`glide_set` glide 2: 80 blocks) whatever
    `P_GLIDE`; non-slid notes keep GLIDE as before.
11. **New presets vs GEN.** `gen.c gen_sound` (`:1371`) picks among all of an engine's presets by tag and recipe words
    (`any[32]`): appended presets would change every GEN piece for a seed (and ANALOG at 25 + 7 = 32 fills `any`).
    Default: GEN sees only the presets it saw in 0.7 (`gen_npre(en)`: ANALOG 25, FM6 15, PHASE 16, LOFI 17, VOICE 15);
    recipe hooks for the new ones are not trivial (they change GEN's output) and go to the deferred list. The SELECT sound
    dice (`ui_input.c:1766`) and PRESETS browse every preset.
12. **Preset count vs image.** A `preset_t` row is 24 B + its name in `.rodata` (~35 B each): 20 presets ≈ 0.7 KB, the
    web editor's offline mirror (`web/editor.html` ENG presets, compared by `web/test_web.mjs:495`) grows too. LOFI's
    browsing order `ui.c:1315 LOFI_ORDER[17]` has a `_Static_assert` on its size: appended LOFI presets are appended to
    it. Default: 20 presets (ANALOG 7, FM6 4, PHASE 3, LOFI 3, VOICE 3).
13. **Which kits are re-voiced.** Kit 0 (`DX_KITS[0]` = `DX_KIT_INIT` = `DX_KIT_DEF`) is what INIT SOUND, NEW PROJECT, a
    project without a kit and GEN's kit offsets (`gen.c:1513` adds `gen_tables.c KIT[]` to `DX_KIT_DEF`) start from;
    changing it changes old projects and every GEN drum part. Default: kit 0, ACOUSTIC, DUB ROOTS and INDUSTRIAL unchanged;
    909 CLUB and 808 BOOM re-voiced whole, TECHNO RAW, BREAKS and LOFI DUST made crunchier on named lanes (Task 8). A
    stored project keeps its own kit copy (`project.c q->dx`), so only a new kit load sounds different.
14. **CPU TEST with TAPE.** `song_dice.c cpu_test_song` keeps the globals it does not set, so AGE set before CPU TEST
    stays on (the device step relies on it); its stored result (`cpt_res`, GEN's calibration) then includes TAPE (an
    overestimate: the safe side). Default: no change to CPU TEST; the device log notes which run stored `cpt_res`.

## Codebase map (inventory 2026-10-30 at c7b9897; verify line numbers)

- Master chain `firmware/src/fx.c`: `softclip` (`dsp.c:61`, the TANH table, Q15); `clip_g`, `clip_mk`, `clip_block`
  `:238` (folds `wet` into `mix_l`/`mix_r`, zeroes `wet`); DJ FILTER `djf_cut` `djf_k` `djf_block` `:251-360` (crossfade
  pattern `djf.w`, `djf.mode`); `master_begin` `:361`; leveler `lev_block` `:379`; `duck_block` `:417` (G_DUCK, G_DREL);
  `master_out` `:510` (DC block, LOWCUT, leveler gain, limiter `LIM_T`, `softclip`); delay `dly_wow_adv` `:655` (its
  MOD's wow / flutter: a model for TAPE's), `dly_rd_q` `:668` (Hermite, `DLY_LEN` mask), `dly_lo` `:1369`, `dly_rate`
  `:1485` (`fx_safe & SAFE_DLY`); `perf_master` `:2131`; core split `mc_run` / `mc_wait` / `mc_split` `:2160-2280`;
  `fx_run0` `:2307`; `master_rest` `:2400`; `mix_hush` `:2415`; `mix_block` `:2449` (order: events, `macro_master`,
  `master_begin`, `duck_block`, parts / buses, join, slicer, `djf_block`, `clip_block`, `master_rest`, `lev_block`,
  `master_out` loop or `perf_master`). FS 44100, CTL 32 (`tools/gen_tables.py`).
- Safety net `firmware/src/audio.c:164-236`: `DSF_HALF DSF_CALM DSF_STEP 172 DSF_REARM 1720`, `dsf_behind`,
  `dsf_rearm[3]`, `dsf_calm`, `dsf_wait`, `dsf_since[3]`, `dly_safety`, `safe_hush_n[2]`, `safe_kills`,
  `fx_safety_tick(r, late)` (lever index `l = up >> 1`; release "the last one on goes first"); governor `gov_tick`
  `:139` (`gov_cap`, `gov_lite`, `LOAD_EFF()` `voice.c:300`); `voice.c:301-320` `SAFE_DLY 1u SAFE_TEX 2u SAFE_ARP 4u`,
  `fx_safe`, `safe_part[2]`, `safe_hushed`, `safe_clear`; console `console.c:209-211` (`dly_safety`, `safety_tex`,
  `safety_arp`); tests `tests/audio_test.c:392-550` (`safe_reset`, `safe_run`, `safety`).
- Globals `firmware/src/core.h:150-192` (`G_PLAYS1, G_PLAYS6 = G_PLAYS1 + 5,` then `G_COUNT` = 96); `GP[]`
  `params.c:170-249` (`PD`, `PE`); FX pages `params.c:878-881` (`{"MASTER", FAM_FX, SC_GLOBAL, GR_NONE, {G_CLIP, G_PUNCH,
  G_DUCK, G_DREL}}`), `UXM_FX_ROWS` `:798-818`; chain `ui_lens.c:83-85` (`CE_FX`, `CG_FX`); `param_ref` / `page_id`;
  knob write `ui_input.c:1293 param_knob`; the DJ FILTER's knob `ui_input.c:478 tracks_edit` → `motion_djf_set`.
- MORE engine `ux_more.h` (`ux_row_t`, `ux_more_t {rows, uint8_t n}`, `UXR_*`, `UXM_H_*` up to `UXM_H_ABOUT`,
  `UXM_H_N`), `ux_more.c:15 UXM_HEAD`, `:171 uxm_paged`, `:193-226 uxm_mask uxm_n uxm_on uxm_pick_ok uxm_vis uxm_nvis
  uxm_row_of`, `:253 uxm_fix`, `:283 ux_more_turn`; other users `ux_glo.c:76,531`, `ux_section.c:916,1011,1074,2995`,
  `tests/ux_cover_test.c:404,1065`, `tests/ui_test.c:765`.
- FX words / pictures `ux_section.c`: `UXS_NAME` `:33` (row `{UXS_ALL, UXS_ALL, 1, G_DREL, "DUCK TIME"}` `:120`),
  `UXS_FX_SAY` `:537` + `UXF_*` `:543` (`uxs_fx_say_ok`), `uxs_fx_val` `:572` (`G_CLIP` `:741`, `G_DUCK` `:767`, `G_DREL`
  `:780`), `uxs_fix1`, `uxs_clip_out` `:2752`, `uxs_kick_calc` `:2784`, `uxs_master_draw` `:2803`, the FX picture sig
  `:3105-3112` (`G[]` list), the MASTER meter pace `:3122`.
- Automation `motion.c`: `motion_param` `:41`, `motion_desc` `:50`, `motion_djf_base/on` `:17-20`, `motion_djf_value`,
  `motion_djf_off`, `motion_valid` `:64`, `motion_base_value` `:128`, `motion_restore` `:134`, `motion_put` `:191`,
  `motion_capture` `:241`, `motion_djf_set` `:275`, `motion_step` `:310` (the `MOT_DJF` branch `:327`); `core.h:581
  MOT_DJF 127u`; `params.c:276 track_desc` (`MOT_DJF` → `&GP[G_DJF]`); `ui_events.c:24` (AUTO LIST); `ux_seq.c:661`;
  `editor.c:452` (`ED_SET` `G_DJF` → `motion_djf_set`).
- Project `project.c`: `G_GX_END` `:183` (FUNB globals, not ours); `proj_djf_events` `:795`; `'P' 'L'`
  `proj_pl_byte` / `proj_pl_used` `:864-879`; `proj_packed_size` `:881-905`; pack: main motion without `MOT_DJF` `:990`,
  `'M' 'G'` `:1036-1044`, `'M' 'R'` / `'P' 'L'` `:1075-1085`; unpack `'M' 'G'` `:1316-1325`, `'M' 'R'` `:1366`, `'P' 'L'`
  `:1377-1382`; the `G_NSTORE .. G_COUNT` defaults loop (absent globals = `GP[].def`).
- ANALOG / voices: `eng_analog.c:244 ANALOG_PRESETS` (25), `ENG_ANALOG` `:275`, cutoff use `(p[P_E4] << 8) + m->cutoff`
  `:72,111,168`; `voice.c`: `trk_vmode` `:336`, `glide_set` `:567` (glide 2: 80 blocks ≈ 58 ms), `voice_start` `:591`,
  `mono_play` `:642`, `trk_note_on` mono branch `:752-761` (`t->slide_glide`), per-voice render glide `:1082-1095`,
  accent `:1112` (`if (v->vel > 110) m.cutoff += (m.envq15 * 24) >> 7;`); `seq.c`: `slide_in` / `vel` `:1908-1909`,
  `t->slide_glide = slide_in` `:2051`, stop reset `:2419`; `core.h:400 SF_ACCENT 1u SF_SLIDE 2u`, `track_t` `:602-700`
  (`seq_hold`, `slide_glide` `:631-632`); `V_POLY V_MONO V_LEGATO V_UNISON` `core.h:306`.
- Presets: `preset_t` `core.h:340-353` (`TAG`, `FX`, `PAT`, `WORDS` macros `:354-363`); `eng_fm6.c FM6_PRESETS` (15, E7
  = factory patch 0..7), `eng_phase.c PHASE_PRESETS` (16), `eng_lofi.c LOFI_PRESETS` (17), `eng_formant.c
  FORMANT_PRESETS` (15, engine "VOICE"), `eng_noise.c` (8), `eng_drum.c DRUM_PRESETS` (9, `_Static_assert` with
  `DX_KITS`); `ui.c:1311-1330` (`preset_orig`, `LOFI_ORDER[17]`, `preset_rank`, `preset_nth`); favorites up to 256 a
  engine (`favorites.c`); GEN `gen.c:1371 gen_sound`, `gen_tables.c:307 SR_*`, `:310 RECIPE`, `:373 GEN_NEVER`.
- DRUM-X: `drumx_voice.c:378 DX_KIT_INIT`, `:388 DX_KIT_DEF`, `:396 DX_KITS[]` (`dx_kit_t {lane[8], lay[8], snd[8]}`;
  lane `{mode, {PITCH PMOD DECAY NOISE COLOR} A, B}`, `DX_MODE(wave, filter, snap)`, `DX_DRIVE`, `DXM_LONG << 5`,
  `DXM_NOISE << 5`), `:515 DX_NKIT`, `dx_kit_ok`; `eng_drum.c:586 dx_kit_load`; GEN kits `gen_tables.c:91-101 KIT_*`.
- GEN budget `gen.c:1985-2090`: `GC_ENG GC_VOX GC_REV GC_DIVQ GC_GRAIN GC_HEAD GC_DLO[2][3]`, `gc_dly_stage`,
  `GC_DEV_X100 160`, `GC_GEN_X100 150`, `GC_DEV2_X100 190`, `gc_dev`, `gc_budget`, `gc_cost(steps)` (`c += GC_DLO[..]`
  `:2050`), `gx_fit_sounds`, `gen_fit_delay`, `gen_fit_rest`, `gx_fit_piece`; CPU model `tests/cpu_vg.c:221-228`
  (`song/cputest`, `song/cputest.djf` 3098 vs 3046), `tests/cpu_vg.py`, `tests/cpu_baseline_vg.txt`.
- Budgets / pins: `tests/target_budget.py FUNCS` (`djf_block` the last), `tests/target_budget.txt`; `tools/build.py:192
  HOT_FNS` (`mix_block` in RAM), `audio_sections`, `unpinned_vars`.
- Editor / web: `web/editor.html:1540-1567` (globals mirror, `E(...)`, `D(...)`), `:1574 ENG` presets mirror `P(name, e,
  env, mono, pat, xp)`, `:2757 knownLayout` (`[EXPECT.gcount, 32, 36, 37, 66, 67, 69, 71, 90, 96]`), `:2773` FX LAYOUT
  (`["MASTER", 1, [21, 25, 26, 13]]`), `EXPECT`; `web/test_web.mjs:68-70 knownLayout96`, `:383,403,426` gcount 96, `:480-500`
  the firmware ↔ mirror comparison (`GP`, presets); `web/EDITOR_PROTOCOL.md`; `tests/descdump.c`.
- Tests: `tests/run_tests.sh` (`run()`; `djf_test` build line `:227`), `tests/regress.c` (goldens `tests/golden.txt`; the
  4-track song `:260-300`), `tests/djf_test.c` (`centre_render`, `DJF_CENTRE_090 0x8b898fe6u`: a sequenced four-part
  mix with CLIP 30 through `mix_block`), `tests/delay_test.c`, `tests/drumx_test.c` (`kitx strike rms crossings`),
  `tests/gen_test.c`, `tests/ui_test.c` (FX 3b task 8 block `:16217-16480`: `UXT8_PG`, `uxt8_open`, `uxt8_row`, the
  value table `V[]` `:16455`), `tests/ui_render.c` (scenes `S_*` `:581-605`, names `:620-650`, `fx_more_at` `:928`,
  `S_FXL_MASTER` `:1807`), `tests/ux_cover_test.c`, `tests/ux_msg_scan.py`, `tests/built_on_test.py`.

## File map

| File | Change |
|---|---|
| `firmware/src/core.h` | `G_TAGE .. G_TTONE` (96–101); `MOT_AGE 126u`; `track_t.acc_sw`, `acc_relax` |
| `firmware/src/params.c` | `GP[]` six entries; MASTER page KNOB 4 AGE; `UXM_FX_ROWS` MASTER + TAPE groups; `track_desc` MOT_AGE |
| `firmware/src/fx.c` | TAPE: state, `tape_set`, `tape_begin`, `tape_block`, `tape_idle`, `tape_age_map`, `tape_age_apply`, `tape_age_custom`; `mix_block` / `master_rest` hooks |
| `firmware/src/audio.c`, `voice.c`, `console.c` | `SAFE_TAPE` rung, `tape_safety`; acid response (`an_acid`, `acc_step`, slide) |
| `firmware/src/seq.c` | `acc_step` calls, stop reset |
| `firmware/src/motion.c`, `project.c`, `editor.c`, `ui_events.c`, `ux_seq.c`, `ui_input.c` | AGE automation; `'T' 'P'` block; AGE knob |
| `firmware/src/ux_more.h`, `ux_more.c`, `ux_glo.c`, `ux_section.c` | 128-row MORE mask; headings MASTER, TAPE; TAPE words and picture |
| `firmware/src/gen.c` | `GC_TAPE`, `gen_npre` |
| `firmware/src/eng_analog.c`, `eng_fm6.c`, `eng_phase.c`, `eng_lofi.c`, `eng_formant.c`, `ui.c` | 20 presets; `LOFI_ORDER` |
| `firmware/src/drumx_voice.c` | kits 909 CLUB, 808 BOOM re-voiced; TECHNO RAW, BREAKS, LOFI DUST lanes |
| `web/editor.html`, `web/test_web.mjs`, `web/EDITOR_PROTOCOL.md` | globals 96–101, TAPE layout, presets mirror |
| tests: new `tests/tape_test.c`, new `tests/acid_test.c`; `tests/audio_test.c`, `tests/gen_test.c`, `tests/ui_test.c`, `tests/ui_render.c`, `tests/drumx_test.c`, `tests/cpu_vg.c`, `tests/cpu_baseline_vg.txt`, `tests/target_budget.py`, `tests/target_budget.txt`, `tests/golden.txt`, `tests/run_tests.sh` | per task |
| `firmware/src/felucca.c`, `README.md`, `web/landing/index.html`, `web/manual/data.js`, `docs/MANUAL.md`, `docs/cheatsheet/sound.html` (via `tools/gen_cheatsheets.py`), `docs/braindancer/device-log.md`, new `docs/superpowers/plans/2026-10-30-braindancer-piece5-deferred.txt` | Task 9 / 10 |

---

### Task 1: TAPE's DSP and its six song globals (out of the chain bit for bit)

**Files:** `firmware/src/core.h` (globals), `firmware/src/params.c` (`GP[]`), `firmware/src/fx.c` (TAPE),
`firmware/src/project.c` (`'T' 'P'` with the six values), `web/editor.html` (globals mirror, `knownLayout` 102,
`EXPECT.gcount`), `web/test_web.mjs` (gcount 102), `web/EDITOR_PROTOCOL.md` (G 96–101), create `tests/tape_test.c`,
`tests/run_tests.sh`.

**Interfaces — Produces:**
```c
/* core.h, after G_PLAYS6 (append-only):                                                  */
    G_TAGE,                     /* (Braindancer 0.8) TAPE (fx.c tape_block): AGE 0..100, the macro (FX > MASTER KNOB 4) */
    G_TWOW, G_TFLUT, G_TSAT,    /* .. its five details 0..100 (FX > MORE > TAPE): WOW, FLUTTER, SAT, */
    G_THISS, G_TTONE,           /* HISS, TONE. project.c: block 'T' 'P' (none: all 0, TAPE off) */
    G_COUNT                     /* 102 */
/* params.c GP[] */
    [G_TAGE] = PD("AGE", F_PCT, 0, 100, 0), [G_TWOW] = PD("WOW", F_PCT, 0, 100, 0),
    [G_TFLUT] = PD("FLUTTER", F_PCT, 0, 100, 0), [G_TSAT] = PD("SAT", F_PCT, 0, 100, 0),
    [G_THISS] = PD("HISS", F_PCT, 0, 100, 0), [G_TTONE] = PD("TONE", F_PCT, 0, 100, 0),
/* fx.c */
#define TAPE_LEN 1024u                       /* the line, a side (23.2 ms) */
#define TAPE_D0 (480 << 8)                   /* the centre read, Q8 samples back (10.9 ms) */
#define TAPE_FADE 256u                       /* in / out of the chain, samples (5.8 ms) */
#define TAPE_QUIET (TAPE_LEN / CTL + 2u)     /* silent blocks before TAPE is at rest (master_rest) */
static int16_t tape_buf[2][TAPE_LEN];        /* the input >> 2, saturated */
static struct {
    uint32_t w, rng, wseg, fph[2];           /* write index; noise; WOW's segment left (samples); FLUTTER's phases */
    int32_t wpos, wto, wstep, wsm;           /* WOW: the ramp now / its target / step a sample (Q8), its smoothed value */
    int32_t env, tlp[2], terr[2];            /* HISS's follower; TONE's one-pole states and their error feedback */
    int32_t g, gt;                           /* in the chain, Q15: now, target (0 out, 32767 in) */
    int32_t sat_g, sat_mk, tone_k, hiss_k, flut_a, wow_a;   /* this block's (tape_begin) */
    uint32_t flut_inc;
    uint16_t quiet;                          /* silent blocks in a row */
    uint8_t on;                              /* in the chain (tape_block runs) */
} tape;
static int tape_set(void);                   /* any of G_TWOW .. G_TTONE non-zero (AGE alone does nothing) */
static void tape_begin(void);                /* a block's targets from the globals and fx_safe; entering: line and states cleared */
static __attribute__((noinline)) void tape_block(uint32_t n);   /* mix_l / mix_r (+ wet, folded and zeroed) through TAPE */
static int tape_idle(void);                  /* at rest: tape.quiet >= TAPE_QUIET, env, tlp exactly 0 */
static const uint16_t TAPE_TONE_K[11] = {30245, 28803, 27013, 24947, 22698, 20368, 18049, 15817, 13727, 11813, 10094};
                                             /* one-pole a (Q15) at TONE 0,10..100: fc 18 kHz .. 2.6 kHz (TONE 0: no filter) */
static const uint8_t TAPE_TONE_HZ100[11] = {180, 148, 122, 101, 83, 68, 56, 46, 38, 31, 26};   /* fc / 100 (the words) */
/* project.c: FUNC's 'T' 'P' after 'P' 'L' (the very last), written when one of the six is not 0 (Task 4: or AGE records):
 * 'T' 'P', six bytes (AGE WOW FLUTTER SAT HISS TONE, 0..100), a count n, n records (Task 4; 0 here) */
#define PROJ_TP_N 7u
static int proj_tp_used(const project_t *q);
```

The sound, per sample (spec §1): `x = l + wet` (and `r + wet`); **SAT** (`G_TSAT` s > 0): `sat_g = 4096 + s * 8192 /
100` (x1 .. x3, Q12), `sat_mk = (6000 << 15) / softclip((6000 * sat_g) >> 12)`, `y = ((softclip(((x + 8) >> 4) * sat_g >>
10) * sat_mk + 16384) >> 15) << 2` (clip_block's form: a −15 dBFS level kept); the line written `clamp((y + 2) >> 2,
-32768, 32767)`; **WOW** (w): a smoothed random LFO: each segment `FS * 10 / (3 + rng % 18)` samples long (0.3–2 Hz), a
new random target in `±w * 512` (Q8, up to ±200 samples), `wpos` ramps linearly to it, `wsm += (wpos - wsm) >> 10` (≈ 23
ms smoothing: no pitch steps at the corners); **FLUTTER** (f): a sine (`osc_sine`) at `6 + 9 * f / 100` Hz on the left,
×1.07 on the right, amplitude `f * 768 / 100` (Q8, up to 3 samples); the read `TAPE_D0 + wsm + flutter` Q8 back with a
4-point Hermite (`tape_rd_q`, `dly_rd_q`'s formula on `TAPE_LEN - 1` as mask) `<< 2`; **TONE** (o > 0): `a` interpolated
in `TAPE_TONE_K` at `o / 10`, `tlp += ((y - tlp) * a + terr) >> 15` with the remainder carried in `terr` (so silence
settles at exactly 0); **HISS** (h): `env` = peak follower of `|x|` (attack at once, release `env -= (env >> 9) + 1`,
floored at 0: exactly 0 ≈ 12 ms after silence), noise = xorshift `noise32` high-passed by a first difference, `hiss = (noise
* env >> 15) * (h * 1035 / 100) >> 15` (−30 dB under the music's peak at 100; 0 on silence), decorrelated left / right;
out `= dry + (((tape_out - dry) * g) >> 15)`, `g` ramping toward `gt` by `32767 / TAPE_FADE` a sample. `tape_begin()`:
`gt = tape_set() && !(fx_safe & SAFE_TAPE) ? 32767 : 0` (`SAFE_TAPE` is Task 5's; define `SAFE_TAPE 8u` in `voice.c`
now, never set until Task 5); entering (`!tape.on && gt`): `memset(tape_buf, 0, ...)`, states 0, `g = 0`, `on = 1`;
leaving (`g == 0 && gt == 0` at a block's end): `on = 0`. `mix_block`: `tape_begin()` after `duck_block()`; `if
(tape.on) tape_block(n);` right after `if (clip_g) clip_block(n);`; `master_rest`: `if (tape.on && !tape_idle()) return 0;`.
The five details' scaling is the only place their 0..100 is read (Task 3's words read the same constants).

- [ ] Step 1 — failing tests (`tests/tape_test.c`, `#define UI_TEST_NO_MAIN 1` + `#include "ui_test.c"` as
  `tests/djf_test.c`; build line next to djf's: `$CC -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/tape_test"
  tests/tape_test.c -lm`, `run "TAPE (0.8): off bit for bit 0.7, WOW / FLUTTER / SAT / HISS / TONE in range, in and out
  without a click, stored" "$OUT/tape_test"`):
  - `test_off`: `djf_test.c`'s `centre_render()` scene copied (four parts, CLIP 30, 2400 blocks of `mix_block`) with
    every TAPE global 0 → hash `== 0x8b898fe6u` (the hash 0.9.0 / 0.7 give: `DJF_CENTRE_090`) and a counter
    `tape_runs` (incremented in `tape_block` under `#ifdef TAPE_COUNT`, defined by the test) stays 0; `tape.on == 0`.
  - `test_tone_through`: a helper `tape_tone(hz, amp, nblk, out_l, out_r)` drives `mix_l = mix_r = sine, wet = 0`
    straight through `tape_begin(); tape_block(CTL);` (no parts). WOW 100 alone: the instantaneous frequency (zero
    crossings over 20 ms windows, 1 kHz input, 8 s) deviates ≥ 3 and ≤ 45 cents peak and its slowest / fastest swings
    lie in 0.25–2.2 Hz (zero crossings of the deviation); WOW 30 peak deviation < WOW 100's. FLUTTER 100 alone: the
    deviation's dominant rate in 13–16 Hz (FLUTTER 0+: 6 Hz at 1), peak ≤ 15 cents; left and right deviations differ
    (correlation < 0.95). SAT 100: a −15 dBFS sine's RMS within ±1.5 dB of the dry's; a 0 dBFS sine's peak ≤ the dry's
    (it compresses); SAT 0 + all else 0 but HISS 1: the dry delayed by exactly 480 samples (± the HISS residual).
    TONE 100: 300 Hz within 0.5 dB, 10 kHz ≥ 8 dB down; TONE 50: 10 kHz between TONE 0's and TONE 100's. HISS 100 (WOW
    FLUTTER SAT TONE 0): residual = out − dry delayed 480; residual RMS / tone RMS in −42..−24 dB for a −12 dBFS tone,
    the residual 6 ± 1.5 dB lower for a −18 dBFS tone; silence in → out exactly 0 after 2 blocks.
  - `test_in_out`: a 220 Hz tone at −12 dBFS, HISS 0, WOW 40: AGE's detail set at block 100, all 0 at block 400: the
    output's second difference never past 4 × the tone's own (`djf_test.c tone_d2`'s measure) around both edges;
    `tape.on` 0 again within `TAPE_FADE / CTL + 1` blocks of the last 0; after it, `tape_block` not called.
  - `test_rest`: TAPE on (WOW 40, TONE 50, HISS 50), a note then silence: `tape_idle()` within `TAPE_QUIET + 2` blocks
    of the tail's end, `tlp`, `env` exactly 0, and `mix_hush()` then takes the silent path (`prf_cur.fxrest` grows).
  - `test_store`: TAPE all 0 → `proj_pack` bytes identical to before this change (no `'T' 'P'`, `proj_packed_size`
    unchanged: compare with the size computed with the six forced 0 and `proj_tp_used` false); WOW 40, TONE 70 → round
    trip `memcmp`-equal `song.g[G_TAGE .. G_TTONE]`, the block the last bytes after `'P' 'L'` (or after `'M' 'R'` when no
    PLAYS); a project without the block → all six 0 even if `song.g` held values before the load; a stored value 255 →
    clamped to 100; NEW PROJECT (`system_init`) → all 0; UNDO restores them.
  - `web/test_web.mjs`: gcount 102 known (`knownLayout102`), DUMP 102 globals, `GP[96..101]` labels AGE WOW FLUTTER SAT
    HISS TONE match the mirror.
  RED (no TAPE).
- [ ] Step 2 — implement (core.h, params.c, fx.c, project.c, the web mirror); `./build.sh` (layout line: no unpinned
  variable; RAM +≈4.2 KB), `python3 tests/target_budget.py` (unchanged: TAPE not yet in `FUNCS`); GREEN; suite (goldens
  unchanged); commit `feat(fx): TAPE master effect (WOW, FLUTTER, SAT, HISS, TONE), stored in an appended block`.

---

### Task 2: MORE lists past 64 rows

**Files:** `firmware/src/ux_more.h` (`UXM_MAXROWS`, `uxm_bits_t`), `firmware/src/ux_more.c`, `firmware/src/ux_glo.c`,
`firmware/src/ux_section.c`, `firmware/src/params.c` (`uxm_fx_fits` → `UXM_MAXROWS`), `tests/ux_cover_test.c`,
`tests/ui_test.c`.

**Interfaces — Produces:**
```c
/* ux_more.h */
#define UXM_MAXROWS 128u                       /* (0.8: was 64, one uint64_t) a MORE list's rows at most */
typedef struct { uint64_t w[2]; } uxm_bits_t;  /* the rows shown now, a bit each */
/* ux_more.c (every former uint64_t mask) */
static uxm_bits_t uxm_mask(const ux_more_t *m);
static uint32_t uxm_n(const ux_more_t *m);                      /* m->n, at most UXM_MAXROWS */
static int uxm_on(const uxm_bits_t *sh, uint32_t i);
static void uxm_bit(uxm_bits_t *sh, uint32_t i);               /* set row i */
static int uxm_pick_ok(const ux_more_t *m, const uxm_bits_t *sh, uint32_t i);
static uint32_t uxm_vis(const uxm_bits_t *sh, uint32_t i);
static uint32_t uxm_nvis(const uxm_bits_t *sh);
static uint32_t uxm_row_of(const ux_more_t *m, const uxm_bits_t *sh, uint32_t v);
static void uxm_fix(const ux_more_t *m, const uxm_bits_t *sh, uint32_t *sel, uint32_t *top);
```
Every caller (`ux_glo.c:76,531`, `ux_section.c:916,1011,1074,2995`, `ux_more.c` internals, `tests/ux_cover_test.c:404,1065`,
`tests/ui_test.c:765`) passes `&sh`. `uxm_mask`'s heading sentinel `h = 64u` becomes `UXM_MAXROWS`. Behaviour of every
list ≤ 64 rows unchanged.

- [ ] Step 1 — failing tests (`tests/ui_test.c`, `test_m2_wide`): a test-only list `UXT_WIDE_ROWS[100]` (a heading every
  10 rows, globals `G_SLP0 ..` repeated) through `uxm_mask` → rows 64..99 shown (`uxm_on(&sh, 99)`), `uxm_nvis == 100`,
  `ux_more_turn` KNOB 1 +99 lands on the last setting row and SELECT scrolls to the end (`top` ≥ 92), `uxm_fix` keeps
  the heading above a top row past 64; every existing MORE list gives the same shown rows as the old 64-bit mask
  (`uxm_mask(&UXM_*)` bits compared with a copy of the old function kept in the test); `ux_cover_test` green. RED
  (`uxm_bits_t` unknown).
- [ ] Step 2 — implement; renders of every MORE scene byte-identical (`build/ui_new/ACID/*more*` compared before / after:
  `cmp`); GREEN; suite; commit `refactor(ui): MORE lists hold up to 128 rows`.

---

### Task 3: TAPE's controls: FX › MASTER AGE, FX › MORE MASTER and TAPE groups

**Files:** `firmware/src/params.c` (MASTER page `{G_CLIP, G_PUNCH, G_DUCK, G_TAGE}`; `UXM_FX_ROWS` appended
`{UXM_H_MASTER, UXR_HEAD, 0}, {G_DREL, UXR_GLOBAL, 0}, {UXM_H_TAPE, UXR_HEAD, 0}, {G_TWOW, UXR_GLOBAL, 0}, {G_TFLUT,
UXR_GLOBAL, 0}, {G_TSAT, UXR_GLOBAL, 0}, {G_THISS, UXR_GLOBAL, 0}, {G_TTONE, UXR_GLOBAL, 0}`), `firmware/src/ux_more.h`
(`UXM_H_MASTER`, `UXM_H_TAPE` before `UXM_H_N`), `firmware/src/ux_more.c` (`UXM_HEAD` += "MASTER", "TAPE"),
`firmware/src/fx.c` (AGE map), `firmware/src/ui_input.c` (`param_knob` AGE), `firmware/src/ux_section.c` (`UXS_NAME`
rows, `UXS_FX_SAY` words, `uxs_fx_val`, picture, sig), `web/editor.html` (FX LAYOUT), `tests/ui_test.c`,
`tests/ui_render.c`, `tests/ux_cover_test.c`, `tools/gen_manual.py` (if `PAGE_SHOT['FX']` names MASTER's knobs).

**Interfaces — Consumes:** Task 1's globals and `TAPE_TONE_HZ100`; Task 2's 128-row lists. **Produces:**
```c
/* fx.c */
static const uint8_t TAPE_AGE_PT[3][5] = {{0, 0, 0, 0, 0}, {30, 20, 30, 20, 35}, {75, 50, 65, 50, 80}};
                                        /* AGE 0, 50, 100 -> WOW FLUTTER SAT HISS TONE (linear between) */
static void tape_age_map(int32_t age, int16_t d[5]);   /* AGE's five (0..100) */
static void tape_age_apply(int32_t age);               /* song.g[G_TAGE] = age and the five from its map (motion_guard'ed) */
static uint32_t tape_age_custom(void);                 /* the details that differ from AGE's map, a bit each (0: none) */
/* ux_section.c: words (appended to UXS_FX_SAY, UXF_* in order) */
UXF_TAPE1 ": A LITTLE TAPE", UXF_TAPE2 ": OLD TAPE", UXF_TAPE3 ": WORN CASSETTE", UXF_BYHAND " BY HAND",
UXF_WOW ": SLOW PITCH DRIFT", UXF_HZ "HZ", UXF_HISS ": FOLLOWS THE MUSIC", UXF_HIGHS ": HIGHS OFF ABOVE ", UXF_KHZ "KHZ"
static void uxs_tape_strip(int32_t x, int32_t y);      /* MASTER's picture: five 4 x 20 px bars (W F S H T), UX_ACID to the value */
```
Spec §1 Controls. KNOB 4 on MASTER: `param_knob(0, G_TAGE, ..)` → `tape_age_apply(param_turn(...))` (the generic
write replaced for this id; Task 4 adds the recording). A detail row (MORE KNOB 2): the generic global write only.
DUCK TIME keeps its words (`DUCK TIME · 208MS`) as a MORE row under MASTER. Words (knob short / status full, `<NAME> ·
<full>`): AGE `OFF` | `50%` / `AGE · 50%: OLD TAPE` (1–34 A LITTLE TAPE, 35–69 OLD TAPE, 70–100 WORN CASSETTE), with
`tape_age_custom()` k bits set: `AGE · 50%: OLD TAPE, 2 BY HAND`; WOW `WOW · 30%: SLOW PITCH DRIFT`; FLUTTER `FLUTTER ·
40%: 9.6HZ` (6 + 9·v/100, `uxs_fix1`); TAPE SAT `TAPE SAT · 50%: DRIVE X2.0` (`UXF_DRIVE`, 1 + 2·v/100); HISS `HISS ·
50%: FOLLOWS THE MUSIC`; TAPE TONE `TAPE TONE · 50%: HIGHS OFF ABOVE 6.8KHZ` (`TAPE_TONE_HZ100` interpolated); every
detail at 0 `OFF`. `UXS_NAME` rows: `G_TWOW "WOW"`, `G_TFLUT "FLUTTER"`, `G_TSAT "TAPE SAT"`, `G_THISS "HISS"`,
`G_TTONE "TAPE TONE"`, `G_TAGE "AGE"`. Picture: `uxs_master_draw` draws `uxs_tape_strip` in the panel's free lower right
(clear of the OUT meter and the kick curves; ui_render's lint must report no overlap) when `tape_set()`, nothing when
not; the FX sig's `G[]` gains `G_TAGE G_TWOW G_TFLUT G_TSAT G_THISS G_TTONE` (not automation-driven except AGE: AGE
records move the details at most once a step, acceptable). Editor: `["MASTER", 1, [21, 25, 26, gcount >= 102 ? 96 :
13]]`, and when `gcount >= 102` `["TAPE", 1, [97, 98, 99, 100]], ["TAPE 2", 1, [101, 13]]` (DUCK TIME with TONE).

- [ ] Step 1 — failing tests (`tests/ui_test.c`, after the 3b task 8 block):
  - update `UXT8_PG` MASTER to `{"CLIP", "PUNCH", "DUCK", "AGE"}, {G_CLIP, G_PUNCH, G_DUCK, G_TAGE}`; the `V[]` row for
    `G_DREL` moves to page 3 (MORE) with the same words; new rows `{2, 3, 1, G_TAGE, 0, "OFF", "AGE \xB7 OFF"}`, `{2, 3,
    1, G_TAGE, 50, "50%", "AGE \xB7 50%: OLD TAPE"}`, `{2, 3, 1, G_TAGE, 100, "100%", "AGE \xB7 100%: WORN CASSETTE"}`,
    `{3, 1, 1, G_TWOW, 30, "30%", "WOW \xB7 30%: SLOW PITCH DRIFT"}`, `{3, 1, 1, G_TFLUT, 40, "40%", "FLUTTER \xB7 40%:
    9.6HZ"}`, `{3, 1, 1, G_TSAT, 50, "50%", "TAPE SAT \xB7 50%: DRIVE X2.0"}`, `{3, 1, 1, G_THISS, 50, "50%", "HISS \xB7
    50%: FOLLOWS THE MUSIC"}`, `{3, 1, 1, G_TTONE, 50, "50%", "TAPE TONE \xB7 50%: HIGHS OFF ABOVE 6.8KHZ"}`.
  - `test_t3_age`: FX › MASTER KNOB 4 from 0 to 50 → `song.g[G_TAGE] == 50` and the five `== TAPE_AGE_PT[1]`; then FX ›
    MORE TAPE's WOW to 10 → WOW 10, the others still AGE's, status `AGE · 50%: OLD TAPE, 1 BY HAND` when MASTER is back;
    KNOB 4 one detent → WOW back on AGE's map (`tape_age_custom() == 0`); AGE 0 → all five 0, `tape_set() == 0`;
    `tape_age_map(25)` is the midpoint of PT[0] and PT[1] (integer division as coded); UNDO after an AGE turn restores
    all six.
  - `test_t3_more`: FX › MORE lists `MASTER` then `DUCK TIME`, `TAPE` then WOW FLUTTER TAPE SAT HISS TAPE TONE after
    PUNCH LANE; DUCK TIME is no longer on MASTER; every key keeps its job (OCT± on MASTER as before; SELECT walks SENDS
    SPACE MASTER MORE; footer `3 / 4 · NEXT MORE`); the ux_cover width sweep covers the new names and values at 0, 1,
    50, 99, 100.
  - renders: `S_FX_MASTER_AGE` (`fx_3_age`: AGE 60, TAPE strip drawn), `S_FX_MORE_TAPE` (`fx_more_tape`: TONE picked),
    `S_FX_MORE_MASTER` (`fx_more_master`: DUCK TIME picked) lint-clean; `S_FXL_MASTER` and `fx_3` unchanged but for KNOB 4.
  - `web/test_web.mjs`: with gcount 102 the FX LAYOUT has MASTER `[21, 25, 26, 96]`, TAPE `[97, 98, 99, 100]`, TAPE 2
    `[101, 13]`; with 96 as before.
  RED.
- [ ] Step 2 — implement; `python3 tools/gen_nav_map.py` (no chain change expected: `--check` green), `python3
  tools/gen_manual.py build/ui_new/ACID` if the manual's FX shot names change; GREEN; suite; commit `feat(ui): FX ›
  MASTER AGE; FX › MORE gains MASTER (DUCK TIME) and TAPE`.

---

### Task 4: AGE's automation (as the DJ FILTER's)

**Files:** `firmware/src/core.h` (`MOT_AGE`), `firmware/src/motion.c`, `firmware/src/params.c` (`track_desc`),
`firmware/src/project.c` (`'T' 'P'` records; main block and `'M' 'G'` skip `MOT_AGE`), `firmware/src/ui_input.c`
(AGE knob records), `firmware/src/editor.c` (`ED_SET` `G_TAGE`), `firmware/src/ui_events.c`, `firmware/src/ux_seq.c`,
`tests/tape_test.c`, `web/EDITOR_PROTOCOL.md`.

**Interfaces — Consumes:** `tape_age_apply` (Task 3), `'T' 'P'` (Task 1). **Produces:**
```c
/* core.h */
#define MOT_AGE 126u            /* (0.8) a record of TAPE's AGE (G_TAGE), automation only, as MOT_DJF */
_Static_assert(P_COUNT < MOT_AGE, "MOT_AGE past every track parameter");
/* motion.c */
static int16_t motion_age_base[6] __attribute__((section(".bss.djf")));   /* G_TAGE .. G_TTONE before records moved them */
static uint8_t motion_age_on __attribute__((section(".bss.djf")));        /* tracks whose AGE record moved them (bit) */
static void motion_age_off(uint32_t k);              /* track k's records stop: the six back when no track's moves them */
static void motion_age_set(track_t *t, int16_t v);   /* the knob / editor: tape_age_apply(v), recorded as motion_djf_set */
/* project.c */
static uint32_t proj_age_events(const motion_store_t *m);   /* MOT_AGE records among m's */
```
`motion_param` accepts `MOT_AGE`; `motion_desc` / `track_desc` give `&GP[G_TAGE]`; `motion_valid` and `motion_put` refuse
a lock on it; `motion_capture` / `motion_step` / `motion_restore` / `motion_base_value` mirror the `MOT_DJF` branches with
`tape_age_apply(MOTION_VAL(e))` and the six-value base. `'T' 'P'` written when `proj_tp_used(q) || proj_age_events(&q->
motion)`; its count byte and records as `'M' 'G'`'s (each `MOTION_ID == MOT_AGE`, else the load fails, as `'M' 'G'`);
the main block and `'M' 'G'` leave `MOT_AGE` out. `ED_SET` scope 1 `G_TAGE` → `motion_age_set(TSEL, v)`.

- [ ] Step 1 — failing tests (`tests/tape_test.c` `test_age_auto`, `djf_test.c test_automation`'s pattern): REC armed on
  T1, playing, AGE turned at steps 0, 4, 8 → three `MOT_AGE` records on T1 (AUTO LIST shows `AGE` rows via
  `ui_events.c`); a WOW turned by hand meanwhile is overwritten at the next AGE record; stop → the six back to their
  values before the first record; play → at each record's step `song.g[G_TAGE]` and the five follow; a lock request on
  `MOT_AGE` (`motion_put(.., MOTION_LOCK)`) returns 1; save / load → the same records, the main motion block's bytes
  identical to a project without AGE records, `'M' 'G'` without them; a crafted `'T' 'P'` with a non-AGE record → load
  refused (`proj_unpack_c` 0); the editor's `ED_SET 1 96 70` → AGE 70 and its five, recorded when REC is armed; a 0.7
  project (no block) loads with no AGE records; `fuzz_proj` green. RED.
- [ ] Step 2 — implement; GREEN; suite; commit `feat(fx): AGE automated as the DJ FILTER, records kept in 'T' 'P'`.

---

### Task 5: TAPE in the safety net, in GEN's budget, in the CPU budgets

**Files:** `firmware/src/voice.c` (`SAFE_TAPE` comment), `firmware/src/audio.c` (`fx_safety_tick`, arrays to 4,
`tape_safety`), `firmware/src/console.c` (`safety_tape`), `firmware/src/gen.c` (`GC_TAPE`), `tests/audio_test.c`,
`tests/tape_test.c`, `tests/gen_test.c`, `tests/cpu_vg.c` (`song/cputest.tape`), `tests/cpu_baseline_vg.txt` (that line
added), `tests/target_budget.py` (`"tape_block"` appended to `FUNCS`), `tests/target_budget.txt` (its line added).

**Interfaces — Consumes:** `tape`, `tape_set`, `tape_begin` (Task 1), `tape_age_apply` (Task 3). **Produces:**
```c
/* voice.c */
#define SAFE_TAPE 8u            /* (0.8) bit 3: TAPE faded out of the chain (fx.c tape_begin), the ladder's first rung */
/* audio.c */
static uint8_t dsf_rearm[4];                            /* [0] delay [1] texture [2] arp [3] TAPE */
static uint16_t dsf_since[4] = {0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu};
static uint32_t tape_safety;                            /* the times TAPE's rung came on (console: safety_tape) */
static uint32_t dsf_lever(uint32_t bit);                /* SAFE_TAPE -> 3, else bit >> 1 */
/* gen.c */
static const uint8_t GC_TAPE[2] = {GC_TAPE1, GC_TAPE1 * 122u / 100u};   /* [two cores]: TAPE on core 0, the critical path */
```
Spec §1 Overload: in `fx_safety_tick` the first lever tried is `!(on & SAFE_TAPE) && tape.on && tape.gt ? SAFE_TAPE :`
then the existing delay / texture / arp; `tape_safety++` when it comes on; release order "the last one on goes first":
`l = on & SAFE_ARP ? 2u : on & SAFE_TEX ? 1u : on & SAFE_DLY ? 0u : 3u`, its bit `l == 3u ? SAFE_TAPE : 1u << l`; the
same per-lever calm and re-arm (`344u << dsf_rearm[l]`). `safe_clear()` clears it (new music: TAPE back). `tape_begin`
(Task 1) already fades on `fx_safe & SAFE_TAPE`. GEN: `gc_cost` adds `GC_TAPE[mc_two() ? 1 : 0]` when `tape_set()` (both
`steps` 0 and 1; GEN does not change TAPE). `GC_TAPE1` = round((`song/cputest.tape` − `song/cputest`) × 1000 / 3046)
from `tests/cpu_vg.py`'s output (the same units as `GC_DLO`'s 52 for +158); write the measured integer into
`#define GC_TAPE1 <value>u` with the two counts in its comment (the step's own measurement, not an estimate).

- [ ] Step 1 — failing tests:
  - `tests/audio_test.c`: `safe_reset` clears 4 levers (`for (i = 0; i < 4u; i++)`) and `tape_safety`, TAPE off (every
    existing check unchanged and green); new block `{ /* (0.8) TAPE the first rung */ }`: AGE 50 via
    `tape_age_apply(50)`, `safe_reset(4u, LOAD_LITE)`, 125 % → `fx_safe == SAFE_TAPE` within 2 halves, `tape_safety ==
    1`, the delay's lever `DSF_STEP` halves later (`on[delay] == on[tape] + DSF_STEP`), then texture and arp as before
    shifted by one step; `tape.g` reaches 0 within `TAPE_FADE / CTL + 1` blocks of the rung and `tape.on` goes 0; calm
    60 %: back arp, texture, delay, TAPE at 344, 688, 1032, 1376 halves, `tape.on` again and `tape.g` 32767 within the
    fade; a borderline load (2 halves behind every 1.16 s, 20 s) flips TAPE's rung ≤ 3 times (its own re-arm
    `dsf_rearm[3]`); TAPE off: the ladder's halves exactly as before (the existing test).
  - `tests/tape_test.c test_rung_click`: a −12 dBFS 220 Hz tone through `mix_block` with AGE 50, `fx_safe |= SAFE_TAPE`
    at block 200, cleared at 600: second difference ≤ 4 × the tone's own around both edges.
  - `tests/gen_test.c`: a seed's piece with AGE 0 has `gc_cost(1)` equal to before; with AGE 50 set before GEN, `gc_cost(1)`
    is exactly `GC_TAPE[mc_two()]` more and GEN leaves `G_TAGE .. G_TTONE` as they were.
  - `tests/cpu_vg.c`: section `song/cputest.tape` (as `song/cputest.djf`, `tape_age_apply(100)`); `tests/target_budget.py`
    `FUNCS += ["tape_block"]`; both checks fail (no baseline line). RED.
- [ ] Step 2 — implement; `./build.sh`; `BUDGET_UPDATE=1 python3 tests/target_budget.py` then `git diff
  tests/target_budget.txt` shows exactly one added line `tape_block N` (every other line unchanged; if `fm1_alnk0_irq`,
  `fm1_timer5_irq` or `slicer_bus` moved, stop and report); run `python3 tests/cpu_vg.py build/host/cpu_vg
  tests/cpu_baseline_vg.txt` with `BUDGET_UPDATE=1` and keep only the added `song/cputest.tape` line (`git diff` shows one
  line added); set `GC_TAPE1` from those two numbers; console `con_kv("safety_tape", (int32_t)tape_safety)`; GREEN;
  suite; commit `feat(audio): TAPE the safety net's first rung; its cost in GEN's budget and the CPU budgets` (the
  commit message lists `tape_block`'s static cost and `song/cputest.tape` vs `song/cputest`).

---

### Task 6: The 303 acid response (ANALOG mono / legato)

**Files:** `firmware/src/core.h` (`track_t.acc_sw`, `acc_relax`), `firmware/src/voice.c` (`an_acid`, `acc_step`,
render, `trk_note_on`), `firmware/src/seq.c` (`acc_step` call, stop reset), create `tests/acid_test.c`,
`tests/run_tests.sh`, `tests/golden.txt` (named lines only).

**Interfaces — Produces:**
```c
/* core.h track_t, after slide_glide (runtime, never saved) */
    int16_t acc_sw;              /* (0.8) the 303 accent sweep: a cutoff offset (CUT << 8 units) consecutive accents climb */
    uint8_t acc_relax;           /* .. a step without accent played: it decays */
/* voice.c */
#define ACC_STEP 6144            /* an accented step's climb (24 CUT steps) */
#define ACC_MAX 16384            /* (64 CUT steps) */
static int an_acid(const track_t *t);              /* ANALOG and V_MONO / V_LEGATO (trk_vmode) */
static void acc_step(track_t *t, int accent);      /* the sequencer's step starts (seq.c): climb or relax */
```
Spec §2. `acc_step`: `if (!an_acid(t)) { t->acc_sw = 0; return; }`; accent: `acc_sw = min(acc_sw + ACC_STEP, ACC_MAX),
acc_relax = 0`; else `acc_relax = 1`. Render (`voice.c` per track block, before the voices): `if (t->acc_relax &&
t->acc_sw) t->acc_sw = t->acc_sw > (t->acc_sw >> 6) + 1 ? t->acc_sw - (t->acc_sw >> 6) - 1 : 0;` (≈ 46 ms time
constant). Per voice at `:1112`: `if (v->vel > 110) m.cutoff += (m.envq15 * (an_acid(t) ? 48 : 24)) >> 7;` then `if
(an_acid(t)) m.cutoff += t->acc_sw;` (level: accent's vel 127 as today). Slide (`trk_note_on` mono branch): `int sl =
t->slide_glide && an_acid(t);` MONO: `mono_play(t, want, vel, !(sl && sounding), sl ? 2 : (t->p[P_GLIDE] != 0 ||
t->slide_glide));` LEGATO: `mono_play(t, want, vel, !sounding, sounding && (sl ? 2 : (t->p[P_GLIDE] != 0 ||
t->slide_glide)));` (`glide_set`'s glide 2: 80 blocks ≈ 58 ms, the same time for any interval). `seq.c`: `acc_step(t,
(s->flags & SF_ACCENT) != 0)` where a synth step's notes play (next to `vel` `:1908`, not for kits, not for rests or TIEs);
the stop reset `:2419` also zeroes `acc_sw`, `acc_relax`.

- [ ] Step 1 — failing tests (`tests/acid_test.c`, `UI_TEST_NO_MAIN` + `ui_test.c`; run line "303 acid response (0.8):
  accent climb and relax, 58 ms slide without retrigger, poly and other engines unchanged"):
  - `test_climb`: T1 ANALOG preset ACID (`apply_preset_to(t, 4)`), V_MONO, 16 steps C2, steps 0–3 and 8 accented, 120
    BPM; `t->acc_sw` sampled at each step start: 6144, 12288, 16384, 16384, then after step 4 (no accent) < 8192 within
    60 ms and < 256 within 400 ms; step 8 → 6144 more than its value just before; the filter's brightness (a
    high-band energy ratio: first difference RMS / RMS over each step's first 40 ms) strictly rises over steps 0, 1, 2.
  - `test_slide`: V_MONO and V_LEGATO, step 0 C2 with SF_SLIDE, step 1 C3: the voice's `stage` never returns to 1
    (attack) at step 1, `v->env` does not drop by > 10 % at the step edge, `pitch_cur` reaches C3's `pitch16` 55–62 ms
    after step 1 starts (80 blocks ± 4) with GLIDE 0 and with GLIDE 100 (fixed time).
  - `test_unchanged`: V_POLY ANALOG with the same accents → `acc_sw` stays 0 and the render equals a render with
    `an_acid` forced 0 (a test hook `acid_off`); FM6 / PHASE mono with accents and slides → render hashes equal to the
    pre-change hashes recorded in the test (compute them in Step 1 on the unchanged code and paste them).
  - `tests/regress.c` goldens: the run lists which renders changed; allowed: `song/4track_mix`,
    `slicer/song_gate_stut`, `slicer/gate/ANALOG_ACID`, `mode/ANALOG/MONO`, `mode/ANALOG/LEGATO` (pre-flight 9).
  RED.
- [ ] Step 2 — implement; run the suite once without `GOLDEN_UPDATE`: the changed list ⊆ the allowed five (else fix);
  then `GOLDEN_UPDATE=1` and `git diff tests/golden.txt` shows only those lines; listen-check WAVs of
  `song/4track_mix` before / after are saved under `build/acid_demo/` (`acid_test` writes them); GREEN; suite; commit
  `feat(analog): the 303 acid response: accents climb the cutoff, slides glide 58 ms without retrigger` (body: the
  golden lines changed).

---

### Task 7: The IDM factory presets (appended)

**Files:** `firmware/src/eng_analog.c`, `eng_fm6.c`, `eng_phase.c`, `eng_lofi.c`, `eng_formant.c` (rows appended),
`firmware/src/ui.c` (`LOFI_ORDER[20]` = the old 17 + `17, 18, 19`), `firmware/src/gen.c` (`gen_npre`), `web/editor.html`
(ENG presets mirror), `tests/gen_test.c`, `tests/ui_test.c`, `tests/golden.txt` (new lines only).

**Interfaces — Produces:**
```c
static uint32_t gen_npre(const engine_t *en);   /* gen.c: the presets GEN chooses among (0.7's): ANALOG 25, FM6 15, PHASE 16,
                                                 * LOFI 17, VOICE 15, else en->npresets */
```
`gen_sound`: `n = gen_npre(en);` (its loops and `gr() % n` on `n`). Rows (spec §3; names English, no band names, ≤ 12
characters, unique in their engine):
```c
/* ANALOG (eng_analog.c, after ORGAN SQR): {WAVE DETUNE MIX NOISE CUT RES DRIVE INT}, {A D S R}, fenv */
    /* (Braindancer 0.8) an IDM set: squelchy acid, detuned pads, dusty wobbly leads */
    {"ACID SQUELCH", {0, 0, 0, 0, 30, 118, 45, 0}, {0, 52, 10, 28}, 62, 1 | TAG(BASS), FX(25, 0, 40, 10), PAT(1)},
    {"ACID SQUARE", {1, 0, 0, 0, 36, 110, 30, 0}, {0, 48, 15, 25}, 56, 1 | TAG(BASS), FX(15, 0, 35, 10), PAT(1)},
    {"ACID LEAD", {0, 8, 50, 0, 48, 100, 35, 12}, {0, 60, 40, 35}, 50, 1 | TAG(LEAD), FX(20, 10, 45, 25), PAT(4)},
    {"DETUNE PAD", {7, 32, 64, 4, 52, 15, 0, 0}, {75, 95, 118, 100}, 10, TAG(PAD), FX(0, 70, 30, 80), PAT(5)},
    {"DUSTY LEAD", {2, 14, 50, 22, 60, 30, 20, 12}, {8, 75, 95, 50}, 15, 1 | TAG(LEAD), FX(10, 40, 45, 40), PAT(4)},
    {"RUBBER BASS", {3, 6, 40, 0, 44, 70, 25, -12}, {0, 55, 45, 22}, 45, 1 | TAG(BASS), FX(10, 0, 15, 10), PAT(2)},
    {"BRAIN BLIPS", {1, 12, 50, 0, 70, 60, 0, 19}, {0, 45, 0, 30}, 40, TAG(PLUCK), FX(0, 20, 60, 40), PAT(3)},
/* FM6 (eng_fm6.c, after WOOD): {ALG FB MOD RATIO MEG VMOD DETUNE SLOT} offsets on factory patch SLOT */
    /* (Braindancer 0.8) bells, glassy keys, metallic basses */
    {"GLASS EP", {0, 0, -12, 0, 10, 0, 25, 0}, {0, 0, 127, 0}, 0, TAG(KEYS), FX(0, 50, 30, 50), PAT(6)},
    {"METAL BASS", {0, 3, 30, 2, 0, 0, 0, 2}, {0, 0, 127, 0}, 0, 1 | TAG(BASS), FX(20, 0, 15, 10), PAT(2)},
    {"TIN BELL", {0, 0, 18, 3, 0, 0, 12, 1}, {0, 0, 127, 0}, 0, TAG(BELL), FX(0, 20, 40, 70), PAT(7)},
    {"MUSIC BOX", {0, 0, -8, 0, -15, 0, 0, 5}, {0, 0, 127, 0}, 0, TAG(PLUCK), FX(0, 10, 35, 60), PAT(3)},
/* PHASE (eng_phase.c, after RING KEYS): {WAVE WAVE2 DCW ENV DTN LINE SUB FB} */
    {"RESO SWEEP", {5, 0, 75, 95, 0, 0, 0, 0}, {0, 70, 25, 45}, 0, 1 | TAG(BASS), FX(15, 0, 35, 20), PAT(1)},
    {"DIGI CHOIR", {5, 4, 32, 28, 14, 0, 0, 0}, {65, 95, 118, 100}, 0, TAG(PAD), FX(0, 65, 25, 85), PAT(5)},
    {"CHIME KEYS", {6, 2, 70, 75, 6, 1, 0, 0}, {0, 80, 20, 70}, 0, TAG(BELL), FX(0, 25, 40, 60), PAT(7)},
/* LOFI (eng_lofi.c, appended): {CHIP WAVE DUTY CRUSH CUT VIB BEND MASK} */
    {"DUSTY KEYS", {0, 1, 64, 30, 70, 25, 0, 0}, {0, 70, 60, 50}, 0, TAG(KEYS), FX(10, 40, 30, 40), PAT(6)},
    {"WOBBLE LEAD", {0, 0, 32, 10, 95, 70, 10, 0}, {4, 65, 95, 40}, 0, 1 | TAG(LEAD), FX(0, 20, 45, 30), PAT(4)},
    {"CRUSH BASS", {1, 1, 0, 45, 75, 0, 0, 0}, {0, 50, 60, 20}, 0, 1 | TAG(BASS), FX(15, 0, 10, 5), PAT(2)},
/* VOICE (eng_formant.c, after ROBOT EN): {VOWEL VOWEL2 TALK SHIFT BUZZ BREATH Q RAND} */
    {"DREAM CHOIR", {0, 40, 40, 0, 35, 30, 58, 10}, {80, 95, 118, 100}, 0, TAG(VOX), FX(0, 75, 20, 95), PAT(5)},
    {"AIR CHOIR", {80, 0, 30, 0, 30, 60, 50, 20}, {70, 90, 115, 95}, 0, TAG(VOX), FX(0, 65, 25, 90), PAT(5)},
    {"LOW HUM PAD", {95, 64, 60, -12, 50, 20, 62, 5}, {60, 90, 115, 90}, 0, TAG(VOX), FX(0, 55, 20, 80), PAT(5)},
```
Every value inside its engine's `.edit` range (FM6: ALG 0..32, FB −7..7, MOD −64..63, RATIO −16..16, MEG −64..63, VMOD −7..7,
DETUNE 0..127, SLOT a factory patch 0..7; ANALOG INT −24..24; LOFI CHIP 0..4, WAVE 0..6; VOICE SHIFT −12..12). If a row's
sound fails a check below, adjust that row's values only (its name and place stay) and say so in the report.

- [ ] Step 1 — failing tests:
  - `tests/ui_test.c test_p7_presets`: the counts are 0.7's + (7, 4, 3, 3, 3); every preset index < 0.7's count has the
    same name and values as before (a table of the 0.7 names in the test, checked by index: stored preset numbers keep
    naming the same sounds); each new preset loads (`apply_preset_to`) with every `t->p[P_E0 + i]` inside its desc range;
    names unique within the engine, ≤ 12 characters, every glyph in `UF_VAL`; `PR_MONO` / `PR_TAG` as listed; PRESETS on
    LOFI browses the three new ones last (`preset_nth(&ENG_LOFI, 17..19)`); `tests/built_on_test.py` passes on them.
  - `tests/gen_test.c`: GEN over 40 seeds × every genre: no track's preset index ≥ `gen_npre(engine)`; the existing
    determinism checks unchanged.
  - `tests/regress.c`: the 20 new `preset/*` renders pass the health checks (peak under full scale, over
    `LIM_PEAK_MIN`, no DC, the tail ends); existing golden lines unchanged.
  - `web/test_web.mjs`: the mirror equals the firmware's presets (the comparison at `:495` fails until the mirror rows
    are appended).
  RED.
- [ ] Step 2 — implement (tables, `LOFI_ORDER`, `gen_npre`, mirror rows `P("ACID SQUELCH", [0, 0, 0, 0, 30, 118, 45, 0],
  [0, 52, 10, 28], 1, 1)` …); `GOLDEN_UPDATE=1` once: `git diff tests/golden.txt` shows only 20 added `preset/` lines;
  image delta in the report (expected < 1.5 KB); GREEN; suite; commit `feat(sound): 20 IDM factory presets (acid basses,
  detuned pads and choirs, FM bells and metal basses, dusty leads)`.

---

### Task 8: DRUM-X kits re-voiced (values only)

**Files:** `firmware/src/drumx_voice.c` (`DX_KITS[1]` 909 CLUB, `[2]` 808 BOOM whole; lanes of `[3]` LOFI DUST, `[7]`
TECHNO RAW, `[8]` BREAKS), `tests/drumx_test.c`.

**Interfaces — Consumes:** `dx_kit_t`, `DX_MODE`, `DX_DRIVE`, `DXM_LONG`, `dx_kit_ok`, `dx_kit_load` (unchanged).

Spec §3 (pre-flight 13). New rows (the lanes' order KICK SNARE CLAP HAT CL HAT OP TOM RIM BELL; layers and sends as now):
```c
    {   /* 909 CLUB (0.8: a shorter, clickier kick, a noisier snare, bright short hats, a ride for the bell) */
        {
            {DX_MODE(DXW_SINE, DXF_LP, 1) | DX_DRIVE, {6, 100, 62, 6, 56}, {11, 127, 78, 12, 96}},
            {DX_MODE(DXW_FM, DXF_BP, 0), {42, 30, 56, 104, 112}, {48, 18, 68, 124, 92}},
            {DX_MODE(DXW_SINE, DXF_BP, 0), {62, 0, 54, 127, 96}, {56, 0, 66, 127, 88}},
            {DX_MODE(DXW_METAL, DXF_HP, 0), {110, 0, 12, 100, 127}, {118, 0, 6, 116, 127}},
            {DX_MODE(DXW_METAL, DXF_HP, 0), {110, 0, 70, 100, 127}, {118, 0, 84, 116, 127}},
            {DX_MODE(DXW_SINE, DXF_LP, 1), {32, 60, 62, 12, 48}, {38, 80, 72, 22, 68}},
            {DX_MODE(DXW_BELL, DXF_HP, 1), {67, 10, 8, 18, 60}, {74, 24, 12, 32, 74}},
            {DX_MODE(DXW_METAL, DXF_HP, 1), {80, 0, 72, 60, 110}, {86, 0, 86, 72, 120}},
        },
        {0, 6, 0, 9, 9, 0, 0, 11}, {0, 210, 214, 0, 0, 0, 0, 0},
    },
    {   /* 808 BOOM (0.8: a long low kick, a tonal snare, soft hats, a long tom, a cowbell for the bell) */
        {
            {DX_MODE(DXW_SINE, DXF_LP, 1) | DXM_LONG << 5, {2, 30, 112, 0, 0}, {6, 56, 124, 4, 36}},
            {DX_MODE(DXW_SINE, DXF_BP, 0), {38, 12, 44, 80, 58}, {44, 4, 54, 104, 40}},
            {DX_MODE(DXW_SINE, DXF_BP, 0), {56, 0, 56, 127, 66}, {50, 0, 70, 127, 58}},
            {DX_MODE(DXW_METAL, DXF_HP, 0), {96, 0, 10, 92, 100}, {104, 0, 4, 108, 106}},
            {DX_MODE(DXW_METAL, DXF_HP, 0), {96, 0, 56, 92, 96}, {104, 0, 68, 108, 102}},
            {DX_MODE(DXW_SINE, DXF_LP, 1) | DXM_LONG << 5, {22, 20, 88, 4, 10}, {28, 40, 98, 12, 30}},
            {DX_MODE(DXW_BELL, DXF_HP, 1), {67, 6, 10, 10, 34}, {74, 18, 16, 22, 48}},
            {DX_MODE(DXW_METAL, DXF_BP, 1), {72, 0, 40, 8, 70}, {76, 0, 48, 14, 80}},
        },
        {0, 4, 0, 6, 6, 0, 0, 8}, {0, 204, 208, 0, 0, 0, 0, 0},
    },
```
Crunchier lanes (replace these lanes only): LOFI DUST SNARE `{DX_MODE(DXW_FM, DXF_BP, 1) | DX_DRIVE, {44, 34, 32, 112,
60}, {50, 22, 42, 127, 40}}`, HAT CL `{DX_MODE(DXW_METAL, DXF_BP, 0) | DX_DRIVE, {104, 0, 10, 100, 80}, {112, 0, 4, 116,
86}}`; TECHNO RAW SNARE `{DX_MODE(DXW_FM, DXF_HP, 0) | DX_DRIVE, {40, 44, 44, 106, 116}, {46, 32, 56, 127, 96}}`, CLAP
`{DX_MODE(DXW_SINE, DXF_BP, 0) | DX_DRIVE, {60, 0, 46, 127, 100}, {54, 0, 58, 127, 92}}`; BREAKS KICK
`{DX_MODE(DXW_SINE, DXF_LP, 1) | DX_DRIVE, {11, 84, 64, 14, 40}, {16, 114, 80, 20, 80}}`, SNARE `{DX_MODE(DXW_FM, DXF_BP,
1) | DX_DRIVE, {42, 40, 50, 100, 118}, {48, 28, 62, 124, 98}}`. `DX_DRIVE` (`drumx_voice.c:26`, mode bit 7) is per lane
(INDUSTRIAL's TOM already uses it), so `dx_kit_ok` accepts it on any lane.

- [ ] Step 1 — failing tests (`tests/drumx_test.c`, `test_kits_08`): `dx_kit_ok` on every `DX_KITS[k].lane`; an FNV hash
  of `DX_KITS[0]`, `DX_KIT_DEF`, `DX_KITS[4]` (INDUSTRIAL), `[5]` (ACOUSTIC), `[6]` (DUB ROOTS) equal to the hashes
  computed on the unchanged tables (paste them in Step 1); kits 1, 2, 3, 7, 8 differ from those hashes; rendered with
  `kitx` / `strike` at MORPH 0 and 127: 808 BOOM's kick rings longer than 909 CLUB's (time to −40 dB ≥ 2 ×), 909's
  closed hat has more zero crossings in its first 20 ms than 808's, 808's bell (cowbell) has its energy within 500–1200
  Hz (crossings), every lane of every kit sounds (RMS over the first 50 ms > −50 dBFS), stays under full scale and ends
  within 3 s; the crunchier lanes' crest factor is lower than their 0.7 values' (the 0.7 lanes copied into the test).
  `tests/ui_test.c:8501` (`ENG_DRUM.npresets == 9u && DX_NKIT == 9u`) unchanged. RED.
- [ ] Step 2 — implement; `preset/DRUM/00_DRUM-X` and `drums/drum_gm_kit` goldens unchanged; GREEN; suite; commit
  `feat(drumx): 909 and 808 kits re-voiced, crunchier LOFI DUST, TECHNO RAW and BREAKS (values only)`.

---

### Task 9: Docs and the manual for 0.8

**Files:** `README.md` (Status 0.8: TAPE, the acid response, 20 presets, kits), `web/manual/data.js` (a `{v: "0.8",
items: [...]}` news entry first in `news`; FX › MASTER now CLIP PUNCH DUCK AGE; FX › MORE MASTER DUCK TIME and TAPE;
the acid response under SEQ / ACCENT and SLIDE; the presets list), `docs/MANUAL.md` (the same), `web/landing/index.html`
(version line), `tools/gen_cheatsheets.py` → regenerate `docs/cheatsheet/sound.html` (DUCK TIME moved, AGE added),
`web/EDITOR_PROTOCOL.md` (G 96–101, `MOT_AGE`, `'T' 'P'`), regenerated manual shots (`python3 tools/gen_manual.py
build/ui_new/ACID`).

- [ ] Step 1 — `tests/built_on_test.py` and the manual checks (`gen_manual.py --check`, the cheat sheet generator's
  check if it has one) fail on the stale DUCK TIME placement once the shots are regenerated; update the texts in plain
  English (no band names; Braindancer built on JIANT FM and Felucca); GREEN; suite; commit `docs: Braindancer 0.8 (TAPE,
  the acid response, presets, kits)`.

---

### Task 10: Release 0.8 and the device

**Files:** `firmware/src/felucca.c` (`FELUCCA_VERSION "v0.8"`), `docs/braindancer/device-log.md` (a section `##
2026-10-xx — Braindancer 0.8 (TAPE, acid, presets, kits)` with every device line **pending**),
`docs/superpowers/plans/2026-10-30-braindancer-piece5-deferred.txt` (everything the task reports flagged for later,
including: TAPE locks, GEN recipe hooks for the new presets, a GEN genre that sets AGE, AGE as a modulation-matrix
destination, the TAPE strip's look).

- [ ] Step 1 (implementer) — version + log skeleton + deferred list; `./build.sh` (image ≥ 16 KiB free, RAM ≤ 96 KiB,
  pool spare ≥ 8 KiB: numbers in the report); full suite green; commit `release prep: Braindancer 0.8 (TAPE, the 303
  acid response, IDM presets, DRUM-X kits)`; tree clean.
- [ ] Step 2 (controller + user) — fresh backup: in `~/MvaveFM1Unbricker` run `softkey --port "BRAINDANCER" --yes`,
  then the user runs `sudo .venv/bin/python fm1_unbrick.py backup DIR`; `./build.sh --release 0.8` on the clean HEAD;
  install `tools/fm1_install.py build/braindancer-0.8.fwsc --port "BRAINDANCER" --yes`; serial `status` (version
  `braindancer v0.8`, idle cpu, `audio_lost`, `safety_tape` 0); the no-manual tasks: "make it sound like a worn tape"
  (expected path: FX › MASTER KNOB 4 AGE), an acid line with accents and slides on an ANALOG mono track (ACID SQUELCH:
  the climb heard, slides glide without a new attack); CPU TEST twice — TAPE off, then AGE 100 before CPU TEST (late /
  lost blocks a second over ~10 s on SEQ STEPS, cpu %, `safety_tape`; which run stored `cpt_res`, pre-flight 14); a GEN
  piece with AGE 50 (no lost blocks); fill the device log; merge ff to main and push only on the user's go-ahead.

---

## Self-review (done while writing)

- Spec §1 TAPE: DSP Task 1, controls Task 3, AGE override pre-flight 4 / Task 3, storage Tasks 1 + 4, automation Task 4
  (locks: pre-flight 3), overload rung + GEN Task 5, CPU budgets Task 5, bypass bit-identical Task 1 `test_off` +
  goldens. §2 acid: Task 6 (poly unchanged `test_unchanged`). §3 sounds: presets Task 7, kits Task 8. §4 limits: Global
  Constraints, Tasks 5, 7, 10. §5 testing: each task's Step 1; device Task 10.
- Names used across tasks: `G_TAGE G_TWOW G_TFLUT G_TSAT G_THISS G_TTONE`, `tape`, `tape_set`, `tape_begin`,
  `tape_block`, `tape_idle`, `tape_age_map`, `tape_age_apply`, `tape_age_custom`, `TAPE_*`, `SAFE_TAPE`, `tape_safety`,
  `MOT_AGE`, `motion_age_set`, `motion_age_off`, `proj_tp_used`, `proj_age_events`, `uxm_bits_t`, `UXM_MAXROWS`,
  `UXM_H_MASTER`, `UXM_H_TAPE`, `GC_TAPE`, `GC_TAPE1`, `gen_npre`, `acc_sw`, `acc_relax`, `an_acid`, `acc_step`.
