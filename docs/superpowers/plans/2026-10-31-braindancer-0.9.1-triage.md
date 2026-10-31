# Backlog triage for 0.9.1

Repo `~/synth/mvavefirmware`, branch main, HEAD 50159e3 (0.9 released; device-proven at d0b79b1). Read-only triage on
2026-10-31. Sources: the eight `docs/superpowers/plans/*deferred*.txt` lists, `docs/braindancer/device-log.md`,
`README.md`, the specs, and the code and tests at HEAD. Nothing was run on the device or changed in the repo.

**Statuses.** DONE means fixed or settled by a later piece, with evidence. OBSOLETE means the item is about something
removed or redesigned since. OPEN means still true. For OPEN items the size is S (under 1 h), M or L. "UV" means
user-visible. The last column says whether the item needs the device or a tool that isn't installed here. `info` marks
a recorded ruling or observation with nothing to do, kept OPEN so that it stays visible.

**Tools on this host.** gcc, make, python3 and `/usr/bin/chromium` are present. node, Playwright, emcc, valgrind and
segno are absent. The `web/*.mjs` tests can't run here.

## Counts

| List | Items | DONE | OBSOLETE | OPEN |
| --- | --- | --- | --- | --- |
| piece1 | 10 | 5 | 0 | 5 |
| piece2 | 9 | 2 | 2 | 5 |
| piece3a | 27 | 17 | 2 | 8 |
| piece3b | 22 | 12 | 0 | 10 |
| piece3c1 | 23 | 8 | 0 | 15 |
| piece3c2 | 28 | 5 | 2 | 21 |
| piece5 | 55 | 10 | 1 | 44 |
| piece6 | 43 | 3 | 0 | 40 |
| **Total** | **217** | **62** | **7** | **148** |

Of the 148 OPEN items, about 13 are `info`/rulings and about 12 duplicate an item in another list (marked "dup").
That leaves roughly 120 distinct pieces of work. Of those, about 30 need the device or an absent tool.

---

## piece1 (2026-10-24, pipeline)

| # | Item | Status | Evidence / note | Size | UV | Needs |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | nav-map PNGs stale; gen_nav_map.py title "JIANT FM" | DONE (title) | gen_nav_map.py:386-387 draws BRAINDANCER; navigation-map.svg regenerated 409351b. The PNG is still stale (last aec409f, 0.7): see piece6 #37 | | | |
| 2 | ui_signature.h JIANT signature on splash | DONE | file gone; ui_draw.c:1207 "JIANT's signature gone" (3a) | | | |
| 3 | ui_draw.c comment "by Juan Jiant" | DONE | no match; only copyright headers remain (required) | | | |
| 4 | web/index_pkg.html "Since 0.6.71 ACID FM uses both cores" | OPEN | Now says "Since 0.6.71 Braindancer uses both of the FM-1's cores (experimental)… has not been tried on a real unit yet" in EN/JA/ES (index_pkg.html:107,155,333). This is wrong: two-core has been device-proven since 0.1. It is the public installer page (make_site.py:10) | S | yes | test_web.mjs needs node (CI) |
| 5 | emulator.yml comments on JIANT 0.8.1 landing; tools/landing_anim dead | OPEN (part) | landing_anim deleted (DONE). emulator.yml:134 still reads "(JIANT 0.8.1) the site's front page (… what JIANT is …)" | S | no | |
| 6 | web/manual describes JIANT ("Todo JIANT en una página") | DONE | manual rewritten (a017c4f, 15c8ba2); only the credits footer names JIANT (guarded by built_on_test) | | | |
| 7 | felucca.c:42 / editor.c:412 comments >120 cols | OPEN | editor.c:412 is 136 cols; editor.c has 8 lines >120 and felucca.c:99-100 has 2 | S | no | |
| 8 | baseline.md has no SPDX header | OPEN | docs/braindancer/baseline.md (moved from docs/acid-fm). No docs/braindancer/*.md has one | S | no | |
| 9 | gen_about_qr.py / about_qr_test.py bare ImportError without segno | OPEN | still `import segno` (gen_about_qr.py:11). segno is absent here, so the test fails with a traceback | S | no | |
| 10 | fm1diag.js stockCustom hint could list "acid" | DONE | fm1diag.js:208 matches `braindancer\|acid[-_ ]?fm` | | | |

Release-state note (not an item): `web/STABLE` is still empty, so the public installer builds whatever is on `main`.
Pin it to the 0.9 proven commit d0b79b1, or to 0.9.1 once that is proven.

## piece2 (2026-10-24, trim)

| # | Item | Status | Evidence / note | Size | UV | Needs |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | [piece 4] sample writer +0xF00 guard etc. | OBSOLETE | BREAK, the sampler, dropped by the user 2026-10-28 (piece3c2 list); no sample writer exists | | | |
| 2 | [piece 4] NSONG / NSONG_INDEX defined twice, untied | OPEN | song_chain.c:25-28 and storage.c:16-19, both `#ifndef`, no static assert ties them | S | no | |
| 3 | [piece 4] DRUM-X and BREAK voices; CPU TEST after BREAK | OBSOLETE | BREAK dropped | | | |
| 4 | [piece 5] CPU TEST after TAPE | DONE | device-log.md:164-166 (0.8: TAPE off and AGE 100) | | | |
| 5 | [piece 3] ANIMAL / WHEEL in tools, MANUAL, cheat sheets | DONE (docs) | MANUAL.md and cheatsheet/sound.html ("SEVEN ENGINES") clean; WHEEL now only the mod-wheel source. Leftovers in the legacy tools/video and gen_readme_hero.py (piece6 #35); PNG/PDF not re-rendered (piece6 #36) | | | |
| 6 | regress: 45 presets with no golden hash | OPEN | piece5 T6 confirms 45 renders still missing from tests/golden.txt | S-M | no | |
| 7 | P_WRDEP / P_WDUST dead slots | OPEN (info) | kept for the stored layout (params.c:275, 312). WHEEL's ROTOR page row (params.c:906) is dead too | S | no | |
| 8 | tools/build.py isdigit() accepts non-ASCII digits | OPEN | build.py:300 unchanged | S | no | |
| 9 | flash-map.md unwrapped long lines | OPEN | now 20 lines >120 cols | S | no | |

## piece3a (2026-10-24, UI foundations)

| # | Item | Status | Evidence / note | Size | UV | Needs |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | [3b] low-contrast creature reds | DONE | marked DONE in the list (creatures gone) | | | |
| 2 | [3b] ux_knob_touch wiring | DONE | marked DONE in the list | | | |
| 3 | [3b] Huffman slot thrash | DONE | marked DONE in the list | | | |
| 4 | [3b] .pool / DC-cell margins | DONE | marked DONE; 0.9 image 563,140 B, 21,196 B free (device-log.md:184) | | | |
| 5 | [3c] menu help contrast; SCREEN rows inert | DONE | MENU on the section engine (465dabf); COLOR/SKIN/STYLE/SCOPE hidden (menu_items.c:26-30 menu_hidden) | | | |
| 6 | [3c] glow ring ~7x4.5 px rim banding | OPEN (low confidence) | no code change found; only matters "if other sections use big glows". Needs a visual check of a render | S | yes (faint) | |
| 7 | [3c] TRACKS bars linear vs dB number | DONE | ux_home.c:99-116 (3c-1 task 7: bar on the mixer's dB) | | | |
| 8 | [3c] NOTICE/DICE/NAME dialogs messy | DONE | ux_dialog.c (3c-2, 0.6) | | | |
| 9 | [3c] knob-row value width test | DONE | marked DONE in the list | | | |
| 10 | [3d] MANUAL.md / MANUAL.es.md full rewrite | DONE | MANUAL.es.md removed (cdae0ae, English only); MANUAL.md updated every release; no JIANT page words (grep creature/X-MOD/MSEQ/BEING/DLY 2: 0) | | | |
| 11 | [3d] tools/video signature stubs | OPEN | tools/video/thumbnails.py:80-81 `signature(): pass`. Legacy film tooling | S-M | no | |
| 12 | [3d] gen_readme_hero.py expects AIR_JIANT; web/manual/shots JIANT | OPEN (part) | shots regenerated (409351b, DONE); gen_readme_hero.py:7-30 still AIR_JIANT (dup piece6 #35) | M | no | |
| 13 | [3d] ui_test LARGE footer check weakened to "has ink" | OPEN | ui_test.c:9856 still an ink count | S | no | |
| 14 | [3d] stale ui_lzhome.c mention in ui_engvis.c | DONE | ui_engvis.c is gone; one historical mention at ux_home.c:47 is harmless | | | |
| 15 | [piece 6] GEN is still JIANT's screen | DONE | 0.9 one-page GEN: gen.c gen_cards / gen_panel (e20ade7). Its hint is still a header message (#16) | | | |
| 16 | [piece 6] GEN's hint only shown after a HOME tap | OPEN | ux_nav.c:29-35: only ux_home_flip (called from ui_lens.c:295) does `ui_message(ux_home_hint())`. Entering by HOME+SELECT shows no hint; ui_test.c:10819 tests only the tap | S | yes | |
| 17 | [device] Task 9 measurement (TRACKS/VISUAL/static, LOAD FULL/LITE) | DONE (superseded) | device-log.md:51-53 (0.3 MENU/TRACKS/VISUAL) and later CPU TEST runs at 0.4-0.6. SPI per frame was never measured, but nothing called for it | | | |
| 18 | [any] gfx.c "never drawn together" comment overstates | OPEN | gfx.c:540 unchanged | S | no | |
| 19 | [any] build/gen/ui_fonts.h staleness; gen_aa_font.py PFX global; adv_digit fallback | OPEN | gen_aa_font.py:69, 288, 303 | S | no | |
| 20 | [any] VISUAL legend clock starts before shown | DONE | ux_home.c:288-306, 507-513: the clock starts on the first call, after the message and knob checks, so a held knob no longer eats it | | | |
| 21 | [any] VISUAL silent-ring tolerance; "≤1 write per pixel" untested | OPEN (part) | 883defd added a real idle-pixel check; the write-per-pixel bound is still untested | S | no | |
| 22 | [any] SCOPE ON not checked in ui_test | OBSOLETE | SCOPE retired: hidden row, "settings byte … unread" (menu_items.c:29-30) | | | |
| 23 | power-on EMPTY patterns | DONE | marked DONE (power-on demo) | | | |
| 24 | HOME TRACKS late blocks | DONE | marked DONE | | | |
| 25 | interim: hide JIANT's non-IDM GEN genres | OBSOLETE | the genre list is gone; 0.9 has one GENERATE button (c02db49, e20ade7) | | | |
| 26 | MIDI page (per-track IN/OUT/CH, clock) | DONE | 3c-1 (0.5); editor 7a1ef30 | | | |
| 27 | [3c] automation mark on the knob row | DONE | 4094400 "Knob row marks a setting with automation or a lock"; ux_section.c:958-996 | | | |

## piece3b (2026-10-25, sound sections)

| # | Item | Status | Evidence / note | Size | UV | Needs |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | [3c] Roll: KNOB 1 is chord LEN with a step held | DONE | 3c-1 task 4: KNOB 4 is LEN (ui_input.c:2683-2687) | | | |
| 2 | [3c] Roll during ARP chord entry: MORE row can't be locked | DONE | 3c-1 task 4 fix 1: KNOB 4 locks the MORE row (ui_input.c:2689-2692) | | | |
| 3 | [3c] SEQ › STEP lock cards still JIANT's | DONE | ux_seq.c:218, 442 lock card "LOCK · NAME · VALUE" | | | |
| 4 | [3c] held MACRO "SELECT DICE" wording | DONE | 4094400; ux_layer.c:47 "SELECT DICES LFO" | | | |
| 5 | [3c] GLO+SELECT and TAP write BPM under an external clock | DONE | ui_layer.c:301 TAP "INT clock only" (3c-1 task 8); d93e6aa | | | |
| 6 | [3c] SCL keeps ROTATE / SEQ PITCH duplicates of SEQ | OPEN | ux_section.c:61 SEQ PITCH on SCL; UXS_CHORD_SAY "ROTATE" (ux_section.c:2226). Needs a design decision | S | yes | |
| 7 | [3c] automation/lock redraws pictures unpaced | DONE | 4094400 "through the pace rule (UX_VIS_MS)" | | | |
| 8 | [3c] engine change on EDIT page 3 lands on SOUND | DONE | 4094400 "keeps the place in the new engine's chain" | | | |
| 9 | [3d] MANUAL chapters 4/9/10 JIANT | DONE | see 3a #10 | | | |
| 10 | [3d] cheat sheets: prose JIANT's; PNG/PDF not re-rendered | DONE (HTML) | docs/cheatsheet/*.html at v0.9 (409351b). PNG/PDF stale: piece6 #36 | | | |
| 11 | [3d] gen_nav_map title JIANT; SYNTHS lists WHEEL/ANIMAL | DONE | gen_nav_map.py:386; no WHEEL/ANIMAL left | | | |
| 12 | [3d] tools/video JIANT renames, captions, beings.js | OPEN | tools/video/* still lists ANIMAL/WHEEL (capture_update.c, tutorial_script*.py, beings.js). Legacy | M | no | |
| 13 | [3d] VOICE's EDIT picture is its own glottal model | OPEN (info) | a note for the manual; no action needed | S | no | |
| 14 | [backlog] build pin guard VAR_DEF only [\w.] names | OPEN | tools/build.py:259 unchanged | S | no | |
| 15 | [backlog] NOTE RAND / CHO DEPTH fall to UF_LBL; UXC_WIDE list | OPEN | ux_cover_test.c:310-316, list grown | S | yes (small face) | |
| 16 | [backlog] DRIFT naming, FM6/DRUM MORE titles, ed_resp, GR_LYRIC/MORPH holders | OPEN (low confidence) | params.c:261/274 DRIFT used twice; the rest unverified | S-M | yes | |
| 17 | [backlog] MORE sel/top per section; first-time hint skipped <600 ms | OPEN | ux_more.c:263 uxm_fix still clamps | S | yes (minor) | |
| 18 | [backlog] FM6 shows DRIVE, DRUM-X hides it; drum-name styles; FIXED FREQ cycling | OPEN | no change found | S-M | yes | |
| 19 | [backlog] ENV's source on FM6 does nothing (could be dimmed) | OPEN (unverified) | no fix commit found; couldn't confirm which control is meant | S | yes | |
| 20 | [backlog] spacing in ARP "C G E HELD" | OPEN | ux_section.c:2101 | S | yes | |
| 21 | [device] DUST/DRUM picture CPU, fast NOISE DUST turn | DONE | device-log.md:65 (0.4: fast NOISE DUST turns while playing, no clicks) | | | |
| 22 | [device] CPU TEST late/s on MENU/TRACKS/EDIT/LFO; demo heard | DONE | device-log.md:63-68 (no clicks; demo heard). Per-page late/s not read; superseded by the 0.5/0.6 runs | | | |

## piece3c1 (2026-10-26, SEQ, GLO, MIDI page)

| # | Item | Status | Evidence / note | Size | UV | Needs |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | [3c-2] non-panel BPM writers ignore the external clock | DONE | d93e6aa "GEN, the demo, the editor and CPU TEST follow the clock rule"; 31bca5c | | | |
| 2 | [3c-2] SAVE, MENU, dialogs inherited layouts | DONE | 0.6 (1c9e56e) | | | |
| 3 | lt_mch per track, not per note (latched chords across a channel change) | OPEN | seq.c:46, 1062, 1103 unchanged. Can leave a note hanging on outboard gear | M | yes (rare) | |
| 4 | midi_forget_track checks current claims, not note-on claims | OPEN | midi_control.c:181-200 unchanged | M | yes (rare) | |
| 5 | bend kept under KEYBOARD PLAYS SELECTED not zeroed when the selection moves | OPEN | midi_control.c:437-439 keeps the selected track's bend; nothing zeroes it on a selection change | S | yes | |
| 6 | two TRIGs on one step: CONDITION shows/edits only the first; ALWAYS clears one | DONE | ux_seq.c:293-314 (task 4: TRIG edited in place in both slots; ALWAYS clears the second too) | | | |
| 7 | STEP EXTRAS PATTERN tests check direction only | OPEN | ui_test.c:18654 test_t33_pattern (`> 0`, `<`) | S | no | |
| 8 | pace test checks the total, not "≤1 per 1/16" | OPEN | ui_test.c:18445 `n1 >= 8u && n1 <= 15u` | S | no | |
| 9 | ux_msg_scan.py opts out by the _LBL suffix; use an explicit list before a second _LBL table | OPEN (now due) | a second table, UXG_MIDI_LBL, now exists; ux_msg_scan.py:87 still uses the suffix | S | no | |
| 10 | gen_aa_keycaps.py still makes the unused knob-arc tables | OPEN | gen_aa_keycaps.py:5, 22-25; knob_arc is no longer in ui_graph.c | S | no | |
| 11 | N_MONOTES "K+SEQ" third short form; stacked ux_more.h comments | DONE (main) | params.c:680 one short form "KEYS+SEQ". The comment nit is not worth tracking | | | |
| 12 | layers: foot/head hints not drawn; REC CLEARS ALL missing on DRUM/playing; REC K2-K4 touch empty column; uxl_word static buffer | OPEN (part) | REC CLEARS ALL now on lanes and playing (ux_layer.c:49-51, DONE). Still open: ui_layer.c:124 `.head`, uxl_word static (ux_layer.c:216), REC knob touch (ui_layer.c:667) | S | no | |
| 13 | layer tests thin (GLO E4/G4/A4/B4, DRUM lane width, REC pixel); dead GLO branches | OPEN | no new tests found | M | no | |
| 14 | stale comments ui_render.c:997, ui_test.c:11693 (hint_tick); EDITOR_PROTOCOL GLO > MIXER | DONE | 4094400 "Stale hint_tick comments"; EDITOR_PROTOCOL.md:42, 108 now say "until ACID FM 0.5" | | | |
| 15 | GLO line "NO RESTART" true only while playing | DONE | 8d2e68d; ux_layer.c:56-57 (playing); test_u9_glo_restart ui_test.c:22088 | | | |
| 16 | mark rule: knobs outside the generic path get no mark | OPEN (part) | 8d2e68d added GLO MIX LEVEL/PAN and MORE rows (ux_section.c:975-996). DJ FILTER and SEQ step pages not verified | S-M | yes | |
| 17 | MANUAL older page words (DLY 2 …) | DONE | grep DLY 2/DLY 3/REVERB 2/SL PITCH in MANUAL.md: 0 | | | |
| 18 | gen_readme_hero.py builds JIANT README images | OPEN (dup) | kept on purpose (JIANT-README.md, built_on_test.py); see piece6 #35 | M | no | |
| 19 | [device] CPU TEST on SEQ STEPS / GLO LIVE / MIDI page; DAW on USB; no-manual checks | OPEN (part) | SEQ STEPS measured (device-log.md:84-87); TRS clock followed; T5 every other bar via PLAYS (0.6). Pending: USB clock and notes from a DAW, ratchet, T2 to ch 10, live loop, GLO LIVE and MIDI page CPU (device-log.md:91-93) | M | yes | device |
| 20 | no Playwright: renders and test_manual Chromium part | OPEN (blocked) | dup piece6 #36/#37/#39 | | | Playwright |
| 21 | no emcc: emulator build and tests | OPEN (blocked) | dup piece6 #38 | | | emcc |
| 22 | 0.4 project within ~9 B of full gets SECTION FULL after a MIDI routing change | OPEN | project.c proj_packed_size; same class as piece5 #12 | S-M | yes (rare) | |
| 23 | web/editor.html per-track MIDI settings | DONE | 7a1ef30 | | | |

## piece3c2 (2026-10-27, SAVE, MENU, dialogs, PLAYS)

| # | Item | Status | Evidence / note | Size | UV | Needs |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | BREAK dropped; README roadmap and landing still list it | OPEN | README.md:5-6 "chopped breakbeats you load yourself", README.md:129 roadmap item 4; web/landing/index.html:48 "chopped breaks", :61 item 4 (README.md:121 and manual data.js:674 are past 0.2 entries) | S | yes | |
| 2 | [rename] ACID FM → BRAINDANCER | DONE | 0.7 (178269b, device-log.md:133-143) | | | |
| 3 | [piece 6] GEN one generate button | DONE | 0.9 (c02db49, e20ade7) | | | |
| 4 | [piece 6] GEN texture/arp levers disarmed after a section change | OPEN | safe_part is set only in gen_build (gen.c:2928-2929). Every section_load / SONG END restore goes through project_restore_runtime → safe_clear (project.c:1958, song_main.c:157 and :425; voice.c:319-323), which resets safe_part to NTRK and nothing re-arms it | S-M | yes, under overload only | |
| 5 | [piece 6] user edits on a GEN texture/arp track don't clear safe_part | OPEN | Only a SEQ dice clears it (gen.c:3055-3056); no engine, preset, step or clear path touches safe_part (grep: audio.c, voice.c, gen.c only), so under overload the net can hush a part the user rewrote | S-M | yes, under overload | |
| 6 | [piece 6] a cleared lever has no release stamp | OPEN | Release by calm stamps it (audio.c, `dsf_since[l] = 0`). safe_clear (voice.c:319-323) zeroes fx_safe but leaves dsf_since / dsf_rearm (audio.c:198-200), so a lever dropped by new music can flap once | S | no (inaudible) | |
| 7 | [piece 6] GC_GEN_X100 150 pending device data | DONE | recalibrated from the device to 118 (gen.c:2116; 7568d5c) | | | |
| 8 | KIT dice: BEFORE/NOW bars show the HOME knobs, which kit dice doesn't move | OPEN | needs design work (the kit's lane values) | M | yes | |
| 9 | SEQ dice: OCT- doesn't restore the level dlevel() set | OPEN, verified | dk_keep / dk_back save and restore steps + P_SLEN only (ui_input.c:1889-1915); dice_seq calls dlevel (gen.c:3071), which lowers P_LEVEL (song_dice.c:58-69). Dice, then OCT- back, leaves the track quieter | S | yes | |
| 10 | dice dialogs' lost line on the status line; UXD_LOST_HDR '%'; NM_TAB_H duplicates DLG_TAB_H | OPEN | ui_name.c:438 NM_TAB_H | S | no | |
| 11 | REDO into the CPU TEST state restores CLOCK INT, cpt_clk 0 | OPEN | not addressed (a11bb5f #4 fixed only the autosave case) | S | yes (rare) | |
| 12 | GEN header under USB/TRS crowded ("GEN: EXPLORE 43..") | OBSOLETE | EXPLORE gone; 0.9 names with a short-name fallback past 232 px (gen.c gen_build) | | | |
| 13 | .pool spare 9,888 B (0.6 snapshot) | OBSOLETE | stale figure; 0.9 image 563,140 B, 21,196 B free (device-log.md:184-185) | | | |
| 14 | ux_msg_scan.py scans PD/PE labels, not enum word tables | OPEN | unchanged | S | no | |
| 15 | PLAYS and MIDI routing per section vs per project; song playback ignores B's stored PLAYS | OPEN | a decision is pending; sec_apply copies only the SLICER globals (song_play.c:9-44) | M | yes | |
| 16 | MORE panel signature hashes every P_* mark (one extra redraw) | OPEN | | S | no | |
| 17 | SAVE > MORE refusals say STOP TO EDIT before EMPTY SLOT / STOP TO SAVE | OPEN (by the brief) | intended, per the brief | S | yes | |
| 18 | MENU status line repeats the panel hint; armed action let go on reopen | OPEN (part) | reopen arms the action (8d2e68d, DONE); the repeated hint is still open | S | yes | |
| 19 | knob labels shortened (CALIBRATE, USB, GLOW, DELETE/CLEAR …) | OPEN (info) | | | | |
| 20 | web editor MIDI tab not rendered in a browser | OPEN (blocked) | a visual check needs a browser session (Playwright / node absent) | S | yes | Playwright |
| 21 | save_on / LZ_SAVE leftover | OPEN | ui.c:641 save_on, :659 lz_on, :668 LZ_SAVE; ui_lens.c:25 | S | no | |
| 22 | SAVE layer JIANT overlay gone; maps in ux_layer.c | DONE (info) | | | | |
| 23 | gen_readme_hero.py JIANT images | OPEN (dup) | piece6 #35 | | | |
| 24 | MANUAL older page words | DONE | see 3c1 #17 | | | |
| 25 | "from the 3c-1 list, still open" bundle | OPEN (dup, part) | per 3c1 rows: two TRIGs, K+SEQ and NO RESTART DONE; lt_mch, forget, bend, thin tests, layer hints, mark rule (part), 9 B case OPEN | | | |
| 26 | [device] no-manual checks; CPU TEST on SAVE SONG / MENU MORE / dialog; CLOCK after CPU TEST; editor MIDI page | OPEN (part) | done: device-log.md:126-130. Not logged: CLOCK back after CPU TEST, the editor's MIDI page | S | yes | device |
| 27 | no Playwright | OPEN (blocked, dup) | piece6 #36/#37/#39 | | | Playwright |
| 28 | no emcc | OPEN (blocked, dup) | piece6 #38 | | | emcc |

## piece5 (2026-10-30, release 0.8)

| # | Item | Status | Evidence / note | Size | UV | Needs |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TAPE step locks (only AGE automation) | OPEN | feature | M | yes | |
| 2 | GEN recipe hooks for the 0.8 presets/kits | OPEN | gen.c:1399-1404 gen_npre still caps at the 0.7 counts. Changes every piece and its goldens | M | yes | |
| 3 | a GEN genre that sets AGE | DONE | 12ca184 AUTO AGE per style; 883a2e3 RETROBOC/AMBIENT keep their tape | | | |
| 4 | AGE as a mod-matrix destination | OPEN | feature; not in the DEST list (cheatsheet/sound.html) | M | yes | |
| 5 | TAPE strip look on FX MASTER (first cut) | OPEN | design | M | yes | |
| 6 | tune the AGE map by ear; WOW 8.7 cents | DONE (map) | 792ec88 retune; device-log.md:207, 215. Widening WOW: piece6 #4 | | | |
| 7 | HISS follower ~60 ms tail; master_rest waits | OPEN (info) | | | | |
| 8 | tape_block per-block divides → shifts/reciprocal | OPEN | 3 divides in tape_block (fx.c:510+). Matters now that tape_block sits at its 780 budget (piece6 D3) | S-M | no | |
| 9 | TAPE in/out click test near the floor (28/32) | OPEN | brittle test | S | no | |
| 10 | left shifts of negatives in tape_block (UB in ISO C) | OPEN | cosmetic (GCC defines them) | S | no | |
| 11 | SAT input wraps above ~2.8M | OPEN (info) | covered by the Task 5 clamp | | | |
| 12 | a project within 9 B of full can become full from the 'TP' block, untold | OPEN | same class as 3c1 #22 | S-M | yes (rare) | |
| 13 | 'TP' / 'MG' readers near-duplicates | OPEN | project.c:792-900 | S | no | |
| 14 | 'TP' room bound never hit by a test | OPEN | | S | no | |
| 15 | proj_music_sig reads motion_age_* unguarded | OPEN | at worst one spurious autosave | S | no | |
| 16 | WOW turned by hand while AGE records play is dropped | DONE | documented as intended (0.8 final review M2, 439fefd) | | | |
| 17 | motion_age_set three guarded sections (lost detent) | OPEN | | S | no | |
| 18 | no test: AGE knob while records play with REC off | OPEN | | S | no | |
| 19 | should TAPE fade before voices are shed? (device CPU TEST) | OPEN | device decision | S | yes | device |
| 20 | GEN pieces with TAPE over budget; GEN budgets TAPE off | DONE | 12ca184 every piece fitted with TAPE counted | | | |
| 21 | GC_TAPE from host counts; confirm on the device | OPEN (dup) | piece6 #5 (GC_TAPE2) | | | device |
| 22 | song/cputest.tape baseline line dropped | OPEN (blocked) | needs the reference host / valgrind | S | no | valgrind |
| 23 | valgrind CPU baseline not run | OPEN (blocked) | | | | valgrind |
| 24 | gen_test pins GT_TAPE_C0_ONE/TWO | OBSOLETE | no GT_TAPE_C0 in tests/gen_test.c after the 0.9 recalibration | | | |
| 25 | acid_test never overlaps keys (LEGATO glide) | OPEN | test gap | S | no | |
| 26 | MONO slide from a released note; sweep reset on mode/engine change untested | OPEN | test gap | S | no | |
| 27 | comment the discriminating check acid_test.c:404-406 | OPEN | | S | no | |
| 28 | MONO slide keeps the higher velocity | OPEN (info) | | | | |
| 29 | sweep and accent hold through rests | OPEN (info) | | | | |
| 30 | x48 accent depth for keys above vel 110 | OPEN | decision; changes 7 ANALOG goldens | S | yes | |
| 31 | 45 renders missing from golden.txt | OPEN (dup) | piece2 #6 | | | |
| 32 | acid line's CPU on the device CPU TEST | OPEN | fold into the second calibration | S | no | device |
| 33 | voice_test skips wordless choirs; add a vowel-choir bound | OPEN | voice_test.c:959 skips them | S | no | |
| 34 | preset-name glyph test narrower than UF_VAL | OPEN | | S | no | |
| 35 | CUT slot stored value not range-checked | OPEN | | S | no | |
| 36 | 808 BOOM kick rings at 3 s (MORPH 127) | OPEN | judge by ear | — | yes | device |
| 37 | "crunchier" judged by ear | OPEN | | — | yes | device |
| 38 | kit 3 snare crest margin 0.3 % | OPEN | brittle test | S | no | |
| 39 | cowbell DFT loop duplicates t8_band | OPEN | cosmetic | S | no | |
| 40 | cowbell retune by ear | OPEN | | — | yes | device |
| 41 | uxm_bit lacks its own i < UXM_MAXROWS guard | OPEN | ux_more.c:194 | S | no | |
| 42 | weak asserts in test_m2_wide | OPEN | | S | no | |
| 43 | section hash fold lets rows i and i+32 cancel | OPEN (info) | | | | |
| 44 | ux_menu signature uses the first 64 rows only | OPEN (info) | | | | |
| 45 | cheat-sheet PNG/PDF, navigation-map.png not re-rendered | OPEN (dup) | piece6 #36/#37 | | | Playwright (Chromium workaround) |
| 46 | 0.8 news omits "built on JIANT FM and Felucca" | OPEN (info) | ruling: built_on_test allows JIANT only on the credits line | | | |
| 47 | past text names Aphex Twin / Rephlex | OPEN (info) | T9 ruling: left as past entries | | | |
| 48 | older "short mode" delay wording in the 0.5.2 entries | OPEN (info) | past entries | | | |
| 49 | merge to main and push on go-ahead | DONE | device-log.md:216 | | | |
| 50 | TAPE / AGE hard to find (FX ×3) | OPEN (dup) | piece6 #1 | | | |
| 51 | GEN does not budget for TAPE | DONE | 0.9 (12ca184, 333d6d1, 883a2e3) | | | |
| 52 | check "TAPE PAUSED · CPU" against the DUCK curve | DONE | closed in the list (rendered worst case) | | | |
| 53 | judge AGE by ear; retune | DONE | 792ec88; device-log.md:207, 215 | | | |
| 54 | (checked) TAPE PAUSED vs DUCK: closed | DONE | | | | |
| 55 | (checked) AGE measured: retune so the middle is clearly worn | DONE | 792ec88 (AGE 50 clearly worn, 100 mangled) | | | |

## piece6 (2026-10-31, release 0.9)

| # | Item | Status | Evidence / note | Size | UV | Needs |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | quicker way to reach TAPE / AGE | OPEN | no shortcut added (ux_layer.c has none); the 0.8 no-manual task failed | M (design) | yes | |
| 2 | mixing styles per role (approach B) | OPEN | feature | L | yes | |
| 3 | more styles in the GENERATE pool | OPEN | feature | M-L | yes | |
| 4 | widen TAPE's line for deeper WOW | OPEN | feature: ~2 KB RAM, latency, GC_TAPE | M | yes | |
| 5 | measure GC_TAPE from the device (GC_TAPE2 92, one snapshot) | OPEN | part of the second calibration | M | no | device |
| 6 | TAPE's model cost depending on AGE | OPEN | model change, after the calibration data | M | no | |
| 7 | judge AGE 75 and 100 by ear | OPEN | | — | yes | device |
| 8 | AGE 50 noise margin 0.2 dB over its test floor | OPEN | brittle test | S | no | |
| 9 | device pitch window under-reads slow wow | DONE | tape-wow-debug-2026-10-31.md: the measurement was at fault; device equals host to 0.02 cents (device-log.md:210-214) | | | |
| 10 | tape_test "each detail rising" label overstates | OPEN | | S | no | |
| 11 | tape_test wobble sd includes hiss; "darker at 100" near-tautologies | OPEN | | S | no | |
| 12 | one-core mode uncalibrated (297/300 pieces to the floor) | OPEN | | L | yes (1 CORE users) | device |
| 13 | replay 1751/1174/8794 with chord COMP off (GC_CCOMP 70) | OPEN | calibration | M | no | device |
| 14 | 2-3 FM6-heavy pieces, 40 s each | OPEN | calibration | S | no | device |
| 15 | one piece at AGE 0 vs 50, 20 s windows | OPEN | calibration | S | no | device |
| 16 | repeat each device window twice (spread) | OPEN | calibration | S | no | device |
| 17 | styles factor ×1.18, WILD ×1.55, 7848 label on few points | OPEN | calibration | M | no | device |
| 18 | textures and arps go first when the fit is tight | OPEN | design / listening | M | yes | device |
| 19 | AMBIENT / RETROBOC "too bare"? | DONE | device-log.md:207-208: AMBIENT 4144 "sounds good (not too bare)", RETROBOC+AMBIENT 7692 passes (one piece each) | | | |
| 20 | a WILD piece can lose its arp/lead to TAPE at estimate time | OPEN | | M | yes | |
| 21 | gc_cost skips muted tracks in a section fit (muted then unmuted live) | OPEN | gen.c:2366-2372 | M | yes, under overload | |
| 22 | one AGE per piece (5/549 later sections over budget) | OPEN | design | M | no | |
| 23 | 0.5.2 EXPLORE (now WILD) over-budget bound 6 % | OPEN | tighten once the model improves | S | no | |
| 24 | one-core "TAPE counted" test weak | OPEN | will flip with #12 | S | no | |
| 25 | gs.grain_off / gs.cost_thin in firmware RAM for tests only | OPEN | gen.c:112-114 | S | no | |
| 26 | gen_make head clears outside load_begin (torn read) | OPEN | | M | yes (a moment) | |
| 27 | seeds fold % 10000: collisions in some sweeps | OPEN (info) | | | | |
| 28 | 0.5.2 over-budget print shows gp.seed, not gp.id.seed | OPEN | test print can name the wrong piece | S | no | |
| 29 | hush_test prints an uncompared hash | OPEN | | S | no | |
| 30 | a rebuilt recent piece follows the tracks as left | DONE (documented) | 15c8ba2 M3: RECALL is "the same piece while TRACKS, BPM LOCK and the clock are the same" | | | |
| 31 | tool paths (gen_go, gen_press) push the recent ring | OPEN | gen.c:2939 gen_build always pushes | S | no | |
| 32 | GEN panel preview density fixed at 550 | OPEN | design | S | yes | |
| 33 | panel rows that never light | OPEN | design note | S-M | yes | |
| 34 | MANUAL internals say "genre" | OPEN (info) | deliberate | | | |
| 35 | tools/gen_readme_hero.py legacy | OPEN | names shots that no longer exist (save_arrange, gen_explore). Delete or repoint; JIANT-README + built_on_test constrain it | M | no | |
| 36 | cheat-sheet PNG/PDF not rendered | OPEN | docs/cheatsheet-*.png/.pdf last cdae0ae (0.5 era; the printed sheet still shows DUCK TIME on MASTER). Playwright absent, but `/usr/bin/chromium` headless is here (used for the nav map in 3b) | S-M | yes | Chromium workaround (not Playwright) |
| 37 | navigation-map.png not rendered | OPEN | PNG last aec409f (0.7); the SVG is current. Same Chromium workaround | S | yes | as #36 |
| 38 | browser emulator (emcc) never built locally | OPEN (blocked) | CI builds it | | | emcc |
| 39 | manual in Chromium via Playwright; valgrind CPU check | OPEN (blocked) | | | | Playwright, valgrind |
| 40 | V15 restore test skipped (input file absent) | OPEN (blocked) | install_test.py:332-341 needs FM-1_v15.fwsc (vendor file; no download) | | | file |
| D1 | about half of 0.9's GEN pieces run late: second calibration on 0.9's own pieces | OPEN | device-log.md:188-206 (late 0-118 per 20 s, lost 0, no clicks). Rows were partly unnamed; console `status` prints no piece identity or model estimate (console.c:204-212) | L | yes | device |
| D2 | deeper / smoother WOW (a longer line) | OPEN (dup) | #4 | | | |
| D3 | tape_block cost exactly at its budget (780) | OPEN | no headroom for TAPE changes; see piece5 #8 | M | no | |

---

## Step 2: the OPEN items grouped

### (a) Bugs a user could hit
- 3c2 #9: SEQ dice then OCT- leaves the track quieter (level not restored). S. **Verified.**
- 3a #16: GEN's hint only after a HOME tap, not on HOME+SELECT. S.
- 3c2 #4: GEN texture/arp levers not re-armed after a section load or SONG END. S-M. Heard only under overload.
- 3c2 #5: user edits on a GEN texture/arp track don't clear safe_part, so the net may hush the user's own part. S-M.
- 3c2 #6: a lever cleared by safe_clear has no release stamp and can flap once. S. Inaudible.
- 3c1 #5: MIDI bend not zeroed when KEYBOARD PLAYS SELECTED moves. S.
- 3c1 #3 and #4: lt_mch per track; midi_forget_track uses current claims. Possible hung notes on outboard gear. M, rare.
- 3c2 #11: REDO into the CPU TEST state brings CLOCK back as INT. S, rare.
- 3c2 #15: song playback ignores a section's stored PLAYS and routing. M, needs a decision.
- 3c2 #8: KIT dice BEFORE/NOW bars equal. M, design.
- 3c1 #22 and piece5 #12: a project near full becomes SECTION FULL after a routing or TAPE change, untold. S-M, rare.
- 3c1 #16: no automation mark on DJ FILTER or the SEQ step pages. S-M.
- 3b #19: FM6 ENV source does nothing. S. Unverified.
- piece6 #21, #26: muted-then-unmuted live track not costed; gen_make torn read. M each.
- piece6 D1: half the GEN pieces late on the device. L, device.
- piece6 #12: one-core pieces thinned to the floor. L, device.

### (b) Clean-up and test gaps
- Dead code and leftovers: save_on/LZ_SAVE (3c2 #21), knob-arc tables (3c1 #10), gs.grain_off/cost_thin (piece6 #25), NSONG assert (piece2 #2), P_WRDEP/ROTOR row (piece2 #7), NM_TAB_H (3c2 #10), layer leftovers (3c1 #12), uxm_bit guard (piece5 #41), 'TP'/'MG' reader duplication (piece5 #13), tape_block divides and UB shifts (piece5 #8, #10), gfx.c comment (3a #18), gen_aa_font PFX (3a #19), long lines (piece1 #7), cowbell DFT duplication (piece5 #39).
- Tool hardening: segno ImportError (piece1 #9), build.py isascii (piece2 #8), VAR_DEF pin guard (3b #14), ux_msg_scan explicit _LBL list (3c1 #9, now due), enum word tables (3c2 #14).
- Test gaps and brittleness: golden hashes for 45 presets (piece2 #6), LARGE footer (3a #13), VISUAL write-per-pixel (3a #21), STEP EXTRAS equality (3c1 #7), pace per 1/16 (3c1 #8), layer tests (3c1 #13), TAPE click margin (piece5 #9), 'TP' bound (piece5 #14), AGE knob with REC off (piece5 #18), acid_test gaps (piece5 #25-27), choir bound (piece5 #33), glyph test (piece5 #34), CUT range (piece5 #35), crest margin (piece5 #38), test_m2_wide (piece5 #42), AGE 50 margin (piece6 #8), tape_test labels and tautologies (piece6 #10-11), WILD bound (piece6 #23), one-core TAPE test (piece6 #24), seed print (piece6 #28), hush_test hash (piece6 #29).
- Minor UI polish: MORE signature redraw (3c2 #16), MENU repeated hint (3c2 #18), MORE sel per section (3b #17), ARP spacing (3b #20), small-face labels (3b #15), DRIFT naming and titles (3b #16), FM6 DRIVE (3b #18), glow banding (3a #6), proj_music_sig guard (piece5 #15), motion_age_set (piece5 #17), recent ring from tools (piece6 #31).

### (c) Docs and roadmap
- BREAK still on the README intro and roadmap and on the landing page (3c2 #1).
- The installer page's "0.6.71 … not tried on a real unit yet" two-core warning, in EN/JA/ES (piece1 #4).
- Cheat-sheet PNG/PDF and navigation-map.png stale (piece6 #36, #37). Chromium is available.
- emulator.yml JIANT comment (piece1 #5); baseline.md SPDX (piece1 #8); flash-map.md long lines (piece2 #9).
- Legacy tools: gen_readme_hero.py (piece6 #35, 3a #12, 3c1 #18) and tools/video (3a #11, 3b #12).
- Info and rulings, no action: 3b #13, 3c2 #19, piece5 #46-48, piece6 #34.
- Not an item: `web/STABLE` is empty. Pin it.

### (d) Features and ideas
- TAPE / AGE quick access (piece6 #1); TAPE locks (piece5 #1); AGE as a mod destination (piece5 #4); TAPE strip look (piece5 #5); deeper WOW (piece6 #4).
- GEN: the 0.8 presets in recipes (piece5 #2); mixing per role (piece6 #2); more styles (piece6 #3); TAPE cost by AGE (piece6 #6); one AGE per piece (piece6 #22); textures and arps first (piece6 #18); WILD estimate (piece6 #20); preview density and rows (piece6 #32-33).
- Others: x48 accent for keys (piece5 #30); SCL duplicates (3b #6); TAPE fade order (piece5 #19).

### (e) Blocked
- Absent tools:
  - Playwright: the renders, the test_manual Chromium part and the editor MIDI tab (3c1 #20, 3c2 #20/#27, piece5 #45, piece6 #39). The renders have a Chromium workaround.
  - emcc: the emulator (3c1 #21, 3c2 #28, piece6 #38).
  - valgrind: the CPU baselines (piece5 #22-23, piece6 #39).
  - node: web/test_web.mjs, so CI must check any index_pkg.html edit.
  - V15 vendor file (piece6 #40).
- Device: second calibration (piece6 D1, #5, #13-17; piece5 #21, #32); one-core calibration (piece6 #12); AGE 75/100 and kits by ear (piece6 #7; piece5 #36-37, #40); TAPE fade decision (piece5 #19); MIDI/USB no-manual checks (3c1 #19); CLOCK after CPU TEST and the editor MIDI page (3c2 #26).

---

## Step 3: proposed 0.9.1 scope (14 items)

| # | Item (source) | Size | UV | Needs |
| --- | --- | --- | --- | --- |
| 1 | Second CPU calibration on 0.9's own pieces (piece6 D1; folds in #5, #13-17, piece5 #21, #32). First add the piece's identity and the model's cost/budget to the console `status`, so every device row is named. Then run a device session (two windows per piece, AGE 0 vs 50, FM6-heavy pieces, chord COMP off), re-fit GC_* and re-run gen_test | L | yes (fewer late blocks) | device |
| 2 | Re-arm the GEN texture/arp levers after a section load or SONG END (3c2 #4) | S-M | yes, under overload | |
| 3 | User edits on a GEN texture/arp track clear safe_part (3c2 #5) | S-M | yes, under overload | |
| 4 | A lever cleared by safe_clear gets a release stamp (3c2 #6) | S | no | |
| 5 | GEN's hint on every way into GEN, not only a HOME tap (3a #16) | S | yes | |
| 6 | SEQ dice OCT- restores the track level (3c2 #9) | S | yes | |
| 7 | MIDI bend zeroed when KEYBOARD PLAYS SELECTED moves (3c1 #5) | S | yes | |
| 8 | BREAK out of the README intro and roadmap and the landing page; write the next roadmap (3c2 #1) | S | yes | |
| 9 | Installer page: drop the stale "0.6.71 … experimental, not tried on a real unit" warning (piece1 #4) | S | yes | CI for test_web.mjs |
| 10 | Re-render the cheat-sheet PNG/PDF and navigation-map.png with the system Chromium (piece6 #36-37) | S-M | yes | |
| 11 | Dead-code sweep: save_on/LZ_SAVE, knob-arc tables, gs.grain_off/cost_thin, NSONG static assert (3c2 #21, 3c1 #10, piece6 #25, piece2 #2) | S-M | no | |
| 12 | Tool hardening: segno-missing message/skip, build.py isascii, explicit _LBL list in ux_msg_scan (piece1 #9, piece2 #8, 3c1 #9) | S | no | |
| 13 | Docs and comment tidy: emulator.yml JIANT comment, baseline.md SPDX, flash-map.md and editor.c/felucca.c long lines (piece1 #5, #7, #8; piece2 #9) | S | no | |
| 14 | Golden hashes for the 45 presets without one (piece2 #6 / piece5 #31) | S-M | no | |

Release step: once 0.9.1 is device-proven, pin `web/STABLE`.

Items 2-4 touch the same code (voice.c safe_clear, gen.c gen_build, audio.c dsf_*) and are best done as one task with
one audio_test case each. Item 1's device session should run after 2-4, so the levers are right while it is measured.

### Left for later, and why
- **TAPE / AGE quick access (piece6 #1).** The UX gap is real (a failed no-manual task), but it needs a design
  decision on where the shortcut goes. Make it the first item of the next feature release.
- **PLAYS and routing per section or per project (3c2 #15)** and **KIT dice bars (3c2 #8).** Both need a design
  decision first.
- **MIDI lt_mch and midi_forget_track (3c1 #3-4).** Size M; the cases are rare and need outboard gear to verify.
- **One-core calibration (piece6 #12).** L, device. Do it after the two-core calibration has settled the model.
- **gen_make torn read, muted-live cost, WILD estimate, one AGE per piece (piece6 #26, #21, #20, #22).** M each.
  They change the fitter; batch them with the post-calibration model work.
- **GEN using the 0.8 presets (piece5 #2), mixing per role, more styles, TAPE locks, AGE as a mod destination, deeper
  WOW (piece5 #1, #4; piece6 #2-4).** Features. They change goldens and identities, or cost RAM and CPU (tape_block
  already sits at its 780 budget).
- **tape_block speed-up (piece5 #8, piece6 D3).** Worth doing before any TAPE feature, not in a clean-up release
  without a device CPU check.
- **Near-full project warning (3c1 #22, piece5 #12).** Rare; it needs a UI message design.
- **Most test-tightening items (group b).** Cheap but not user-facing. Pick them up next to related code changes.
- **Legacy tools (gen_readme_hero.py, tools/video).** Constrained by JIANT-README and built_on_test, and nobody uses
  them.
- **Blocked items (emcc, valgrind, Playwright-only checks, V15).** No tools here, and installing is forbidden. Rely
  on CI.

### Items I couldn't classify confidently
- 3a #6 (glow ring banding): needs a look at a render.
- 3b #16 (the GR_LYRIC / MORPH "rule holders" and ed_resp parts): the wording is unclear.
- 3b #19 (ENV source on FM6): couldn't pin down which control is meant.
- 3c2 #17 (refusal order "by the brief"): possibly intended. All four are listed as OPEN.
