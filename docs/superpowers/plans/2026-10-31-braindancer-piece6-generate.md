# Braindancer piece 6 (one GENERATE button, blends, TAPE in GEN, the AGE retune) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Braindancer 0.9: GEN becomes one page (ENERGY, FORM, COMPLEX, TAPE) whose OCT+ GENERATE picks a style itself
(WILD ~10 %, else one of the ten, never the previous main, ~45 % blended with a compatible partner), fits every piece to
the CPU budget with TAPE counted, sets TAPE's AGE per style, keeps the last 8 pieces for PRESETS to bring back, and a
steeper AGE map makes AGE 50 a clearly worn tape.

**Architecture:** A piece is an 8-byte identity `gid_t` (main style or WILD, partner or none, seed, ENERGY, FORM,
COMPLEX, the TAPE knob, the CPU factor and core count as made). `gen_build(&id)` is the one way a piece is made
(UNDO, `gen_make`, name, message, the recent ring); GENERATE draws an identity (`gen_draw`: a fresh seed, `gid_pick`),
a recent piece is rebuilt from its stored identity (no draw), and the tools' and tests' `gen_go()` maps the old
`gp.genre` onto an identity. A blend is the 0.6.95 RANDOM mutant code with one partner (`gen_blend`); the partner is
mixed into the non-PICK seed streams, so a solo style's piece is 0.8's draw for draw. `gen_make` computes the piece's
AGE (`gs.age`) before any fitting, `gc_cost` counts `GC_TAPE1/2` while `gs.age > 0`, every piece goes through
`gx_fit_piece`, and only then AUTO lowers AGE (`gen_fit_tape`). The AGE retune is a table change in `fx.c`
(`TAPE_AGE_PT`) pinned by a host measurement in `tests/tape_test.c`.

**Tech Stack:** C (JieLi clang 4 for pi32v2 / host gcc via `tools/host-cc`), Python checks and generators
(`tools/gen_manual.py`, `tools/gen_nav_map.py`, `tools/gen_cheatsheets.py`), Node for the web tests.

**Spec:** `docs/superpowers/specs/2026-10-31-braindancer-piece6-generate.md` (binding); parents
`docs/superpowers/specs/2026-10-24-acid-fm-design.md` (the generator), `docs/superpowers/specs/2026-10-30-braindancer-piece5-sound.md`
(§1 TAPE); device findings `docs/braindancer/device-log.md` (0.8, 2026-10-31), `docs/superpowers/plans/2026-10-30-braindancer-piece5-deferred.txt`.

## Global Constraints

- C only. GPL-3.0-only; never remove JIANT / Felucca copyright lines; changed files gain `Portions Copyright (C) 2026
  NorfolkHacker` if they lack it.
- Budgets: `fm1_alnk0_irq 300`, `fm1_timer5_irq 2170`, `slicer_bus 821` unchanged (`tests/target_budget.txt`; every
  line unchanged in this piece: no audio code changes but a `const` table); the ISR pin guard (`tools/build.py
  audio_sections` / `unpinned_vars`): the build's `layout:` line reports no unpinned variable.
- Image: 22,604 B free at 7cb4307; floor **16 KiB free**. Spec §6: "RAM for §4 a few dozen bytes" (the ring: 8 × 8 B).
  Each task reports `./build.sh`'s image and RAM lines.
- Storage append-only. Spec §6: "Projects, presets and songs: unchanged formats; they load as they were (TAPE's sound at
  a given AGE as §5)." "GEN's remembered settings: the dropped fields (genre, seed, the page 2/3 amounts) are ignored on
  load; the plan checks how they are stored and keeps the storage append-only." (pre-flight 1: nothing of GEN is stored.)
- Spec §1: "Every user-visible word in plain upper-case English; no band names (RETROBOC is ours and stays). HOME's
  existing hints that say GENERATE stay true." The manual is English only. JIANT is named only in the credits line
  ("Built on JIANT FM and Felucca"); `tests/built_on_test.py` stays green.
- Spec §2: "Identity: a piece is fully determined by (main style or WILD, partner or none, seed, ENERGY, FORM, COMPLEX,
  the TAPE knob, the device's CPU factor). The same identity always builds the same piece on the same firmware."
- Spec §5: "AGE 0 stays off, bit-identical to 0.7 / 0.8."
- Spec §6: "Fitting every piece changes the piece a given style and seed makes compared with 0.8; the generator's golden
  values are re-recorded." (`tests/gen_test.c GEN_HOUSE_HASH`, Task 3; no `tests/golden.txt` line is expected to change.)
- Every status / label literal passes `tests/ux_msg_scan.py` and fits its column; `tests/ui_render.c`'s lint reports no
  overlap on the GEN scenes.
- Environment (venv required): `cd ~/synth/mvavefirmware && . .venv/bin/activate && export PATH=$PWD/.venv/bin:$PWD/build/node/bin:$PATH JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK`;
  build `./build.sh`; suite `CC=tools/host-cc sh tests/run_tests.sh` (≈ 8 min; its log is binary-ish: read it with
  `grep -a`); host test builds need `-O1` (as `run_tests.sh` builds them). One test alone:
  `tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/gen_test tests/gen_test.c -lm && build/host/gen_test | grep -a -E "FAIL|passed"`
  (`tape_test` the same with its name). Playwright, emcc and valgrind are absent: those checks are skipped (say so).
  Chromium (`/usr/bin/chromium`) and Pillow are present (`gen_nav_map.py`, `gen_manual.py` shots).
- Golden renders: never commit a whole-file `GOLDEN_UPDATE=1` rewrite. At base about 45 renders are missing from
  `tests/golden.txt`; add only lines that legitimately change and name them in the task. This plan expects **no**
  golden line to change (no golden render uses TAPE or GEN); a changed golden is a bug: stop and report.
- STANDING RULE: no agent may download, unpack or install any tool, package or binary. A missing tool: skip that check
  and report it.
- Process: **foreground commands only** (Bash timeout ≤ 600000 ms; if the suite exceeds it run its parts by hand, never
  in the background); never start wait loops (no `until` / `while pgrep`); stop anything you start; never leave HEAD
  detached.
- Branch `piece6-generate`; commits end with a blank line + `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`;
  never on main, never push; no device or serial access (Task 8 step 2 is the controller's with the user).

## Pre-flight notes (spec points the code makes hard or ambiguous; the plan's rulings)

Each with the plan's default, which the tasks implement unless the controller rules otherwise.

1. **Where GEN's settings are stored: nowhere.** `gp` (`gen.c:31-40`) is RAM only: no field of it is written by
   `settings_persist.c`, `project.c` or `editor*.c` (grep), and HOME's page memory `ui.nmem` (`ui.c:287`) is not
   persisted either. So there is nothing to "ignore on load" and no storage format changes. Task 6 pins it: a
   project and the settings are byte for byte the same whatever GEN's page and recent pieces hold.
2. **The identity** (`gid_t`, 8 bytes, Task 2): `seed:14 two:1 | dev:9 form:2 tape:4 | main | part | energy | complex`.
   `main` / `part` are GENRE indices (GENERATE only ever puts one of `GEN_SHOWN` there; tests and tools may name hidden
   ones), `GID_WILD` 0xFE, `GID_SOLO` 0xFF. **The CPU factor** is `dev = gc_dev()` (100..400, from CPU TEST or the
   default) and `two = mc_two()` at the press; `gen_make` copies them to `gcf`, which `gc_budget` and `gc_cost` read
   instead of the live device (`gc_dev()` stays the live formula, tested as such at `gen_test.c:739-775`). Not in the
   identity (as in 0.8): TRACKS 4 / 6, BPM LOCK, an outside clock, and engines / QUANT a user changed on the tracks
   (`gen_make` reads which track is the DRUM track and the first synth's QUANT); a recent piece rebuilt after those
   changed follows the tracks as they are.
3. **"Never the same main twice" vs deterministic rebuild.** The previous main is never part of an identity: it is
   `gp.id.main`, the piece built last by any path (GENERATE, a recent piece, a tool), read only by `gen_draw` when
   GENERATE draws. A rebuild takes the stored identity and never draws, so it cannot be steered by the exclusion. WILD
   is not a "main style": WILD after WILD is allowed, and after WILD any of the ten may come.
4. **A blend** (`gen_blend`, Task 2) is the 0.6.95 mutant block (`gen.c:2424-2455`) with its two donors `b` and `c`
   both the partner, except that the main keeps its **drums** (map, kit, ghosts, hats, fills, laid-back snare, drum %),
   its **tempo range** and its **swing** (spec "the main style sets tempo and feel"; 0.6.95 averaged the tempos and took
   the drums from `b`). From the partner: the bass always; chords, arp, lead and space each on a `gch(50)` (the same
   four draws, same order, from the GS_PICK stream); `scale[1]` and `scale[3]`; the texture. `gs.mutant = 1` as before
   (COMPLEX + 20, an arp / lead in another metre half the time).
5. **`gseed_mix` for a blend.** Today `gseed_mix(part)` is 0 for GS_PICK and `(gs.gi + 1) * 0x85EBCA6B` otherwise.
   New: `^ (gs.pi + 1) * 0xC2B2AE35` when there is a partner (`gs.pi != GID_SOLO`), GS_PICK still unmixed. A solo
   style's streams are exactly 0.8's (so `GEN_HOUSE_HASH` holds through Task 2), IDM+AMBIENT 0421 and IDM+RETROBOC 0421
   differ in every part, and the blend's key / progression choice (GS_PICK) is its main's for that seed.
6. **The FORM hash** (`gform_hash`, `gen.c:1673`) hashed `gp.genre`. New key `gid_form_key(id)`: WILD → `GEN_EXPLORE`,
   solo → `main + 1` (0.8's value), blend → `(main + 1) | (part + 1) << 8`. Solo arrangements are 0.8's.
7. **RANDOM is retired from the UI;** `gp.genre` survives only as the tools' and tests' way in (`gen_go`, `gen_with`,
   `web_gen`, `dice_all`, the film tools): 0 = a GENERATE-style draw from the **seed** (deterministic: `gid_pick` on a
   state derived from `gp.seed`, no exclusion), `1 + i` = GENRE i alone, `GEN_EXPLORE` = WILD. `dice_force_style`
   (the films) forces `main`, solo. `gen_random_genre` goes. `gp.amt[]`, `gp.swing`, `gp.human` stay as internal fields
   (not on the page, not in the identity): GENERATE and a recall reset them to `GA_AUTO`, `GA_AUTO`, 30 ("the generator
   uses their AUTO behaviour for every piece"); tests may still set them through `gen_go`.
8. **`gen_open` no longer sets EXPLORE.** It sets `gp.hot = 0xFF`, `gen_fresh = 1`, `gp.at = 0` (PRESETS on NEW); the
   four knobs keep their values for the session. NEW PROJECT (`ui_input.c:2451`) no longer touches GEN: its knobs and
   recent pieces are the session's (the line `gp.genre = GEN_EXPLORE;` is removed).
9. **The full fitter for every piece** = `gx_fit_piece` (`gen.c:2203`: the delay, grain off, the delay again,
   `gen_fit_rest`: PLAYHEADS, chord notes, long releases, texture, reverb, arp, lead, then the delay back if room). Its
   chord-note step loses its `gx.on &&` guard (`gen.c:2179`). `gx_fit_sounds` (engine swaps before the parts, `gx_sound`
   re-voicing) stays WILD's own: spec §3 names the `gx_fit_piece` order only.
10. **TAPE counted from the piece, not the song.** `gs.age` (the AGE the piece asks for) is computed before any fitting;
    `gc_cost` adds `GC_TAPE[gcf.two]` while `gs.age > 0` (so `gc_cost(0)` before the parts already sees it; the song's
    TAPE globals still hold the previous piece's AGE at that point). `tape_age_apply(gs.age)` writes AGE and its five
    with the other FX writes. `GC_TAPE1 139 / GC_TAPE2 169` stay: the retune changes TAPE's values, not its code paths
    (all five details are non-zero at AGE 1..100 on both maps), and `tape_block`'s static line (780) is unchanged.
11. **"AUTO lowers AGE step by step (to 0 at the last)".** `gen_fit_tape` lowers `gs.age` by 10 while the fitted piece is
    over the budget. TAPE is a fixed cost while AGE > 0 in the model, so once it starts it ends at 0; the loop is written
    as the spec says so a later AGE-dependent model works unchanged. A knob-set AGE (OFF or 10..100) is never changed.
    Spec §7 "within budget" is read for AUTO pieces (the knob's AGE may leave a piece over: the safety net carries it).
12. **WILD's AUTO AGE** `grr(0, 50)` from a new stream `GS_TAPE` (appended to the enum), with `gq` saved and restored
    around it so no other stream moves.
13. **The AGE retune's host target** (Task 1). The device's AGE 0 figures are the SINE KEY preset's own vibrato (sd 2.25
    cents) and the device floor (−53 dB); a host sine has neither, so "2×" and "+6 dB" would be trivial. The host target
    is TAPE's own share, the device's AGE 0 taken out: sd ≥ √(4.50² − 2.25²) = 3.90 → **4.0 cents**; non-harmonic noise
    ≥ 10·log10(10^−4.7 − 10^−5.3) = −48.3 → **−48.0 dB rel**. And a bridge between the two trackers: on the device 0.8's
    AGE 100 had 6.58 cents and −51.2 dB of TAPE's own; the device's AGE 50 reading (0.89 cents of its own) is far under
    what the same WOW gives on the host, so the device's pitch window under-reads slow wow. Hence also: AGE 50's host sd
    ≥ the host sd of **0.8's AGE 100 row**, and its noise ≥ that row's + 3 dB (−48.3 vs −51.2). WOW is capped by the
    line (`TAPE_LEN` 256, 8.7 cents peak), so the new map reaches WOW 100 at AGE 50; 50 → 100 adds FLUTTER, SAT, HISS and
    TONE. Widening the line is out of scope (latency, `GC_TAPE`).
14. **PRESETS on GEN** (`gp.at`): 0 = NEW; k = recent piece k−1 (`gen_recent[0]` is the newest, i.e. the piece just
    made). Turning back (counter-clockwise) goes older, one detent a step, clamped at NEW and at the oldest (no wrap).
    OCT+ at NEW: GENERATE (`gp.at` stays 0); OCT+ at k: that piece rebuilt, moved to the front, `gp.at = 1`. The panel's
    right label: `OCT+ GENERATE` / `OCT+ RECALL` (RECALL is the firmware's word for bringing a sound back); its left line
    `LAST: <name>` / `RECENT k: <name>`. "Plays it, as making a piece does now": exactly today's OCT+ (no transport start
    of its own; refused while ARRANGE runs, `chain_busy()`).
15. **The ring** (`gen_recent[8]`, `gen_nrecent`): every `gen_build` pushes (GENERATE, a recall: moved; a tool's
    `gen_go`); equal identities (`gid_eq`, field by field) are never twice. RAM statics are 0 at boot; `session_dice`
    (the power-on, `ui_layer.c:521`) also clears it, which is what the host test calls "a restart".
16. **SELECT on GEN does nothing** (both ways). HOME's chain keeps one GEN entry (`CE_HOME` `"GEN"`, `UXH_NGEN 1`); a
    remembered HOME entry 3 / 4 (GEN 2 / 3) reads as TRACKS (`ux_home_entry`, past the chain). The lens count reads
    `GEN` (no `n/3`).
17. **Names.** The panel and the status line use the full shown names (`RETROBOC+AMBIENT 0421`, `WILD 0421`); where a
    name does not fit (status: `GEN: ` + name over 232 px in UF_HDR; panel: its line), the cards' short names
    (`GEN_SHOWN_CARD`) are used. `gp.last` grows to 28 bytes (`BREAKS+DRILL'N'BASS 0421` is 24 characters).
18. **The web editor** (`web/editor.html`) shows and sets nothing of GEN (grep `GENRE|GEN`: no hit): unchanged. The
    browser emulator's `web_gen(genre, seed, energy, form)` (`web/emu/felucca_web.c:466`) keeps its signature (genre as
    `gp.genre`, pre-flight 7) and loses `gp.sig = gen_sig();`. emcc is absent; `web/emu/native_check.c` (which includes
    `felucca_web.c`) builds with `tools/host-cc` (checked at base) and is the compile check.
19. **TAPE knob OFF** writes AGE 0 (and its five 0) on every piece, whatever was set; UNDO brings the old TAPE back (it
    is marked before). This withdraws 0.8's "GEN never changes TAPE" (`gen_test.c:1258-1295`, reversed in Task 3).
20. **Tests that measured music, not the budget,** may now see their piece thinned (the hidden genres are fitted too).
    Such a check is moved onto a budget its piece fits by setting `gt_cpt = 63` (the device as the model holds it, ×1.00)
    around it; its assertion is never loosened; each one is listed in the task's commit message.

## Codebase map (inventory at 7cb4307; line numbers verified)

- Styles `firmware/src/gen_tables.c`: `ggenre_t` `:383-401`, `GENRE[]` `:403-585` (43), `NGENRE` `:586`,
  `GEN_EXPLORE` `:592`, `GEN_NPOS` `:593`, `GEN_NSHOWN` `:598`, `GEN_NSPOS` `:599`, `GEN_SHOWN` `:600` (IDM 41, BROKEN
  42, BREAKS 30, DRUM & BASS 31, ACID HOUSE 4, ELECTRO 11, MICRO HOUSE 2, BOOM BAP 26, AMBIENT 37, DUB TECHNO 7),
  `GEN_SHOWN_NAME` `:606`, `GEN_SHOWN_CARD` `:610`, `GEN_SHOWN_TEMPO` `:613`, `gen_shown_index` `:616`,
  `gen_shown_tempo` `:624`, `GX_*` `:635-669`.
- Generator `firmware/src/gen.c`: `gp` `:31-40` (`genre seed energy form amt[] complex swing human page hot sig last[24]`),
  `gr` `:44`, `gen_random_genre` `:45`, `gseed` `:47`, `GS_*` enum `:66`, `gs` `:73-91`, `gseed_mix` `:93`, `gmut`
  `:94`, `gmut_name[20]` `:95`, `gx` `:101`, `gform_hash` `:1673`, `gform_order` `:1680`, `gen_parts` `:1701`
  (`gp.form` `:1704`, `:1706`), `gx_genre` `:1851`, `gx_heads` `:1962`, `gx_mutate` `:1978`, budget comment and `GC_*`
  `:1993-2031` (`GC_TAPE1/2` `:2031`), `gc_dly_stage`, `GC_DEV_X100 160`, `GC_GEN_X100 150`, `GC_DEV2_X100 190`,
  `gc_dev` `:2058`, `gc_budget` `:2065`, `gc_cost` `:2070`, `gsnd`, `gx_fit_sounds` `:2110`, `gen_fit_delay` `:2149`,
  `gen_leave_out` `:2160`, `gen_fit_rest` `:2173`, `gx_fit_piece` `:2203`, `gen_post` `:2219`, `gen_scale_page`,
  `dice_force_style` `:2307`, `gen_pick` `:2309` (`gp.energy/complex` `:2329-2330`), `gen_make` `:2396` (EXPLORE
  `:2411`, genre + mutant `:2418-2462`, sounds `:2497-2510`, FX writes `:2578-2620`, macro routes `:2620-2628`, fitting
  `:2632-2636`), `gen_go` `:2641`, `gen_sig` `:2664`, `gseed_last`, `gen_new_seed` `:2673`, `gen_press` `:2687`,
  `dice_all` `:2695`, `dice_print` `:2704`, `dice_seq` `:2719` (`gp.seed` save / restore `:2724`, `:2770`), the screen
  `:2775-2931` (`GEN_LBL`, `GEN_FORM`, `gen_fresh`, `gen_open` `:2779`, `gen_amt_turn`, `gen_pos`, `gen_knob` `:2798`,
  `gen_page` `:2821`, `gen_page_put`, `gen_value` `:2829`, `gen_cards` `:2851`, `gen_draw_sig` `:2868`, `gen_hash`,
  `GEN_LAST_W`, `gen_last_line` `:2875`, `gen_panel` `:2882`).
- UI: `ui_input.c:2451` (NEW PROJECT sets EXPLORE), `:2606-2609` (OCT± on GEN), `:2640-2643` (PRESETS → `gen_knob(0)`),
  `:2650-2659` (SELECT), `:2670-2673` (knobs → `gen_knob`); `ui_lens.c:48-49` `CE_HOME`, `:139 gen_on`, `:141`, `:165`
  (`nav_cur`), `:229-230`, `:252-261` (`nav_go` opens GEN, `gen_page_put`); `ux_nav.h:9-11` (`UXH_GEN0 2`,
  `UXH_NGEN 3`); `ux_nav.c:10-15` (hints), `:53 UXH_HINT_BUSY`; `ux_home.c:287 UX_DEMO`; `ui_draw.c:992-997` (`GEN n/3`),
  `:1435-1440` (`gen_cards`); `ui_graph.c:511-521, 672-674` (`gen_draw_sig`, `gen_panel`); `ui_layer.c:521-528`
  (`session_dice`: `gp.seed`).
- TAPE `firmware/src/fx.c`: `TAPE_LEN 256` `:379`, `TAPE_WOW_Q 184` `:382`, `TAPE_FLUT_Q 768` `:383`, `tape` `:395`,
  `tape_set` `:412`, `TAPE_AGE_PT` `:420` (`{0..}, {60, 20, 30, 20, 35}, {100, 50, 65, 50, 80}`), `tape_age_map` `:425`,
  `tape_age_apply` `:432`, `tape_age_custom` `:443`, `tape_begin` `:456`, `tape_block` `:503`.
- Tests: `tests/tape_test.c` (helpers `tape_reset` `:169`, `tape_g` `:178`, `tape_tone` `:207`, `cents_of` `:210`,
  `in_l/out_l/out_r[NMAX]` 20 s, `main` at the end), `tests/gen_test.c` (`gen_reset` `:46`, `gen_with` `:52`, `all_sig`
  `:60`, `GEN_HOUSE_HASH` `:117`, knob isolation `:400-409`, FORM `:410-450`, EXPLORE `:779-800`, DICE variety `:882-971`
  incl. power-on `:943-948`, NEW PROJECT `:972-978`, the screen `:979-1102`, budget `:1123-1225`, safety parts
  `:1226-1257`, the 0.8 TAPE pin `:1258-1295`); `tests/ui_test.c` (GEN repaint `:7745-7772`, HOME pages `:10795-10835`,
  SELECT walk `:11965-11985`, hints `:11018`, `:11478`, `:11536`, `:12410`; `gen_go` users `:2405`, `:7833`, `:9217`,
  `:22033`); `tests/ui_render.c` (scenes `S_GEN S_GEN_PARTS S_GEN_LEAD S_GEN_EXPLORE S_GEN_BEAT S_GEN_DNB S_GEN_RETRO`
  `:587-610`, names `:632-657`, cases `:1345-1354`, `:1501-1502`); `tests/editor_test.c:1056-1074`, `tests/ui_test.c:17386-17423`
  (AGE map, symbolic `TAPE_AGE_PT`); `tests/stems_test.c:40`, `tests/hush_test.c:58`, `tests/dice_metrics.c:239`,
  `tests/gen_dump.c:125`, `tools/dice_render.c:33`, `tools/video/capture_*.c` (set `gp.*` then `gen_go`).
- Docs: `tools/gen_manual.py:32-33` (`PAGE_SHOT` HOME GEN 1..3), `tools/gen_nav_map.py:85, 93, 111`,
  `tools/gen_cheatsheets.py:164`, `web/manual/data.js:14, 212, 269-277, 565-569, 615 news`, `docs/MANUAL.md`,
  `README.md`, `docs/cheatsheet/controls.html`, `docs/cheatsheet/sound.html`, `web/landing/index.html`,
  `firmware/src/felucca.c:42 FELUCCA_VERSION "v0.8"`.

## File map

| File | Change |
|---|---|
| `firmware/src/fx.c` | `TAPE_AGE_PT` retuned (Task 1) |
| `firmware/src/gen_tables.c` | `GEN_PARTNER`, `GEN_NPARTNER`, `GEN_WILD_PCT`, `GEN_BLEND_PCT`, `GEN_AUTO_AGE`, `GEN_AGE_MAX`, `GEN_AGE_WILD`; `GEN_NSPOS` / `GEN_NPOS` gone (Task 5) |
| `firmware/src/gen.c` | `gid_t` and its functions, `gen_blend`, `gen_build`, `gcf`, `gs.pi/age/age_cut/cost_thin`, `gen_age`, `gen_fit_tape`, the ring, the one-page screen |
| `firmware/src/ui_input.c`, `ui_lens.c`, `ux_nav.h`, `ux_nav.c`, `ui_draw.c`, `ui_layer.c` | PRESETS, SELECT, NEW PROJECT, HOME's chain, the lens count, `session_dice` clears the ring |
| `web/emu/felucca_web.c` | `gp.sig` line gone |
| tests: `tests/tape_test.c`, `tests/gen_test.c`, `tests/ui_test.c`, `tests/ui_render.c`, `tests/stems_test.c`, `tests/hush_test.c`, `tests/dice_metrics.c` | per task |
| `tools/gen_manual.py`, `tools/gen_nav_map.py`, `docs/navigation-map.svg` (+ PNG), `web/manual/nav.js`, `web/manual/shots/*` | Task 5 (generated) |
| `README.md`, `web/manual/data.js`, `docs/MANUAL.md`, `web/landing/index.html`, `tools/gen_cheatsheets.py` → `docs/cheatsheet/*.html` | Task 7 |
| `firmware/src/felucca.c`, `docs/braindancer/device-log.md`, new `docs/superpowers/plans/2026-10-31-braindancer-piece6-deferred.txt` | Task 8 |

---

### Task 1: The AGE retune (a steeper map, pinned by a host measurement)

**Files:** Modify `firmware/src/fx.c:416-421` (`TAPE_AGE_PT` and its comment); Test `tests/tape_test.c` (new block before
`main`, `main` calls it).

**Interfaces — Produces:**
```c
/* fx.c (values; the type, tape_age_map / tape_age_apply / tape_age_custom unchanged) */
static const uint8_t TAPE_AGE_PT[3][5] = {{0, 0, 0, 0, 0}, {100, 55, 45, 72, 50}, {100, 100, 85, 100, 90}};
                                        /* AGE 0, 50, 100 -> WOW FLUTTER SAT HISS TONE (linear between) */
```
Spec §5 (pre-flight 13). Every other user of the map reads `TAPE_AGE_PT` symbolically (`tests/editor_test.c:1067,1073`,
`tests/ui_test.c:17386-17423`) or only its AGE value (words, safety tests at AGE 50): they stay green unchanged.

- [ ] **Step 1: Write the failing test** (`tests/tape_test.c`, after `test_new_project_motion`, before `main`):

```c
/* ---- 9. (Braindancer 0.9) the AGE retune: AGE 50 a clearly worn tape (spec 5) ----
 * The device (docs/braindancer/device-log.md, 2026-10-31; SINE KEY A3 held, its own vibrato 2.25 cents sd): AGE 0 / 50
 * / 100 gave pitch sd 2.25 / 2.42 / 6.95 cents, non-harmonic noise -53 / -50 / -49 dB. Spec 5 at AGE 50: sd >= 2 x AGE
 * 0's (4.50) and noise >= AGE 0's + 6 dB (-47). A host sine has no vibrato and no floor: the targets are TAPE's own
 * share, AGE 0's taken out (in quadrature / in power):
 *   sd     sqrt(4.50^2 - 2.25^2) = 3.90 cents          -> AGE50_SD_MIN 4.0
 *   noise  10 log10(10^-4.7 - 10^-5.3) = -48.3 dB     -> AGE50_NZ_MIN -48.0
 * The bridge between the two trackers: 0.8's AGE 100 row (WOW 100, FLUTTER 50, HISS 50) had 6.58 cents and -51.2 dB of
 * TAPE's own on the device (its AGE 50 reading, 0.89 cents of its own, is far under what the same WOW gives here: the
 * device's window under-reads a slow wow), so AGE 50 must also wobble at least as that row does here and hiss 3 dB more */
#define AGE50_SD_MIN 4.0
#define AGE50_NZ_MIN (-48.0)
static const uint8_t AGE_PT_08[3][5] = {{0, 0, 0, 0, 0}, {60, 20, 30, 20, 35}, {100, 50, 65, 50, 80}};
static const uint8_t AGE_PT_09[3][5] = {{0, 0, 0, 0, 0}, {100, 55, 45, 72, 50}, {100, 100, 85, 100, 90}};
#define AGE_NB (44100u * 20u / CTL)                     /* 20 s */
#define AGE_FFT 32768u
static double age_re[AGE_FFT], age_im[AGE_FFT], age_c[AGE_NB * CTL / 2205u + 2u];
static void age_fft(double *re, double *im, uint32_t n)   /* in place, radix 2 */
{
    uint32_t i, j = 0, k, m;
    for (i = 1; i < n; i++) {
        uint32_t b = n >> 1;
        for (; j & b; b >>= 1) j ^= b;
        j ^= b;
        if (i < j) { double t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; }
    }
    for (m = 2; m <= n; m <<= 1) {
        double a = -2.0 * M_PI / (double)m;
        for (k = 0; k < n; k += m)
            for (i = 0; i < m / 2u; i++) {
                uint32_t p = k + i, q = p + m / 2u;
                double wr = cos(a * i), wi = sin(a * i);
                double xr = re[q] * wr - im[q] * wi, xi = re[q] * wi + im[q] * wr;
                re[q] = re[p] - xr; im[q] = im[p] - xi;
                re[p] += xr; im[p] += xi;
            }
    }
}
/* TAPE's own wobble and noise on a held A3 (220 Hz, -12 dBFS), the five details d: the pitch's sd in cents over 50 ms
 * windows (20 s, the first 0.5 s left out, its mean removed); the non-harmonic share (dB of the whole, 50 Hz .. 16 kHz:
 * the bins further than 40 Hz from every harmonic; a Hann window of 32768 from 1 s); the share above 3 kHz */
static void age_measure(const uint8_t *d, double *sd, double *nz, double *hi)
{
    uint32_t n = AGE_NB * CTL, nw, i;
    double m = 0, v = 0, tot = 0, non = 0, up = 0;
    tape_reset();
    tape_g(d[0], d[1], d[2], d[3], d[4]);
    tape_tone(220, 8000, AGE_NB);
    nw = cents_of(out_l, n, 22050u, 2205u, 220, age_c);
    for (i = 0; i < nw; i++) m += age_c[i];
    m /= nw;
    for (i = 0; i < nw; i++) v += (age_c[i] - m) * (age_c[i] - m);
    *sd = sqrt(v / nw);
    for (i = 0; i < AGE_FFT; i++) {
        age_re[i] = out_l[44100u + i] * (0.5 - 0.5 * cos(2.0 * M_PI * i / AGE_FFT));
        age_im[i] = 0;
    }
    age_fft(age_re, age_im, AGE_FFT);
    for (i = 1; i < AGE_FFT / 2u; i++) {
        double hz = (double)i * FS / AGE_FFT, p = age_re[i] * age_re[i] + age_im[i] * age_im[i], h = fmod(hz, 220.0);
        if (hz < 50.0 || hz > 16000.0) continue;
        tot += p;
        if (h > 40.0 && h < 180.0) non += p;
        if (hz > 3000.0) up += p;
    }
    *nz = 10.0 * log10(non / tot + 1e-30);
    *hi = 10.0 * log10(up / tot + 1e-30);
}
static void test_age_09(void)
{
    char what[260];
    uint8_t d50[5], d100[5];
    int16_t m[5];
    double sdr, nzr, hir, sd5, nz5, hi5, sd50, nz50, hi50, sd100, nz100, hi100;
    uint32_t k;
    age_measure(AGE_PT_08[1], &sd5, &nz5, &hi5);       /* (0.8's AGE 50, for the record) */
    age_measure(AGE_PT_08[2], &sdr, &nzr, &hir);       /* (0.8's AGE 100: the bridge) */
    tape_age_map(50, m); for (k = 0; k < 5u; k++) d50[k] = (uint8_t)m[k];
    tape_age_map(100, m); for (k = 0; k < 5u; k++) d100[k] = (uint8_t)m[k];
    age_measure(d50, &sd50, &nz50, &hi50);
    age_measure(d100, &sd100, &nz100, &hi100);
    printf("tape: (0.9) host figures, sd cents / noise dB / above 3 kHz dB: 0.8 AGE 50 %.2f / %.1f / %.1f, 0.8 AGE 100 %.2f / "
           "%.1f / %.1f; 0.9 AGE 50 %.2f / %.1f / %.1f, AGE 100 %.2f / %.1f / %.1f\n",
           sd5, nz5, hi5, sdr, nzr, hir, sd50, nz50, hi50, sd100, nz100, hi100);
    snprintf(what, sizeof what, "(0.9) AGE 50: pitch sd %.2f cents (>= %.1f and >= 0.8's AGE 100 %.2f), noise %.1f dB (>= %.1f "
             "and >= %.1f + 3)", sd50, AGE50_SD_MIN, sdr, nz50, AGE50_NZ_MIN, nzr);
    ck(what, sd50 >= AGE50_SD_MIN && sd50 >= sdr && nz50 >= AGE50_NZ_MIN && nz50 >= nzr + 3.0);
    ck("(0.9) AGE 100 more worn than AGE 50 (sd, noise, TONE), AGE 50 darker than 0.8's (TONE >= 50)",
       sd100 >= sd50 && nz100 >= nz50 && d100[4] >= d50[4] && d50[4] >= 50u);
    ck("(0.9) the map pinned: TAPE_AGE_PT is the retuned one, row 0 all 0, each detail rising",
       !memcmp(TAPE_AGE_PT, AGE_PT_09, sizeof AGE_PT_09));
    tape_reset();
    tape_age_apply(0);
    tape_tone(220, 8000, 400);
    ck("(0.9) AGE 0: every detail 0, TAPE out of the chain, the output the input bit for bit",
       !tape_set() && !tape.on && !memcmp(out_l, in_l, 400u * CTL * sizeof out_l[0]));
    tape_reset();
}
```
  and in `main`, after `test_new_project_motion();`: `test_age_09();`.
- [ ] **Step 2: Run to see it fail** — `tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/tape_test tests/tape_test.c -lm && build/host/tape_test | grep -a -E "0.9|FAIL|checks"`.
  Expected: FAIL on "the map pinned" and on the AGE 50 figures (0.8's AGE 50 prints well under 0.8's AGE 100). Keep the
  printed host line for the commit message.
- [ ] **Step 3: Implement** — `fx.c`: replace `TAPE_AGE_PT`'s values with the Interfaces row; rewrite its comment:
  `(Braindancer 0.9) AGE's map, steeper (spec 5): at 50 a clearly worn tape (WOW at the line's limit, a quick flutter,
  hiss and highs off above 6.8 kHz), at 100 a mangled cassette; WOW reaches 100 at AGE 50 because the line (TAPE_LEN)
  holds 8.7 cents at most; measured by tests/tape_test.c test_age_09 (0.8: {60 20 30 20 35}, {100 50 65 50 80})`.
  If the AGE 50 check still fails, raise only row 1's FLUTTER, then HISS (never past row 2's value), keep `AGE_PT_09` in
  the test equal to the table, and report the final row.
- [ ] **Step 4: Run to see it pass** — the same command: `tape: all checks ok`. Then `tools/host-cc -O1 -w -Ibuild/gen
  -Ifirmware/src -Itests -o build/host/editor_test tests/editor_test.c -lm && build/host/editor_test | grep -a -E "FAIL|passed"`
  (build line as in `tests/run_tests.sh`), the audio safety test the same way; `./build.sh` (image, RAM);
  `python3 tests/target_budget.py` (every line unchanged); the suite (no golden line changes).
- [ ] **Step 5: Commit** — `git add firmware/src/fx.c tests/tape_test.c && git commit` with
  `fix(fx): AGE retuned: 50 a clearly worn tape, 100 mangled (map pinned by a host measurement)`; body: the printed
  host figures line, the derivation (pre-flight 13), the final row.

---

### Task 2: The identity, the style picker and blends

**Files:** Modify `firmware/src/gen_tables.c` (after `gen_shown_tempo` `:632`: the partner table and the AUTO AGE
table), `firmware/src/gen.c` (`gp` `:31-40`, `gseed` `:47-54`, `gs` `:73-91`, `gseed_mix` `:93`, `gmut_name` `:95`,
`gform_hash` / `gform_order` `:1673-1699`, `gen_parts` `:1704-1706`, `gc_budget` / `gc_cost` `:2065-2108`, `gen_pick`
`:2329-2330`, `gen_make` `:2402-2462`, `gen_go` `:2641-2663`, `dice_seq` `:2724, 2733, 2770`, `gen_panel` `:2911`),
`tests/gen_test.c`. Test `tests/gen_test.c`.

**Interfaces — Produces:**
```c
/* gen_tables.c */
#define GEN_WILD_PCT 10u                                /* a GENERATE press: WILD this often (%) */
#define GEN_BLEND_PCT 45u                               /* .. else a blend this often (%) */
#define GEN_MAXPART 6u
static const uint8_t GEN_PARTNER[GEN_NSHOWN][GEN_MAXPART];   /* spec 2's table, GEN_SHOWN indices */
static const uint8_t GEN_NPARTNER[GEN_NSHOWN];               /* {6, 4, 4, 2, 3, 4, 3, 2, 3, 3} */
static const uint8_t GEN_AUTO_AGE[GEN_NSHOWN];               /* {20, 0, 0, 0, 0, 0, 15, 60, 40, 25} (used in Task 3) */
#define GEN_AGE_MAX 70u
#define GEN_AGE_WILD 50u
/* gen.c, before gp */
#define GID_WILD 0xFEu                                  /* main: WILD (EXPLORE's generator, no template) */
#define GID_SOLO 0xFFu                                  /* part: none */
#define GT_AUTO 0u                                      /* the TAPE knob: AUTO, OFF, then 2..11 = AGE 10..100 */
#define GT_OFF 1u
#define GT_N 12u
typedef struct {
    uint16_t seed : 14, two : 1, : 1;                   /* 0..9999; made with two cores (mc_two()) */
    uint16_t dev : 9, form : 2, tape : 4, : 1;          /* the CPU factor as made (gc_dev() 100..400); FORM 0..2; GT_* */
    uint8_t main, part;                                 /* GENRE indices; GID_WILD; GID_SOLO */
    uint8_t energy, complex;                            /* 0..100 */
} gid_t;
_Static_assert(sizeof(gid_t) == 8u, "gid_t: 8 bytes (the 8 recent pieces: 64)");
/* gp gains (designated initializers: .genre = GEN_EXPLORE, .seed = 1, .energy = 50, .form = 1, .complex = 50,
 * .tape = GT_AUTO, .amt = {GA_AUTO x5}, .swing = GA_AUTO, .human = 30) */
    uint8_t tape;                                       /* (0.9) the TAPE knob, GT_* */
    gid_t id;                                           /* (0.9) the piece being made / made last */
    char last[28];                                      /* its name: "RETROBOC+AMBIENT 0421", "WILD 0421" */
/* gs gains */
    uint8_t pi;                                         /* (0.9) the partner (GENRE index) or GID_SOLO */
/* gen.c */
static struct { uint16_t dev; uint8_t two; } gcf;       /* the identity's CPU factor: gc_budget / gc_cost read it */
static uint32_t gid_form_key(const gid_t *id);          /* pre-flight 6 */
static uint32_t gform_order(const gid_t *id, uint32_t form, uint8_t *ord, uint8_t *mul);   /* (was: no id) */
static void gen_blend(ggenre_t *m, uint32_t pi);        /* pre-flight 4 */
static uint32_t gid_r(uint32_t *s);                     /* a xorshift step on *s */
static void gid_pick(gid_t *id, uint32_t *s, uint32_t prev);   /* main / part / WILD drawn; never main == prev */
static void gid_now(gid_t *id);                         /* the page's knobs, gp.seed, the device now; WILD, SOLO */
static void gid_legacy(gid_t *id);                      /* gp.genre (pre-flight 7) as an identity */
static void gid_name(const gid_t *id, char *b, uint32_t n, int card);   /* "RETROBOC+AMBIENT 0421" (card: short names) */
static void gen_build(const gid_t *id);                 /* the one way a piece is made: UNDO, gen_make, name, message */
static void gen_go(void);                               /* (kept) gid_legacy + gen_build */
static void gen_draw(gid_t *id);                        /* a GENERATE press's identity: gen_new_seed, gid_now, gid_pick */
```

- [ ] **Step 1: Write the failing tests** (`tests/gen_test.c`). Helpers after `all_sig` (`:65`):

```c
/* (0.9) a piece's parts as played, and with TAPE: the steps and engines, the delay, the reverb, the grain, the parts
 * played, the PLAYHEADs; piece_sig adds the swing and TAPE's six */
static uint32_t parts_sig(void)
{
    uint32_t h = all_sig() ^ (uint32_t)song.g[G_DTIME] * 0x9e3779b9u ^ (uint32_t)song.g[G_RTYPE] << 20 ^
                 (uint32_t)(song.g[G_DPIT] != 0) << 27 ^ (uint32_t)(song.g[G_DSPRY] != 0) << 28, k, q;
    for (q = 0; q < 6u; q++) h = h * 31u + (gs.act[q] != 0);
    for (k = 0; k < NTRK; k++) for (q = 0; q < FG_HEADS; q++) h = h * 3u + (fg_set[k][q].rate != 0);
    return h;
}
static uint32_t piece_sig(void)
{
    uint32_t h = parts_sig(), k;
    h = (h ^ (uint32_t)(uint16_t)song.g[G_SWING]) * 16777619u;
    for (k = G_TAGE; k <= G_TTONE; k++) h = (h ^ (uint32_t)(uint16_t)song.g[k]) * 16777619u;
    return h;
}
static uint32_t gt_style(const char *name)              /* a shown style's GENRE index by its screen name */
{
    uint32_t i;
    for (i = 0; i < GEN_NSHOWN; i++) if (!strcmp(GEN_SHOWN_NAME[i], name)) return GEN_SHOWN[i];
    printf("gen: no style %s\n", name);
    return GID_SOLO;
}
static void gid_set(gid_t *id, uint32_t main, uint32_t part, uint32_t seed)
{
    gid_now(id);
    id->main = (uint8_t)main; id->part = (uint8_t)part; id->seed = (uint16_t)(seed % 10000u);
}
/* spec 2's table, by name: main, then its partners */
static const char *const PT[GEN_NSHOWN][7] = {
    {"IDM", "GLITCH HOP", "MICRO", "RETROBOC", "AMBIENT", "DRILL'N'BASS", "ELECTRO"},
    {"GLITCH HOP", "IDM", "BREAKS", "MICRO", "ELECTRO"}, {"BREAKS", "GLITCH HOP", "DRILL'N'BASS", "ACID", "ELECTRO"},
    {"DRILL'N'BASS", "IDM", "BREAKS"}, {"ACID", "BREAKS", "ELECTRO", "AMB TECHNO"},
    {"ELECTRO", "IDM", "GLITCH HOP", "BREAKS", "ACID"}, {"MICRO", "IDM", "GLITCH HOP", "AMB TECHNO"},
    {"RETROBOC", "IDM", "AMBIENT"}, {"AMBIENT", "IDM", "RETROBOC", "AMB TECHNO"}, {"AMB TECHNO", "ACID", "MICRO", "AMBIENT"}};
static int pt_ok(uint32_t main, uint32_t part)          /* part one of main's partners in spec 2's table */
{
    uint32_t i, j;
    for (i = 0; i < GEN_NSHOWN; i++)
        if (gt_style(PT[i][0]) == main)
            for (j = 1; j < 7u && PT[i][j]; j++) if (gt_style(PT[i][j]) == part) return 1;
    return 0;
}
```
  New block in `main`, after the deterministic block (`:136`):

```c
    {   /* (0.9 spec 2) GENERATE: the identity makes the piece; the picker; the blends */
        gid_t a, b, d;
        uint32_t h1, h2, h3, i, j, pr, wild = 0, blend = 0, rep = 0, badp = 0, nonw = 0, cnt[GEN_NSHOWN] = {0}, seenp = 0, ok;
        char nm[32];
        gen_reset();
        gid_set(&a, gt_style("RETROBOC"), gt_style("AMBIENT"), 421);
        gen_build(&a); h1 = piece_sig();
        ok = !strcmp(gp.last, "RETROBOC+AMBIENT 0421") && !strcmp(ui.msg, "GEN: RETROBOC+AMBIENT 0421") && gs.mutant;
        gid_set(&b, gt_style("DRILL'N'BASS"), GID_SOLO, 77); gen_build(&b);
        gen_build(&a); h2 = piece_sig();
        d = a; d.part = GID_SOLO; gen_build(&d); h3 = piece_sig();
        bad += gcheck("(0.9) the same identity twice the same piece (RETROBOC+AMBIENT 0421, another between); solo another", h1 == h2 && h1 != h3);
        bad += gcheck("  named on the panel and the status line: RETROBOC+AMBIENT 0421; solo RETROBOC 0421", ok && !strcmp(gp.last, "RETROBOC 0421"));
        d.main = GID_WILD; gen_build(&d);
        bad += gcheck("  WILD: the EXPLORE generator, named WILD 0421", gx.on && !strcmp(gp.last, "WILD 0421"));
        for (ok = 1, i = 0; i < GEN_NSHOWN; i++) {      /* the firmware's table is spec 2's */
            uint32_t n = 0;
            for (j = 1; j < 7u && PT[i][j]; j++) n++;
            ok &= GEN_NPARTNER[gen_shown_index(gt_style(PT[i][0]) + 1u)] == n;
            for (j = 0; j < GEN_NPARTNER[i]; j++) ok &= pt_ok(GEN_SHOWN[i], GEN_SHOWN[GEN_PARTNER[i][j]]);
        }
        bad += gcheck("  GEN_PARTNER is spec 2's table (every row, every partner)", ok);
        gen_reset();
        for (pr = 0; pr < 2000u; pr++) {                /* 2,000 presses' draws (the identity only, then as if built) */
            gid_t id;
            uint32_t prev = gp.id.main, si;
            host_ticks += 1300000u + pr * 777u;
            gen_draw(&id);
            if (id.main == GID_WILD) { wild++; badp += id.part != GID_SOLO; gp.id = id; continue; }
            nonw++;
            si = gen_shown_index(id.main + 1u);
            if (si >= GEN_NSHOWN) { badp++; continue; }
            cnt[si]++;
            rep += id.main == prev;
            if (id.part != GID_SOLO) { blend++; badp += !pt_ok(id.main, id.part); seenp |= 1u << gen_shown_index(id.part + 1u); }
            gp.id = id;
        }
        for (ok = 1, i = 0; i < GEN_NSHOWN; i++) ok &= cnt[i] * 100u >= nonw * 7u && cnt[i] * 100u <= nonw * 13u;
        printf("gen: (0.9) 2000 presses: WILD %u, blends %u of %u, the same main twice running %u, outside the table %u\n",
               wild, blend, nonw, rep, badp);
        bad += gcheck("  WILD about 10 % (7..13 %), never blended", wild >= 140u && wild <= 260u);
        bad += gcheck("  blends about 45 % of the rest (40..50 %), partners only from the table, every style a partner", blend * 100u >= nonw * 40u &&
                      blend * 100u <= nonw * 50u && !badp && seenp == (1u << GEN_NSHOWN) - 1u);
        bad += gcheck("  the main evenly (7..13 % each), never the same main as the press before", ok && !rep);
        for (ok = 1, i = 0; i < GEN_NSHOWN; i++)       /* a blend: the main's tempo, swing and drums, the partner's bass */
            for (j = 0; j < GEN_NPARTNER[i]; j++) {
                uint32_t mi = GEN_SHOWN[i], pa = GEN_SHOWN[GEN_PARTNER[i][j]];
                gid_set(&d, mi, pa, 100u * i + j + 3u); gen_build(&d);
                ok &= song.g[G_BPM] >= GEN_SHOWN_TEMPO[i][0] - 3 && song.g[G_BPM] <= GEN_SHOWN_TEMPO[i][1] + 3 &&
                      song.g[G_SWING] == GEN_SHOWN_TEMPO[i][2] && gs.g->kit == GENRE[mi].kit && gs.g->bass_lo == GENRE[pa].bass_lo &&
                      gs.g->kick_mode == GENRE[pa].kick_mode && gs.mutant;
                gid_name(&d, nm, sizeof nm, 1);         /* (the short form: the fallback, pre-flight 17) */
                ok &= text_w(&UF_HDR, ui.msg) <= 232 && str_len(gp.last) + 1u < sizeof gp.last && strchr(gp.last, '+') &&
                      (strstr(gp.last, GEN_SHOWN_NAME[i]) == gp.last || !strcmp(gp.last, nm));
            }
        bad += gcheck("  every blend: the main's tempo, swing and kit, the partner's bass; its name whole and on the status line", ok);
        {   /* the CPU factor is the identity's: rebuilt after CPU TEST changed, the same piece and budget */
            uint32_t b0, s0;
            mc_host_v12 = 1; gt_cpt = 74; gen_reset();
            gid_set(&d, gt_style("IDM"), gt_style("MICRO"), 9641); gen_build(&d); s0 = piece_sig(); b0 = gc_budget();
            cpt_res = 0; cpt_cores = 0;
            gen_build(&d);
            bad += gcheck("  the CPU factor in the identity: CPU TEST forgotten, the same piece and budget rebuilt", piece_sig() == s0 &&
                          gc_budget() == b0 && gc_dev() != d.dev);
            mc_host_v12 = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
        }
        {   /* the tools' way in: gen_go with gp.genre 0 draws from the seed (the same seed, the same draw) */
            uint32_t s1, s2;
            gen_reset();
            gen_with(0, 31, 50, 0); s1 = piece_sig(); a = gp.id;
            gen_with(1, 7, 50, 0);
            gen_with(0, 31, 50, 0); s2 = piece_sig();
            bad += gcheck("  gen_go, genre 0 (the tools'): a draw from the seed, deterministic", s1 == s2 && gid_eq_t(&a, &gp.id));
        }
    }
```
  (`gc_dev() != d.dev`: CPU TEST 74 % gives the factor ×1.17 · 1.5, unset ×1.9: the live factor did change.) Add
  `gid_eq_t` before the block:
  `static int gid_eq_t(const gid_t *a, const gid_t *b) { return a->seed == b->seed && a->main == b->main && a->part == b->part && a->energy == b->energy && a->complex == b->complex && a->form == b->form && a->tape == b->tape && a->dev == b->dev && a->two == b->two; }`.
  Adapt the old picker checks: `:1016-1020` (`gen_random_genre` gone) becomes
  `for (sd = 0; sd < 2500u; sd++) { gen_with(0, sd, 50, 0); hid += gp.id.main != GID_WILD && gen_shown_index(gs.gi + 1u) == 255u; hid += gp.id.part != GID_SOLO && gen_shown_index(gp.id.part + 1u) == 255u; }`
  (label `(0.9) seed draws over 2,500 seeds: only the ten`); the mutant-name block `:1061-1080` counts
  `gp.id.part != GID_SOLO`, splits `gp.last` at `+` and accepts `GEN_SHOWN_NAME` **or** `GEN_SHOWN_CARD` names (pre-flight
  17), `nm > 400u` (45 % of 90 %), and drops the `GEN_LAST_W` / `gen_last_line` term until Task 5 replaces the panel
  (keep `text_w(&UF_HDR, ui.msg) <= 232`).
- [ ] **Step 2: Run to see it fail** — build and run `gen_test`. Expected: compile errors (`gid_t`, `gen_build`, `gid_now`,
  `gen_draw`, `gid_name`, `GEN_PARTNER` unknown).
- [ ] **Step 3: Implement.** `gen_tables.c` after `gen_shown_tempo`:

```c
/* (Braindancer 0.9) GENERATE (gen.c gid_pick): WILD (EXPLORE, no template) GEN_WILD_PCT of the presses, else a main
 * style of the ten, never the press before's, blended GEN_BLEND_PCT of the time with a partner from its row (spec 2's
 * table, GEN_SHOWN indices); TAPE on AUTO (gen.c gen_age): a style's AGE, a blend the main's + half the partner's
 * (GEN_AGE_MAX at most), WILD 0..GEN_AGE_WILD from its seed */
#define GEN_WILD_PCT 10u
#define GEN_BLEND_PCT 45u
#define GEN_MAXPART 6u
static const uint8_t GEN_PARTNER[GEN_NSHOWN][GEN_MAXPART] = {
    {1, 6, 7, 8, 3, 5},                                 /* IDM: GLITCH HOP, MICRO, RETROBOC, AMBIENT, DRILL'N'BASS, ELECTRO */
    {0, 2, 6, 5},                                       /* GLITCH HOP: IDM, BREAKS, MICRO, ELECTRO */
    {1, 3, 4, 5},                                       /* BREAKS: GLITCH HOP, DRILL'N'BASS, ACID, ELECTRO */
    {0, 2},                                             /* DRILL'N'BASS: IDM, BREAKS */
    {2, 5, 9},                                          /* ACID: BREAKS, ELECTRO, AMB TECHNO */
    {0, 1, 2, 4},                                       /* ELECTRO: IDM, GLITCH HOP, BREAKS, ACID */
    {0, 1, 9},                                          /* MICRO: IDM, GLITCH HOP, AMB TECHNO */
    {0, 8},                                             /* RETROBOC: IDM, AMBIENT */
    {0, 7, 9},                                          /* AMBIENT: IDM, RETROBOC, AMB TECHNO */
    {4, 6, 8},                                          /* AMB TECHNO: ACID, MICRO, AMBIENT */
};
static const uint8_t GEN_NPARTNER[GEN_NSHOWN] = {6, 4, 4, 2, 3, 4, 3, 2, 3, 3};
static const uint8_t GEN_AUTO_AGE[GEN_NSHOWN] = {20, 0, 0, 0, 0, 0, 15, 60, 40, 25};
#define GEN_AGE_MAX 70u
#define GEN_AGE_WILD 50u
```
  `gen.c`: the `gid_t` block (Interfaces) before `gp`; `gp` with designated initializers and the new fields (keep
  `genre`, `seed`, `amt`, `swing`, `human`, `page`, `hot`, `sig` for now: Task 5 removes `page`, `sig`); `gs.pi`; `gcf`.
  Streams and hashes:

```c
static void gseed(uint32_t part)                        /* (0.9: the identity's seed) */
{
    gq = ((uint32_t)gp.id.seed + 1u) * 2654435761u ^ (part + 1u) * 0x9E3779B9u ^ 0x6D2B79F5u ^ gseed_mix(part);
    if (!gq) gq = 1;
    gr(); gr(); gr();
}
static uint32_t gseed_mix(uint32_t part)                /* (0.9) a blend's partner in every stream but GS_PICK's */
{
    return part == GS_PICK ? 0u : (gs.gi + 1u) * 0x85EBCA6Bu ^ (gs.pi != GID_SOLO ? (gs.pi + 1u) * 0xC2B2AE35u : 0u);
}
static uint32_t gid_form_key(const gid_t *id)           /* gform_hash's style: 0.8's gp.genre for a solo style and WILD */
{
    return id->main == GID_WILD ? GEN_EXPLORE : id->part == GID_SOLO ? id->main + 1u : (id->main + 1u) | (id->part + 1u) << 8;
}
static uint32_t gform_hash(const gid_t *id)
{
    uint32_t h = 2166136261u;
    h = (h ^ id->seed) * 16777619u; h = (h ^ gid_form_key(id)) * 16777619u; h = (h ^ id->form) * 16777619u;
    h ^= h >> 13; h *= 2246822519u; h ^= h >> 16;
    return h;
}
```
  `gform_order(const gid_t *id, uint32_t form, uint8_t *ord, uint8_t *mul)` calls `gform_hash(id)`; `gen_parts` passes
  `&gp.id` and reads `gp.id.form` (`:1704`, `:1706`); `gen_panel` (`:2911`) passes `&gp.id` for now. `gen_pick`:
  `gs.energy = dice_force_energy >= 0 ? (uint32_t)dice_force_energy * 50u : gp.id.energy; gs.complex = gp.id.complex;`.
  `dice_seq`: save / restore `gp.id.seed` instead of `gp.seed` (`:2724`, `:2730`, `:2764`, `:2770`) and set
  `gs.pi = GID_SOLO;` next to `gs.mutant = 0;` (`:2733`). `gc_dev` stays; `gc_budget` and `gc_cost` read `gcf`:

```c
static uint32_t gc_budget(void) { return gcf.two ? 1030u * 100u / gcf.dev : 714u * 100u / gcf.dev - 120u; }
/* gc_cost: every mc_two() in it -> gcf.two (the base 295 / 380, GC_DLO's index) */
```
  The blend (replaces the mutant block `gen.c:2424-2455`):

```c
/* (Braindancer 0.9) a blend (spec 2): the 0.6.95 mutant's code with one partner. The main keeps its drums, tempo range and
 * swing ("the main style sets tempo and feel"); the partner gives the bass, and the chords, the arp, the lead and the
 * space each half the time (the GS_PICK stream's four draws, as the mutant's), the scale's second and fourth choices and
 * the texture. gs.mutant: COMPLEX + 20, an arp or a lead in another metre half the time, as before */
static void gen_blend(ggenre_t *m, uint32_t pi)
{
    const ggenre_t *c = &GENRE[pi % NGENRE];
    uint32_t k;
    for (k = 0; k < 3u; k++) m->bass[k] = c->bass[k];
    m->bass_lo = c->bass_lo; m->kick_mode = c->kick_mode; m->bass_gate = c->bass_gate; m->snd[GP_BASS] = c->snd[GP_BASS];
    if (gch(50)) {
        for (k = 0; k < 3u; k++) m->prog[k] = c->prog[k];
        m->cbars = c->cbars; m->pads = c->pads; m->voicing = c->voicing; m->snd[GP_CHORDS] = c->snd[GP_CHORDS];
        m->hx = c->hx;
    }
    if (gch(50)) { m->arp[0] = c->arp[0]; m->arp[1] = c->arp[1]; m->snd[GP_ARP] = c->snd[GP_ARP]; }
    if (gch(50)) { m->lead = c->lead; m->snd[GP_LEAD] = c->snd[GP_LEAD]; }
    if (gch(50)) { m->rtype = c->rtype; m->dtime = c->dtime; m->wet = c->wet; m->dtype = c->dtype; }
    m->scale[1] = m->scale[3] = c->scale[0];
    m->tex = c->tex;
}
```
  `gen_make`'s head (`:2401-2463`) becomes:

```c
    gcf.dev = gp.id.dev >= 100u ? (uint16_t)gp.id.dev : (uint16_t)gc_dev();   /* (0.9) the identity's CPU factor */
    gcf.two = (uint8_t)gp.id.two;
    gs.pi = gp.id.part != GID_SOLO && gp.id.main != GID_WILD ? gp.id.part : (uint8_t)GID_SOLO;
    gs.gi = gp.id.main == GID_WILD ? NGENRE : gp.id.main % NGENRE;   /* (before any stream: gseed_mix reads them) */
    gseed(GS_PICK);
    gx.on = (uint8_t)(gp.id.main == GID_WILD);
    gx.nvo = gx.np = 0; gx.eng = -1; gx.w = gp.id.complex; gx.nmax = 4;
    /* .. the !gx.on && gx.dirty reset and the EXPLORE branch as before (gs.gi = NGENRE already set) .. */
    } else {
        gmut = GENRE[gs.gi];
        gen_shown_tempo(&gmut, gs.gi + 1u);            /* (0.5.1) the shown genre's tempo and swing */
        gs.mutant = 0;
        if (gs.pi != GID_SOLO) { gen_blend(&gmut, gs.pi); gs.mutant = 1; }
        /* .. "a throw its own" (DM_ALT, CS_ALT, SR_ALT) as before .. */
        g = gs.g = &gmut;
    }
```
  Delete `gen_random_genre` and `gmut_name` (no reader left; if `gx_genre` writes `gmut.name`, leave that). The identity
  functions and the builder (after `gen_make`, replacing `gen_go` `:2641-2663`):

```c
static uint32_t gid_r(uint32_t *s) { *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5; return *s; }
/* (0.9 spec 2) a style drawn: WILD GEN_WILD_PCT of the time; else a main of the ten, evenly, never prev (the piece
 * built last: not part of any identity, pre-flight 3); then GEN_BLEND_PCT of the time a partner from its row */
static void gid_pick(gid_t *id, uint32_t *s, uint32_t prev)
{
    uint32_t p = prev < NGENRE ? gen_shown_index(prev + 1u) : 255u, i;
    if (!*s) *s = 0x9E3779B9u;
    id->part = GID_SOLO;
    if (gid_r(s) % 100u < GEN_WILD_PCT) { id->main = GID_WILD; return; }
    i = gid_r(s) % (p < GEN_NSHOWN ? GEN_NSHOWN - 1u : GEN_NSHOWN);
    if (p < GEN_NSHOWN && i >= p) i++;
    id->main = GEN_SHOWN[i];
    if (gid_r(s) % 100u < GEN_BLEND_PCT) id->part = GEN_SHOWN[GEN_PARTNER[i][gid_r(s) % GEN_NPARTNER[i]]];
}
static void gid_now(gid_t *id)                          /* the page's knobs and the device now; WILD, no partner */
{
    memset(id, 0, sizeof *id);
    id->seed = (uint16_t)(gp.seed % 10000u); id->energy = gp.energy; id->form = (uint16_t)(gp.form % 3u);
    id->complex = gp.complex; id->tape = (uint16_t)(gp.tape % GT_N);
    id->two = mc_two() ? 1u : 0u; id->dev = (uint16_t)gc_dev();
    id->main = GID_WILD; id->part = GID_SOLO;
}
static void gid_legacy(gid_t *id)                       /* (the tools and the tests) gp.genre as an identity */
{
    gid_now(id);
    if (dice_force_style >= 0) { id->main = (uint8_t)((uint32_t)dice_force_style % NGENRE); return; }
    if (gp.genre == GEN_EXPLORE) return;
    if (gp.genre) { id->main = (uint8_t)((gp.genre - 1u) % NGENRE); return; }
    {   uint32_t s = ((uint32_t)gp.seed + 1u) * 2654435761u ^ 0x5BD1E995u;   /* (RANDOM: a draw from the seed) */
        gid_pick(id, &s, GID_SOLO); }
}
static void gid_part_name(uint32_t g, char *b, uint32_t n, int card)
{
    uint32_t i = g == GID_WILD ? 255u : gen_shown_index(g + 1u);
    str_cpy(b, g == GID_WILD ? "WILD" : i < GEN_NSHOWN ? (card ? GEN_SHOWN_CARD[i] : GEN_SHOWN_NAME[i]) : GENRE[g % NGENRE].name, n);
}
static void gid_name(const gid_t *id, char *b, uint32_t n, int card)
{
    uint32_t k;
    gid_part_name(id->main, b, n, card);
    if (id->part != GID_SOLO && id->main != GID_WILD && (k = str_len(b)) + 2u < n) {
        b[k++] = '+';
        gid_part_name(id->part, b + k, n - k, card);
    }
    k = str_len(b);
    if (k + 6u <= n) {
        b[k] = ' '; b[k + 1] = (char)('0' + id->seed / 1000u % 10u); b[k + 2] = (char)('0' + id->seed / 100u % 10u);
        b[k + 3] = (char)('0' + id->seed / 10u % 10u); b[k + 4] = (char)('0' + id->seed % 10u); b[k + 5] = 0;
    }
}
/* (0.9) the one way a piece is made: UNDO marked, the piece from its identity, its name, the message */
static void gen_build(const gid_t *id)
{
    char m[40];
    cpt_clock_back();
    undo_mark();
    gp.id = *id;
    gen_make();
    safe_part[0] = gs.act[RL_TEX] && gs.trk_of[RL_TEX] < trk_live ? gs.trk_of[RL_TEX] : (uint8_t)NTRK;   /* (as gen_go) */
    safe_part[1] = gs.act[RL_ARP] && gs.trk_of[RL_ARP] < trk_live ? gs.trk_of[RL_ARP] : (uint8_t)NTRK;
    gid_name(&gp.id, gp.last, sizeof gp.last, 0);
    str_cpy(m, "GEN: ", sizeof m); str_cpy(m + 5, gp.last, sizeof m - 5);
    if (text_w(&UF_HDR, m) > 232) gid_name(&gp.id, gp.last, sizeof gp.last, 1);   /* (too wide: the short names) */
    ui_say("GEN: ", gp.last);
    str_cpy(ui.msg2, live.mode ? "PARTS: PLAY THE SONG" : song.g[G_CLOCK] == 1 ? "CLOCK: USB" : song.g[G_CLOCK] ? "CLOCK: TRS" :
            "UNDO: GLO + G4", sizeof ui.msg2);
    ui.force = 1;
}
static void gen_go(void) { gid_t id; gid_legacy(&id); gen_build(&id); }
static void gen_draw(gid_t *id)                         /* a GENERATE press: a fresh seed, the page, a style drawn */
{
    uint32_t s;
    gen_new_seed();
    gid_now(id);
    s = rng() | 1u;
    gid_pick(id, &s, gp.id.main);
}
```
  `gen_new_seed` and `gen_press` stay as they are in this task (OCT+ still runs the old `gp.genre` path; Task 5 rewires
  it). `gen_new_seed` must be declared before `gen_draw` if it is defined after it (move `gen_draw` below it).
- [ ] **Step 4: Run to see it pass** — `gen_test`: the new block green; `GEN_HOUSE_HASH` (`:1027`) **unchanged** (a solo
  style's streams are 0.8's; if it moved, the stream mixing or the block order is wrong: fix, never re-record here);
  `ui_test`, `stems_test`, `hush_test` built and green as in `run_tests.sh`; `./build.sh`; the suite.
- [ ] **Step 5: Commit** — `git add firmware/src/gen.c firmware/src/gen_tables.c tests/gen_test.c && git commit` with
  `feat(gen): a piece's identity; GENERATE's picker (WILD, the ten, no repeat) and blends from the compatibility table`.

---

### Task 3: Every piece fitted, TAPE counted, AUTO AGE per style

**Files:** Modify `firmware/src/gen.c` (`GS_*` enum `:66`, `gs` `:73-91`, the budget comment and `GC_TAPE1/2` comment
`:1993-2031`, `gc_cost` `:2070-2108`, `gen_fit_rest` `:2179`, `gen_make` `:2463`, `:2630-2637`), `tests/gen_test.c`
(`:117` hash, `:1123-1225`, `:1258-1295`), `tests/stems_test.c:40-46`, `tests/hush_test.c:58-61`.

**Interfaces — Consumes:** `gid_t`, `gp.id`, `gcf`, `gen_build`, `gid_set`, `gt_style`, `parts_sig`, `piece_sig` (Task 2);
`GEN_AUTO_AGE`, `GEN_AGE_MAX`, `GEN_AGE_WILD`, `GT_*` (Task 2); `tape_age_apply`, `tape_age_map`, `tape_set` (fx.c).
**Produces:**
```c
enum { GS_PICK, GS_DRUMS, GS_BASS, GS_CHORDS, GS_ARP, GS_LEAD, GS_SOUND, GS_FORM, GS_MIX, GS_TEX, GS_PLAN, GS_TAPE };
/* gs gains */
    uint8_t age, age_cut;                               /* (0.9) the piece's AGE (gen_age); AUTO lowered it (gen_fit_tape) */
    uint16_t cost_thin;                                 /* (0.9) gc_cost(1) after the thinning, before AGE was looked at */
static uint32_t gen_age(void);                          /* the AGE gp.id asks for: the knob's, or AUTO the style's */
static void gen_fit_tape(void);                         /* AUTO and still over after gx_fit_piece: AGE down 10 at a time */
```

- [ ] **Step 1: Write the failing tests** (`tests/gen_test.c`). Delete the 0.8 block `:1258-1295` ("GEN doesn't pay for
  TAPE") and the "(review I2) RANDOM and the genres keep their parts" loop and check (`:1185-1213`, its `r_*` counters
  and printf). Add after the budget block:

```c
    {   /* (0.9 spec 3) TAPE on AUTO: the style's AGE; a blend the main's + half the partner's, 70 at most; WILD 0..50 from
         * its seed. On a budget every piece fits (CPU TEST 63 %: the model's x1.00, two cores) */
        static const struct { const char *m, *p; uint8_t age; } AA[] = {
            {"RETROBOC", 0, 60}, {"AMBIENT", 0, 40}, {"AMB TECHNO", 0, 25}, {"IDM", 0, 20}, {"MICRO", 0, 15},
            {"ACID", 0, 0}, {"ELECTRO", 0, 0}, {"BREAKS", 0, 0}, {"GLITCH HOP", 0, 0}, {"DRILL'N'BASS", 0, 0},
            {"RETROBOC", "AMBIENT", 70}, {"IDM", "RETROBOC", 50}, {"AMBIENT", "IDM", 50}, {"ACID", "AMB TECHNO", 12},
            {"MICRO", "IDM", 25}, {"BREAKS", "ACID", 0}};
        uint8_t seen[GEN_AGE_WILD + 1u] = {0};
        uint32_t n, ok = 1, cut = 0, sd, a, lo = 100, hi = 0, nseen = 0, w1;
        gid_t id;
        int16_t d[5];
        mc_host_v12 = 1; gt_cpt = 63; gen_reset();
        for (n = 0; n < NELEM(AA); n++) {
            gid_set(&id, gt_style(AA[n].m), AA[n].p ? gt_style(AA[n].p) : GID_SOLO, 300u + n);
            gen_build(&id);
            tape_age_map(AA[n].age, d);
            cut += gs.age_cut;
            if (!gs.age_cut && (song.g[G_TAGE] != AA[n].age || memcmp(&song.g[G_TWOW], d, sizeof d))) {
                ok = 0; printf("  %s: AGE %d, want %u\n", gp.last, song.g[G_TAGE], AA[n].age);
            }
        }
        bad += gcheck("(0.9) AUTO AGE: RETROBOC 60, AMBIENT 40, AMB TECHNO 25, IDM 20, MICRO 15, the rest 0; blends main + half, <= 70", ok && !cut);
        for (sd = 0; sd < 200u; sd++) {
            gid_set(&id, GID_WILD, GID_SOLO, sd * 37u + 1u); gen_build(&id);
            a = (uint32_t)song.g[G_TAGE];
            if (gs.age_cut) continue;
            lo = a < lo ? a : lo; hi = a > hi ? a : hi;
            if (a <= GEN_AGE_WILD && !seen[a]) { seen[a] = 1; nseen++; }
        }
        gid_set(&id, GID_WILD, GID_SOLO, 41); gen_build(&id); w1 = (uint32_t)song.g[G_TAGE];
        gid_set(&id, GID_WILD, GID_SOLO, 42); gen_build(&id);
        gid_set(&id, GID_WILD, GID_SOLO, 41); gen_build(&id);
        bad += gcheck("  WILD: AGE 0..50 from its seed (20 values at least over 200 seeds), the same seed the same AGE",
                      hi <= GEN_AGE_WILD && nseen >= 20u && (uint32_t)song.g[G_TAGE] == w1);
        {   /* gc_cost counts TAPE while the piece's AGE is above 0 (two cores: GC_TAPE2; one: GC_TAPE1) */
            uint32_t e0, e1, p0, p1, c;
            for (c = 0; c < 2u; c++) {
                mc_host_v12 = (uint8_t)c; mc_off = (uint8_t)!c; gt_cpt = 63; gen_reset();
                gp.tape = GT_OFF; gid_set(&id, gt_style("ACID"), GID_SOLO, 1234); gen_build(&id); e0 = gc_cost(1); p0 = parts_sig();
                id.tape = 6u; gen_build(&id); e1 = gc_cost(1); p1 = parts_sig();   /* (50) */
                bad += gcheck(c ? "  TAPE counted, two cores: AGE 50 costs GC_TAPE2 more, the same parts (under the budget)" :
                                  "  TAPE counted, one core: AGE 50 costs GC_TAPE1 more, the same parts (under the budget)",
                              p0 == p1 && e1 == e0 + (c ? GC_TAPE2 : GC_TAPE1) && e1 <= gc_budget() && song.g[G_TAGE] == 50);
            }
            gp.tape = GT_AUTO;
        }
        mc_host_v12 = 0; mc_off = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
    {   /* the TAPE knob: OFF -> AGE 0 on every piece whatever was set (UNDO brings it back); an amount -> that AGE and its
         * five, never lowered, even over the budget (one core, CPU TEST 74 %: the tightest) */
        uint32_t sd, okoff = 1, okset = 1, okundo = 1, over = 0;
        gid_t id;
        int16_t d[5];
        mc_off = 1; gt_cpt = 74; gen_reset();
        tape_age_map(70, d);
        for (sd = 0; sd < 60u; sd++) {
            tape_age_apply(50);
            gp.tape = GT_OFF; gid_set(&id, GEN_SHOWN[sd % GEN_NSHOWN], sd & 1u ? GEN_SHOWN[GEN_PARTNER[sd % GEN_NSHOWN][0]] : GID_SOLO, sd * 53u + 7u);
            gen_build(&id);
            okoff &= !song.g[G_TAGE] && !tape_set() && !gs.age_cut;
            if (sd == 0u) okundo = !undo_do() && song.g[G_TAGE] == 50 && tape_set();
            id.tape = 8u; gen_build(&id);                 /* (70) */
            okset &= song.g[G_TAGE] == 70 && !memcmp(&song.g[G_TWOW], d, sizeof d) && !gs.age_cut;
            over += gc_cost(1) > gc_budget();
        }
        gp.tape = GT_AUTO;
        printf("gen: (0.9) the TAPE knob at 70 on 60 pieces, one core, CPU TEST 74 %%: %u over the budget (the knob's AGE kept)\n", over);
        bad += gcheck("(0.9) TAPE OFF: AGE 0 and its five 0 on every piece (whatever was set); UNDO: TAPE back", okoff && okundo);
        bad += gcheck("  TAPE 70: AGE 70 and its five on every piece, never lowered", okset);
        mc_off = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
    {   /* every style, blends and WILD, CPU TEST unset and 74 %, one core and two, TAPE on AUTO: the estimate with TAPE
         * within the budget (4 % over at most: the fitter's floor, AGE already 0), never over with AGE on; AUTO lowered
         * AGE only after the thinning: still over then, and the same identity with the knob at 100 has the same parts */
        static const uint8_t CPT[2] = {0, 74};
        uint32_t c, n, pieces = 0, over = 0, over_on = 0, cuts = 0, cut_bad = 0;
        for (c = 0; c < 4u; c++)
            for (n = 0; n < 150u; n++) {
                gid_t id;
                uint32_t ps, s = n * 2654435761u + c + 1u;
                mc_host_v12 = (uint8_t)(c & 1u); mc_off = (uint8_t)!(c & 1u); gt_cpt = CPT[c >> 1]; gen_reset();
                gid_now(&id);
                gid_pick(&id, &s, GID_SOLO);
                id.seed = (uint16_t)((n * 89u + c * 7u + 5u) % 10000u); id.energy = (uint8_t)(30u + (n * 17u) % 60u);
                id.form = (uint16_t)(n % 3u); id.tape = GT_AUTO;
                gen_build(&id);
                pieces++;
                over += gc_cost(1) > gc_budget();
                over_on += gc_cost(1) > gc_budget() && song.g[G_TAGE];
                if (gs.age_cut) {
                    cuts++;
                    ps = parts_sig();
                    cut_bad += gs.cost_thin <= gc_budget();
                    id.tape = GT_N - 1u; gen_build(&id);   /* (the knob at 100) */
                    cut_bad += parts_sig() != ps || song.g[G_TAGE] != 100;
                }
            }
        printf("gen: (0.9) %u pieces (TAPE AUTO): %u over the budget, %u with AGE on; AUTO lowered AGE on %u\n", pieces, over, over_on, cuts);
        bad += gcheck("(0.9) every piece fitted with TAPE counted: over the budget 4 % at most, none with AGE on", over * 25u <= pieces && !over_on);
        bad += gcheck("  AUTO lowered AGE only after the thinning (still over then; the parts those of AGE kept)", cuts && !cut_bad);
        mc_host_v12 = 0; mc_off = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
```
  (`cuts && ` requires at least one cut; if the sweep has none, raise `id.energy` to `60u + (n * 17u) % 40u` for the
  CPU TEST 74 % one-core quarter and say so.)
- [ ] **Step 2: Run to see it fail** — `gen_test`: compile error `gs.age_cut` / `gs.cost_thin` unknown.
- [ ] **Step 3: Implement** (`gen.c`):

```c
/* (0.9 spec 3) the AGE the piece asks for: the TAPE knob's (OFF 0, 2..11: 10..100); AUTO the style's (GEN_AUTO_AGE), a
 * blend's main + half its partner's (GEN_AGE_MAX at most), WILD's 0..GEN_AGE_WILD from its own stream (gq kept: no other
 * stream moves), a hidden genre (the tests') 0 */
static uint32_t gen_age(void)
{
    const gid_t *id = &gp.id;
    uint32_t a, i, j, q;
    if (id->tape == GT_OFF) return 0;
    if (id->tape != GT_AUTO) return (id->tape - 1u) * 10u;
    if (id->main == GID_WILD) {
        q = gq; gseed(GS_TAPE); a = (uint32_t)grr(0, (int32_t)GEN_AGE_WILD); gq = q;
        return a;
    }
    i = gen_shown_index(id->main + 1u);
    j = id->part == GID_SOLO ? 255u : gen_shown_index(id->part + 1u);
    a = (i < GEN_NSHOWN ? GEN_AUTO_AGE[i] : 0u) + (j < GEN_NSHOWN ? GEN_AUTO_AGE[j] / 2u : 0u);
    return a > GEN_AGE_MAX ? GEN_AGE_MAX : a;
}
/* (0.9 spec 3) still over the budget after the thinning (gx_fit_piece): on AUTO, AGE down 10 at a time, to 0 at the last;
 * an AGE set with the knob is kept (the safety net's TAPE rung is the last resort). TAPE is a fixed cost while AGE is
 * above 0 (GC_TAPE1 / 2), so today the steps end at 0 once they start */
static void gen_fit_tape(void)
{
    gs.cost_thin = (uint16_t)gc_cost(1);
    gs.age_cut = 0;
    if (gp.id.tape != GT_AUTO) return;
    while (gs.age && gc_cost(1) > gc_budget()) {
        gs.age = (uint8_t)(gs.age > 10u ? gs.age - 10u : 0u);
        gs.age_cut = 1;
    }
    if (gs.age_cut) tape_age_apply((int32_t)gs.age);
}
```
  `gc_cost`: after the delay line's term, `if (gs.age) c += gcf.two ? GC_TAPE2 : GC_TAPE1;   /* (0.9) TAPE while the
  piece's AGE is above 0 */`. Rewrite the `GC_TAPE1` comment (`:2023-2030`): counted since 0.9 (spec §3), the figure and
  its origin unchanged; the budget comment (`:1993-2010`) ends "(0.9) every piece fitted: gx_fit_piece, then
  gen_fit_tape" instead of "RANDOM's and a genre's pieces left as they were …". `gen_fit_rest`: drop `gx.on && ` from the
  chord-notes `while` (`:2179`). `gen_make`: `gs.age = (uint8_t)gen_age();` right before `gen_pick();`; the end
  (`:2630-2637`) becomes:

```c
    for (k = 0; k < NTRK; k++) trk[k].p[P_MSND] = trk[k].p[P_MPAT] = 0;   /* (0.6.96, as before) */
    tape_age_apply((int32_t)gs.age);                    /* (0.9) TAPE: the knob's AGE, or AUTO the style's (gen_age) */
    if (gx.on) { gx_mutate(); gx_heads(); }             /* (0.8.6) EXPLORE: MUTATE, PLAYHEADS (before the sections) */
    gx_fit_piece();                                     /* (0.9) every piece: the full fitter, TAPE counted */
    gen_fit_tape();                                     /* (0.9) AUTO, still over: AGE lowered */
    mut_keep();
    gen_parts();
```
  `tests/stems_test.c` and `tests/hush_test.c`: set `gp.tape = GT_OFF;` with their other `gp.*` lines (they measure the
  tracks and the silence path, not the master: TAPE on would make "the sum = the mix" untrue by design); comment
  `(0.9: TAPE off: the stems are the tracks')`.
- [ ] **Step 4: Run to see it pass** — `gen_test`. `GEN_HOUSE_HASH` (`:117`): if its check fails now (HOUSE 0421 thinned),
  set the printed value with the comment `(0.9: re-recorded at <short hash of this commit's parent>: every piece fitted,
  spec 6)` — the only re-recorded generator value; name it in the commit. Any other failing music check: pre-flight 20
  (`gt_cpt = 63` around it, listed in the commit). Then `ui_test`, `stems_test`, `hush_test`, `audio_test` (as in
  `run_tests.sh`), `./build.sh`, the suite.
- [ ] **Step 5: Commit** — `git add firmware/src/gen.c tests/gen_test.c tests/stems_test.c tests/hush_test.c && git commit`
  with `feat(gen): every piece fitted with TAPE counted; AUTO AGE per style, lowered only after the thinning`; body: the
  0.8 rule withdrawn, `GEN_HOUSE_HASH` old → new (if it moved), the checks moved to `gt_cpt = 63`.

---

### Task 4: Recent pieces (the last 8 identities in RAM)

**Files:** Modify `firmware/src/gen.c` (`gp` gains `at`; the ring after `gen_build`; `gen_build` pushes),
`firmware/src/ui_layer.c:521-528` (`session_dice` clears the ring), `tests/gen_test.c`.

**Interfaces — Consumes:** `gid_t`, `gen_build`, `gen_draw`, `piece_sig`, `gid_eq_t` (Task 2). **Produces:**
```c
#define GEN_NRECENT 8u
static gid_t gen_recent[GEN_NRECENT];                   /* (0.9 spec 4) newest first; RAM: power-off clears it */
static uint8_t gen_nrecent;
/* gp gains */
    uint8_t at;                                         /* (0.9) PRESETS on GEN: 0 NEW, k the recent piece k - 1 */
static int gid_eq(const gid_t *a, const gid_t *b);      /* field by field */
static void gen_recent_push(const gid_t *id);           /* to the front; already there: moved; a ninth: the oldest out */
static void gen_recent_clear(void);                     /* none; PRESETS on NEW */
static void gen_ui_defaults(void);                      /* gp.amt GA_AUTO, swing GA_AUTO, human 30 (pre-flight 7) */
static void gen_generate(void);                         /* OCT+ at NEW: gen_draw, defaults, build; gp.at 0 */
static void gen_recall(uint32_t k);                     /* recent piece k (0 newest) rebuilt and moved to the front */
```

- [ ] **Step 1: Write the failing test** (`tests/gen_test.c`, after Task 2's block):

```c
    {   /* (0.9 spec 4) recent pieces: 9 presses keep 8, newest first; each brought back exactly (the oldest each time),
         * moved to the front, never twice; UNDO marked; GENERATE never repeats a recalled piece's main; a restart clears */
        gid_t ids[9];
        uint32_t sigs[9], pr, j, ok = 1, okb = 1, rep = 0;
        gen_reset(); session_dice(4242u);
        for (pr = 0; pr < 9u; pr++) {
            host_ticks += 1300000u + pr * 777u;
            gen_generate();
            ids[pr] = gp.id; sigs[pr] = piece_sig();
        }
        ok &= gen_nrecent == 8u && gp.at == 0u;
        for (j = 0; j < 8u; j++) ok &= gid_eq(&gen_recent[j], &ids[8u - j]);
        bad += gcheck("(0.9) recent pieces: 9 presses keep the last 8, newest first", ok);
        for (j = 0; j < 8u; j++) {
            undo_ok = 0;
            gen_recall(gen_nrecent - 1u);
            okb &= piece_sig() == sigs[1u + j] && gid_eq(&gen_recent[0], &ids[1u + j]) && gen_nrecent == 8u && undo_ok;
        }
        for (j = 1; j < 8u; j++) okb &= !gid_eq(&gen_recent[j], &gen_recent[0]);
        bad += gcheck("  each brought back (the oldest each time): the same piece, to the front, never twice, UNDO marked", okb);
        for (pr = 0; pr < 50u; pr++) {
            gid_t id;
            gen_recall(3);
            host_ticks += 990000u;
            gen_draw(&id);
            rep += id.main != GID_WILD && id.main == gp.id.main;
        }
        bad += gcheck("  GENERATE after a piece brought back: never its main", !rep);
        session_dice(77u);
        bad += gcheck("  a restart (the power-on's session_dice) clears them, PRESETS on NEW", gen_nrecent == 0u && gp.at == 0u);
    }
```
- [ ] **Step 2: Run to see it fail** — `gen_test`: `gen_generate`, `gen_recall`, `gen_recent`, `gid_eq` unknown.
- [ ] **Step 3: Implement** (`gen.c`, after `gen_draw`; `gen_build` ends with `gen_recent_push(&gp.id);`):

```c
/* (0.9 spec 4) the last pieces' identities, newest first: a few dozen bytes of RAM, nothing in the flash */
#define GEN_NRECENT 8u
static gid_t gen_recent[GEN_NRECENT];
static uint8_t gen_nrecent;
static int gid_eq(const gid_t *a, const gid_t *b)
{
    return a->seed == b->seed && a->two == b->two && a->dev == b->dev && a->form == b->form && a->tape == b->tape &&
           a->main == b->main && a->part == b->part && a->energy == b->energy && a->complex == b->complex;
}
static void gen_recent_push(const gid_t *id)
{
    uint32_t k, n = gen_nrecent;
    for (k = 0; k < n && !gid_eq(&gen_recent[k], id); k++) {}
    if (k == n) k = n < GEN_NRECENT ? n++ : GEN_NRECENT - 1u;
    for (; k; k--) gen_recent[k] = gen_recent[k - 1u];
    gen_recent[0] = *id;
    gen_nrecent = (uint8_t)n;
}
static void gen_recent_clear(void) { gen_nrecent = 0; gp.at = 0; }
static void gen_ui_defaults(void)                       /* (pre-flight 7) the parts' amounts as AUTO does */
{
    uint32_t k;
    for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
    gp.swing = GA_AUTO; gp.human = 30;
}
static void gen_generate(void)
{
    gid_t id;
    gen_draw(&id);
    gen_ui_defaults();
    gen_build(&id);
    gp.at = 0;
}
static void gen_recall(uint32_t k)
{
    gid_t id;
    if (k >= gen_nrecent) return;
    id = gen_recent[k];                                 /* (a copy: gen_build's push moves the ring under it) */
    gen_ui_defaults();
    gen_build(&id);
}
```
  `ui_layer.c session_dice`: after `gp.seed = …;` add `gen_recent_clear();   /* (0.9) a new session: no recent pieces */`.
  Remove `ids`' unused-variable warnings if any; `gid_eq_t` in the test may now be replaced by `gid_eq`.
- [ ] **Step 4: Run to see it pass** — `gen_test`, `ui_test` (`session_dice` users `:10525`, `:10538`, `:21350`), `./build.sh`
  (RAM + 64 B + `gp`'s growth), the suite.
- [ ] **Step 5: Commit** — `git add firmware/src/gen.c firmware/src/ui_layer.c tests/gen_test.c && git commit` with
  `feat(gen): the last 8 pieces kept in RAM and rebuilt from their identity`.

---

### Task 5: The one GEN page (four knobs, OCT+ GENERATE, PRESETS through the recent pieces)

**Files:** Modify `firmware/src/gen.c` (`gp`: drop `page`, `sig`; drop `gen_sig`, `gen_amt_turn`, `gen_pos`, `gen_page`,
`gen_page_put`, `GEN_LAST_W`, `gen_last_line`; rewrite `gen_press`, `dice_all`, `GEN_LBL`, `gen_open`, `gen_knob`,
`gen_value`, `gen_cards`, `gen_draw_sig`, `gen_panel` `:2687-2931`), `firmware/src/gen_tables.c` (drop `GEN_NPOS`,
`GEN_NSPOS`; the comment at `:588-592`), `firmware/src/ui_input.c` (`:2451`, `:2640-2643`, `:2650-2659`),
`firmware/src/ui_lens.c` (`:48-49`, `:141`, `:165`, `:229-230`, `:252-261`), `firmware/src/ux_nav.h:3-11`,
`firmware/src/ux_nav.c:4-6`, `firmware/src/ui_draw.c:992-997`, `web/emu/felucca_web.c:473`, `tests/dice_metrics.c:239`,
`tests/gen_test.c` (`:943-948`, `:972-1102`), `tests/ui_test.c` (`:7745-7772`, `:10795-10835`, `:11965-11985`),
`tests/ui_render.c` (`:587-657`, `:1345-1354`, `:1501-1502`), `tools/gen_manual.py:32-33`, `tools/gen_nav_map.py:85, 93,
111`, `web/manual/data.js` (shot names only: `gen_explore` → `gen_wild`, `gen_parts` → `gen_recent`, `gen_lead` →
`gen_tape`; `"HOME/GEN 1..3"` → `"HOME/GEN": ["ENERGY", "FORM", "COMPLEX", "TAPE"]`), generated: `docs/navigation-map.svg`
(+ PNG), `web/manual/nav.js`, `web/manual/shots/`.

**Interfaces — Consumes:** `gen_generate`, `gen_recall`, `gen_recent`, `gen_nrecent`, `gp.at` (Task 4), `gid_name`,
`gform_order(const gid_t *, …)`, `GT_*` (Task 2). **Produces:**
```c
static const char *const GEN_LBL[4] = {"ENERGY", "FORM", "COMPLEX", "TAPE"};
static void gen_open(void);                             /* gp.hot 0xFF, gen_fresh, gp.at 0 (pre-flight 8) */
static void gen_knob(uint32_t k, int32_t s);            /* ENERGY +-5, FORM +-1, COMPLEX +-5, TAPE +-1 position */
static void gen_recent_turn(int32_t s);                 /* PRESETS: back (s < 0) older, on toward NEW; clamped */
static void gen_press(void);                            /* OCT+: gp.at ? gen_recall(gp.at - 1), gp.at = 1 : gen_generate() */
static void gen_value(uint32_t k, char *b);             /* "50%", "LOOP", "AUTO" / "OFF" / "60%" */
static const gid_t *gen_shown_id(void);                 /* the piece the panel names: a recent one, else gp.id */
static const char *gen_oct_label(void);                 /* "OCT+ GENERATE" / "OCT+ RECALL" */
static void gen_panel_line(char *v, uint32_t n);        /* "LAST: <name>" / "RECENT k: <name>", fitted to its room */
/* ux_nav.h */
#define UXH_NGEN 1u                                     /* (0.9: GEN is one page) */
```

- [ ] **Step 1: Write the failing tests.** `tests/gen_test.c`: delete the NEW PROJECT block (`:972-978`) and the screen
  block's GENRE walk, hidden-genre-name and `SELECT: page 2` parts (`:979-1012`, `:1028-1029`, `:1086-1102`); keep the
  tempo, PLAY and safety checks. Power-on block (`:945-946`): drop `gp.genre = GEN_EXPLORE; gp.sig = 0;`. New screen block:

```c
    {   /* (0.9 spec 1) the GEN page: one page, ENERGY FORM COMPLEX TAPE; OCT+ GENERATE; PRESETS NEW then the recent
         * pieces (OCT+ brings one back); SELECT nothing; OCT- TRACKS; no GENRE, no SEED; NEW PROJECT keeps it */
        static const char *const LBL[4] = {"ENERGY", "FORM", "COMPLEX", "TAPE"};
        static const char *const TV[13] = {"AUTO", "OFF", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%", "100%"};
        char v[40], n0[32];
        uint32_t k, ok = 1, s1, s2;
        gen_reset(); session_dice(9u);
        go_home(); frames(16); fm1_ms += 1200u;
        held_turn(B_HOME, 1);
        ok &= ui.confirm == CF_DICE_ALL && gp.at == 0u && !strcmp(gen_oct_label(), "OCT+ GENERATE");
        for (k = 0; k < 4u; k++) ok &= !strcmp(GEN_LBL[k], LBL[k]) && strcmp(GEN_LBL[k], "GENRE") && strcmp(GEN_LBL[k], "SEED");
        bad += gcheck("(0.9) HOME + SELECT: GEN, one page: ENERGY FORM COMPLEX TAPE (no GENRE, no SEED), PRESETS on NEW, OCT+ GENERATE", ok);
        frames(800);
        gp.tape = GT_AUTO;
        for (ok = 1, k = 0; k < 13u; k++) { gen_value(3, v); ok &= !strcmp(v, TV[k]); turn(EN_K4, 1); }
        for (k = 0; k < 13u; k++) turn(EN_K4, -1);
        gen_value(3, v); ok &= !strcmp(v, "AUTO") && gp.tape == GT_AUTO;
        gp.energy = 50; turn(EN_K1, 2); ok &= gp.energy == 60u; gen_value(0, v); ok &= !strcmp(v, "60%");
        gp.form = 0; turn(EN_K2, 1); gen_value(1, v); ok &= !strcmp(v, "PARTS"); turn(EN_K2, 5); gen_value(1, v); ok &= !strcmp(v, "SONG");
        gp.complex = 50; turn(EN_K3, -1); ok &= gp.complex == 45u;
        bad += gcheck("  KNOB 4 TAPE: AUTO, OFF, 10 .. 100 %, stops; KNOB 1 ENERGY +-5, KNOB 2 FORM, KNOB 3 COMPLEX", ok);
        turn(EN_K2, -5); gp.tape = GT_AUTO;
        press(B_OCTUP); frames(4);
        str_cpy(n0, gp.last, sizeof n0); s1 = piece_sig();
        ok = ui.confirm == CF_DICE_ALL && !strncmp(ui.msg, "GEN: ", 5) && gen_nrecent == 1u;
        press(B_OCTUP); frames(4); s2 = piece_sig();
        ok &= gen_nrecent == 2u && s2 != s1 && strcmp(gp.last, n0);
        bad += gcheck("  OCT+: GENERATE (GEN: name seed), the screen stays; again: another piece", ok);
        frames(800);                                    /* (the knobs quiet a moment after a button: ui_layer.c) */
        turn(EN_PRESET, -1);
        gen_panel_line(v, sizeof v);
        ok = gp.at == 1u && !strncmp(v, "RECENT 1: ", 10) && !strcmp(gen_oct_label(), "OCT+ RECALL");
        turn(EN_PRESET, -1); turn(EN_PRESET, -1);
        ok &= gp.at == 2u;                              /* (clamped at the oldest) */
        press(B_OCTUP); frames(4);
        ok &= piece_sig() == s1 && gp.at == 1u && gen_nrecent == 2u && !strcmp(gp.last, n0);
        frames(800);
        turn(EN_PRESET, 5);
        ok &= gp.at == 0u && !strcmp(gen_oct_label(), "OCT+ GENERATE");
        bad += gcheck("  PRESETS back: RECENT 1, 2 (stops at the oldest), OCT+ RECALL brings it back (the same piece); on: NEW", ok);
        fm1_ms += 1000u;
        k = nav_cur();
        turn(EN_SELECT, 1); frame(); turn(EN_SELECT, -1); frame();
        bad += gcheck("  SELECT: nothing (GEN is one page)", gen_on() && nav_cur() == k && k == UXH_GEN0 && gp.at == 0u);
        press(B_OCTDN);
        bad += gcheck("  OCT-: back to TRACKS, the piece stays", !ui.confirm && ui.home && home_level() == 0u && piece_sig() == s1);
        tools_run(5); press(B_OCTUP);
        bad += gcheck("  NEW PROJECT: GEN's knobs and recent pieces kept", msg_is("NEW PROJECT") && gen_nrecent == 2u && gp.form == 0u);
    }
    {   /* every name fits: the status line (232 px) and the panel's line beside its label, for every style and blend */
        uint32_t i, j, wide = 0;
        gid_t id;
        char v[40];
        gen_reset();
        for (i = 0; i <= GEN_NSHOWN; i++)
            for (j = 0; j <= (i < GEN_NSHOWN ? GEN_NPARTNER[i] : 0u); j++) {
                gid_now(&id); id.seed = 8888;
                id.main = i < GEN_NSHOWN ? GEN_SHOWN[i] : GID_WILD;
                id.part = i < GEN_NSHOWN && j < GEN_NPARTNER[i] ? GEN_SHOWN[GEN_PARTNER[i][j]] : GID_SOLO;
                gen_build(&id);
                gen_panel_line(v, sizeof v);
                wide += text_w(&UF_HDR, ui.msg) > 232 || text_w(&AF_X, v) > 228 - text_w(&AF_X, gen_oct_label()) - 8 || !strstr(v, " 8888");
            }
        bad += gcheck("(0.9) every style, blend and WILD: the name and seed whole on the status line and the panel", !wide);
    }
```
  `tests/ui_test.c`: `:7755-7768` → after opening GEN, turn KNOB 3 six times and SELECT once: `host_big_fills` unchanged,
  `gen_on()` (no page field); `:10804` → `ux_home_at(UXH_GEN) && nav_cur() == UXH_GEN0` (GEN at its one page: drop the
  SELECT to GEN 2 and `gen_page() == 1u`); `:10814` → `cnt[2] == 1u` and `m == 3u`; `:10824` → `lz_count_is("GEN")`;
  `:10826-10828` → after SELECT still `lz_count_is("GEN")`; `:11974-11980` → one GEN step, `n == UXH_GEN + UXH_NGEN`
  (unchanged expression, `UXH_NGEN` now 1). The hint strings at `:11018`, `:11478`, `:11536`, `:12410` stay as they are
  (spec: they stay true); add next to `:11478`: after `press(B_HOME)` reaches GEN, `str_eq(gen_oct_label(), "OCT+
  GENERATE")` in the same check.
  `tests/ui_render.c`: scenes `S_GEN` (`"gen"`: NEW after RETROBOC+AMBIENT 0421), `S_GEN_RECENT` (`"gen_recent"`, was
  `S_GEN_PARTS`: three recent pieces, `gp.at = 2`), `S_GEN_TAPE` (`"gen_tape"`, was `S_GEN_LEAD`: `gp.hot = 3`, `gp.tape
  = 7` = 60 %), `S_GEN_WILD` (`"gen_wild"`, was `S_GEN_EXPLORE`), `S_GEN_LONG` (`"gen_long"`, replaces `S_GEN_BEAT`,
  `S_GEN_DNB`, `S_GEN_RETRO`: BREAKS+DRILL'N'BASS 8888). Each sets `ui.confirm = CF_DICE_ALL`, fills `gp.id` / `gp.last`
  with `gid_now` + fields + `gid_name` (no build), and `gen_recent_push` for the recent scene. RED: compile errors
  (`gen_oct_label`, `gen_panel_line`, new `gen_value` signature).
- [ ] **Step 2: Run to see it fail** — `gen_test` and `ui_test` builds fail as stated.
- [ ] **Step 3: Implement.** `gen.c`:

```c
static const char *const GEN_LBL[4] = {"ENERGY", "FORM", "COMPLEX", "TAPE"};   /* (0.9: one page) */
static const char *const GEN_FORM[3] = {"LOOP", "PARTS", "SONG"};
static uint8_t gen_fresh;
static void gen_open(void) { gp.hot = 0xFF; gen_fresh = 1; gp.at = 0; }   /* (0.9: the knobs as left; PRESETS on NEW) */
static void gen_knob(uint32_t k, int32_t s)
{
    gp.hot = (uint8_t)k;
    if (k == 0u) gp.energy = (uint8_t)clamp((int32_t)gp.energy + s * 5, 0, 100);
    else if (k == 1u) gp.form = (uint8_t)clamp((int32_t)gp.form + (s > 0 ? 1 : -1), 0, 2);
    else if (k == 2u) gp.complex = (uint8_t)clamp((int32_t)gp.complex + s * 5, 0, 100);
    else gp.tape = (uint8_t)clamp((int32_t)gp.tape + (s > 0 ? 1 : -1), 0, (int32_t)GT_N - 1);
    ui.force = 1;
}
static void gen_recent_turn(int32_t s)                  /* (0.9) PRESETS: a detent a piece, back older, on to NEW */
{
    gp.at = (uint8_t)clamp((int32_t)gp.at + (s < 0 ? 1 : -1), 0, (int32_t)gen_nrecent);
    ui.force = 1;
}
static void gen_press(void)                             /* OCT+ on GEN: GENERATE, or the recent piece shown */
{
    if (gp.at && gp.at <= gen_nrecent) { gen_recall(gp.at - 1u); gp.at = 1; }
    else gen_generate();
}
static void dice_all(void)                              /* (the tools, the films) a press without the screen */
{
    uint8_t g = gp.genre;
    gp.genre = 0;
    gen_new_seed();
    gen_go();
    gp.genre = g;
}
static void gen_value(uint32_t k, char *b)
{
    uint32_t v = k == 0u ? gp.energy : gp.complex;
    if (k == 1u) { str_cpy(b, GEN_FORM[gp.form % 3u], 16); return; }
    if (k == 3u) {
        if (gp.tape == GT_AUTO) { str_cpy(b, "AUTO", 16); return; }
        if (gp.tape == GT_OFF) { str_cpy(b, "OFF", 16); return; }
        v = (gp.tape - 1u) * 10u;
    }
    fmt_int(b, (int32_t)v);
    str_cpy(b + str_len(b), "%", 2);
}
static void gen_cards(void)
{
    uint32_t k;
    char v[16];
    for (k = 0; k < 4u; k++) {
        int32_t r = k == 0u ? gp.energy * 10 : k == 1u ? gp.form * 500 : k == 2u ? gp.complex * 10 :
                    (int32_t)gp.tape * 1000 / (int32_t)(GT_N - 1u);
        gen_value(k, v);
        draw_column(k, GEN_LBL[k], v, "", VAL(k), r, ICON_NONE);
    }
}
static uint32_t gen_draw_sig(void)
{
    uint32_t h = ((((uint32_t)gp.energy * 131u + gp.form) * 131u + gp.complex) * 131u + gp.tape) * 131u + gp.at;
    return (h * 131u + gen_nrecent) * 31u + str_hash(gp.hot * 977u, gp.last) + ux.gen;
}
static const gid_t *gen_shown_id(void) { return gp.at && gp.at <= gen_nrecent ? &gen_recent[gp.at - 1u] : &gp.id; }
static const char *gen_oct_label(void) { return gp.at ? "OCT+ RECALL" : "OCT+ GENERATE"; }
static void gen_panel_line(char *v, uint32_t n)
{
    int32_t w = 228 - text_w(&AF_X, gen_oct_label()) - 8;
    char nm[32];
    if (!gp.at && !gp.last[0]) { str_cpy(v, "LAST: -", n); return; }
    gid_name(gen_shown_id(), nm, sizeof nm, 0);
    if (gp.at) { str_cpy(v, "RECENT ", n); fmt_int(v + 7, gp.at); str_cpy(v + str_len(v), ": ", n - str_len(v)); }
    else str_cpy(v, "LAST: ", n);
    str_cpy(v + str_len(v), nm, n - str_len(v));
    if (text_w(&AF_X, v) > w) str_cpy(v, nm, n);                 /* (too wide: the name alone) */
    if (text_w(&AF_X, v) > w) gid_name(gen_shown_id(), v, n, 1); /* (still: the cards' short names) */
}
```
  `gen_panel`: the five rows drawn as now but never lit (`on = 0`), no amount text, the preview hashed on
  `gen_shown_id()->seed` / `->complex` / `->energy`; the chips from `gid_t f = *gen_shown_id(); if (!gp.at) f.form =
  gp.form;` (`f.form == 0`: one chip; else `gform_order(&f, f.form == 2u ? (f.main < NGENRE && GENRE[f.main].form ==
  FM_ARC ? FM_ARC : FM_CLUB) : FM_PARTS, ord, mul)`); the last line `gen_panel_line(v, sizeof v); cv_clip_text(6, 126,
  &AF_X, v, T_MID, T_SURF, 228 - text_w(&AF_X, gen_oct_label()) - 8); cv_text_r(234, 126, &AF_X, gen_oct_label(),
  T_ACCENT, T_SURF);`; the "GENRE:" line for a turned genre card goes. `gp`: drop `page`, `sig`; drop `gen_sig`,
  `gen_amt_turn`, `gen_pos`, `gen_page`, `gen_page_put`, `GEN_LAST_W`, `gen_last_line`. `gen_tables.c`: drop `GEN_NPOS`,
  `GEN_NSPOS`; the comment at `:588-592` says `gp.genre` is the tools' and tests' way in since 0.9.
  `ui_input.c`: `:2451` line removed (pre-flight 8); `:2642` → `gen_recent_turn(s);   /* (0.9) GEN: the recent pieces */`;
  the SELECT branch:
```c
    } else if ((s = panel_enc(EN_SELECT)) != 0 && !lay && !ui.menu) {
        if (gen_on())                                   /* (0.9) GEN is one page: SELECT does nothing there */
            ;
        else if (lock_held())
```
  `ui_lens.c`: `CE_HOME` `NV(0, "TRACKS", 0, 0), NV(0, "VISUAL", 1, 0), NV(0, "GEN", 2, 2)`; drop the `gen_page` /
  `gen_page_put` declarations; `nav_cur`: `return gen_on() ? UXH_GEN0 : ux_home_page();`; `nav_go`: drop
  `gen_page_put(e - UXH_GEN0);`. `ux_nav.h`: `#define UXH_NGEN 1u` and the comments ("GEN, one page since 0.9");
  `ux_nav.c:4-6` comment (2 GEN). `ui_draw.c:992-997`: `str_cpy(b, "GEN", 4); return text_w(&AF_X, b);`.
  `web/emu/felucca_web.c:473`: delete `gp.sig = gen_sig();`. `tests/dice_metrics.c:239`: drop `gp.sig = 0;`.
  `tools/gen_manual.py`: `'GEN': 'gen'` in place of the three; `tools/gen_nav_map.py:93` notes `'TRACKS → VISUAL → GEN:
  HOME again (round) or SELECT; HOME + SELECT straight to GEN. GEN: ENERGY, FORM, COMPLEX, TAPE; OCT+ GENERATE; PRESETS
  the recent pieces; OCT− back to TRACKS.'`, `:85` "the recent pieces in GEN", `:111` unchanged. `web/manual/data.js`:
  the shot names and the `"HOME/GEN"` row only (texts are Task 7).
- [ ] **Step 4: Run to see it pass** — `gen_test`, `ui_test`; `ui_render` (as `run_tests.sh:295-299`: the GEN scenes
  lint-clean); `python3 tools/gen_nav_map.py` (Chromium present; else `--svg` and say so) then `--check`;
  `python3 tools/gen_manual.py build/ui_new/ACID` then `--check`; `node web/test_manual.mjs`; compile checks
  `tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/dice_metrics tests/dice_metrics.c -lm` and
  `tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -o build/host/emu_native web/emu/native_check.c -lm`;
  `python3 tests/ux_msg_scan.py` (as the suite runs it); `./build.sh`; the suite.
- [ ] **Step 5: Commit** — `git add` the files above and the generated ones (`docs/navigation-map.*`, `web/manual/nav.js`,
  `web/manual/shots/`) `&& git commit` with `feat(ui): GEN is one page: ENERGY FORM COMPLEX TAPE, OCT+ GENERATE, PRESETS
  the recent pieces`.

---

### Task 6: Nothing of GEN is stored; the tools and the emulator build

**Files:** Test `tests/gen_test.c`; check-only: `tests/gen_dump.c`, `tools/dice_render.c`, `web/emu/native_check.c`,
`tests/target_budget.py`.

**Interfaces — Consumes:** `gid_now`, `gen_recent_push`, `gp.energy/form/complex/tape/at` (Tasks 2, 4, 5);
`project_capture(project_t *)` (`project.c:1710`), `proj_pack(project_store_t *, const project_t *)` (`:952`),
`settings_export(persist_t *)` (`settings_persist.c:139`).

- [ ] **Step 1: Write the test** (`tests/gen_test.c`, before the final printf):

```c
    {   /* (0.9 spec 6, pre-flight 1) GEN's page and its recent pieces are the session's: a project and the settings are
         * byte for byte the same whatever they hold (nothing to ignore on load: nothing of GEN was ever stored) */
        static project_t pa, pb;
        static project_store_t sa, sb;
        static persist_t ea, eb;
        gid_t x;
        gen_reset();
        gen_with(1u + GEN_SHOWN[2], 55, 50, 0);
        memset(&sa, 0, sizeof sa); memset(&sb, 0, sizeof sb); memset(&ea, 0, sizeof ea); memset(&eb, 0, sizeof eb);
        project_capture(&pa); proj_pack(&sa, &pa); settings_export(&ea);
        gp.energy = 85; gp.form = 2; gp.complex = 10; gp.tape = 7; gp.at = 0;
        gid_now(&x); x.seed = 999; gen_recent_push(&x);
        project_capture(&pb); proj_pack(&sb, &pb); settings_export(&eb);
        bad += gcheck("(0.9) GEN's page and recent pieces: a project and the settings byte for byte the same",
                      !memcmp(&sa, &sb, sizeof sa) && !memcmp(&ea, &eb, sizeof ea));
    }
```
- [ ] **Step 2: Run** — `gen_test`: this check passes at once (the property holds by design: it pins it). If it fails,
  something of GEN reached storage: stop and report.
- [ ] **Step 3: The tools and the emulator** — build each as its header says: `tests/gen_dump.c`, `tools/dice_render.c`
  (`build/dice_render`), `web/emu/native_check.c` (Task 5's line), `tests/dice_metrics.c`; run `build/host/gen_dump 0 421
  50 0` and `build/host/dice_metrics 8` (both print a piece and exit 0). `tools/video/capture_*.c`: `grep -n "gp\.\(page\|sig\)"
  tools/video/` must print nothing (they set only kept fields). emcc absent: the wasm build and `web/emu/emu_test.mjs`
  are skipped (report). `./build.sh`, `python3 tests/target_budget.py` (every line unchanged: report the three ISR
  lines), the image line (≥ 16 KiB free), the full suite.
- [ ] **Step 4: Commit** — `git add tests/gen_test.c && git commit` with `test(gen): GEN's page and recent pieces never reach
  a project or the settings`; body: the tool runs, the budget lines, the image and RAM lines, the skipped checks.

---

### Task 7: Docs for 0.9

**Files:** `README.md` (Status 0.9: GENERATE, blends, recent pieces, TAPE in GEN, the AGE retune), `web/manual/data.js`
(a `{v: "0.9", items: [...]}` entry first in `news` `:615`; section `gen` `:14` titled `GEN / GENERATE`; the entries
`:269-277` and the task `:565-569` rewritten: HOME + SELECT opens GEN; KNOBS ENERGY, FORM (LOOP PARTS SONG), COMPLEX, TAPE
(AUTO: the style decides; OFF; 10–100 %); OCT+ GENERATE picks a style itself (sometimes WILD, often a blend of two:
RETROBOC+AMBIENT 0421); PRESETS steps back through the last 8 pieces, OCT+ brings one back; OCT− back to TRACKS; GLO + G4
undoes), `docs/MANUAL.md` (the same GEN section; the TAPE section notes AGE 50 now a clearly worn tape and that 0.8
projects with AGE above 0 sound more worn), `web/landing/index.html` (version line, one GENERATE sentence),
`tools/gen_cheatsheets.py:164` (`'TRACKS · VISUAL · GEN'`, GEN's row: `ENERGY · FORM · COMPLEX · TAPE`, OCT+ GENERATE,
PRESETS RECENT) → regenerate `docs/cheatsheet/controls.html` and `docs/cheatsheet/sound.html`. The cheat-sheet PNG / PDF
stay pending (no Playwright: say so).

- [ ] **Step 1** — `grep -n -i "GENRE\|EXPLORE\|RANDOM\|GEN 1\|GEN 2\|GEN 3\|SEED" README.md docs/MANUAL.md web/manual/data.js
  web/landing/index.html docs/cheatsheet/*.html tools/gen_cheatsheets.py`: every hit describing the old GEN is rewritten
  (plain upper-case control names, English only, no band names; RETROBOC allowed; JIANT only in the credits line).
- [ ] **Step 2** — `python3 tools/gen_cheatsheets.py` (its documented invocation), `python3 tools/gen_manual.py --check`,
  `node web/test_manual.mjs`, `python3 tests/built_on_test.py`, `python3 tests/ux_msg_scan.py`; the suite.
- [ ] **Step 3: Commit** — `git add README.md web/manual/data.js docs/MANUAL.md web/landing/index.html tools/gen_cheatsheets.py
  docs/cheatsheet/*.html && git commit` with `docs: Braindancer 0.9 (GENERATE, blends, recent pieces, TAPE in GEN, AGE)`.

---

### Task 8: Release 0.9 and the device

**Files:** `firmware/src/felucca.c:42` (`FELUCCA_VERSION "v0.9"`), `web/manual/nav.js` (regenerated: the version),
`docs/braindancer/device-log.md` (a section `## 2026-10-xx — Braindancer 0.9 (GENERATE, blends, TAPE in GEN, AGE)` with
every device line **pending**), new `docs/superpowers/plans/2026-10-31-braindancer-piece6-deferred.txt` (what the task
reports flagged for later, including: a quicker way to reach TAPE / AGE outside GEN; per-role mixing of styles (approach
B); more styles; widening TAPE's line for a deeper WOW; `GC_TAPE` from the device; AGE-dependent TAPE cost in the model).

- [ ] **Step 1 (implementer)** — version, `python3 tools/gen_manual.py build/ui_new/ACID` (nav.js version), log skeleton,
  deferred list; `./build.sh` (image ≥ 16 KiB free, RAM, pool spare: numbers in the report); `python3 tests/target_budget.py`;
  full suite green; commit `release prep: Braindancer 0.9 (one GENERATE button, blends, TAPE in GEN, the AGE retune)`;
  tree clean.
- [ ] **Step 2 (controller + user)** — fresh backup: in `~/MvaveFM1Unbricker` run `softkey --port "BRAINDANCER" --yes`,
  then the user runs `sudo .venv/bin/python fm1_unbrick.py backup DIR`; `./build.sh --release 0.9` on the clean HEAD;
  install `tools/fm1_install.py build/braindancer-0.9.fwsc --port "BRAINDANCER" --yes`; serial `status` (version
  `braindancer v0.9`, idle cpu, `audio_lost`, `safety_tape` 0). Then: about 20 GENERATE presses (the user presses and
  listens; console counters read after each few: no lost audio, `safety_tape` 0, the names seen: WILD and blends among
  them), the heavy-GEN case of the 0.8 session (a dense blend or WILD at ENERGY 90+) with `safety_tape` 0; PRESETS back
  through the recent pieces and two brought back (the same piece heard); a RETROBOC piece on AUTO sounds worn; AGE 50
  and 100 on SINE KEY A3 judged by ear (set over USB SysEx: `ED_SET 1 96 50`, then 100) and, if the measuring setup of
  2026-10-31 is at hand, its pitch sd and noise at AGE 0 / 50 / 100 against spec §5; fill the device log; merge ff to
  main and push only on the user's go-ahead.

---

## Self-review (done while writing)

- Spec §1 the page: Task 5 (four knobs, OCT+, PRESETS, OCT−, SELECT inert, names, GENRE / SEED gone, hints still true).
  §2 picking and blending: Task 2 (pool, WILD 10 %, no repeat, 45 % blends from the table, identity, determinism);
  pre-flight 3, 4, 5. §3 TAPE and budget: Task 3 (AUTO AGE, blend and WILD rules, knob OFF / amount, full fitter, TAPE
  counted, AUTO lowered after thinning); the device aim: Task 8. §4 recent pieces: Task 4 (8, newest first, move not
  duplicate, RAM only, rebuild with UNDO) and Task 5 (PRESETS / OCT+). §5 AGE retune: Task 1 (map, host measurement
  with derived targets, AGE 0 bit for bit); the ear: Task 8. §6 limits: Global Constraints, pre-flight 1, 18, Task 6
  (storage), Task 3 (re-recorded hash). §7 testing: each task's Step 1; device Task 8. §8 docs / release: Tasks 7, 8.
- Placeholders: none ("xx" in the device-log heading is the session's date, unknown until the device session).
- Names used across tasks: `gid_t`, `GID_WILD`, `GID_SOLO`, `GT_AUTO`, `GT_OFF`, `GT_N`, `gcf`, `gid_form_key`,
  `gform_order(const gid_t *, …)`, `gen_blend`, `gid_r`, `gid_pick`, `gid_now`, `gid_legacy`, `gid_name`, `gen_build`,
  `gen_go`, `gen_draw`, `GEN_PARTNER`, `GEN_NPARTNER`, `GEN_WILD_PCT`, `GEN_BLEND_PCT`, `GEN_AUTO_AGE`, `GEN_AGE_MAX`,
  `GEN_AGE_WILD`, `GS_TAPE`, `gs.pi`, `gs.age`, `gs.age_cut`, `gs.cost_thin`, `gen_age`, `gen_fit_tape`, `GEN_NRECENT`,
  `gen_recent`, `gen_nrecent`, `gp.at`, `gp.tape`, `gp.id`, `gid_eq`, `gen_recent_push`, `gen_recent_clear`,
  `gen_ui_defaults`, `gen_generate`, `gen_recall`, `gen_recent_turn`, `gen_press`, `gen_value(k, b)`, `gen_shown_id`,
  `gen_oct_label`, `gen_panel_line`, `UXH_NGEN`; tests `parts_sig`, `piece_sig`, `gt_style`, `gid_set`, `PT`, `pt_ok`,
  `gid_eq_t`, `age_measure`, `age_fft`, `test_age_09`, `AGE_PT_08`, `AGE_PT_09`.
