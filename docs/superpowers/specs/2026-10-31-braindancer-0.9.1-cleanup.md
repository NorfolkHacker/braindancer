# Braindancer 0.9.1: the clean-up release

Date: 2026-10-31. Status: scope approved by the user (2026-10-31, "yes" to the 14 items). Source: the triage of all eight
deferred lists, `docs/superpowers/plans/2026-10-31-braindancer-0.9.1-triage.md` (217 items: 62 done, 7 obsolete, 148
open), which gives each item's evidence (file:line). Release: Braindancer **0.9.1**.

## 1. User-visible fixes

1. **Second CPU calibration** (on 0.9's own pieces; spec piece 6 §3 "Recalibration").
   - First, the serial console's `status` names the playing piece (`gen_piece` = the panel's name and number, or `-`)
     and GEN's model figures for it (`gen_cost`, `gen_budget`), so no device row is unnamed.
   - Then a device session: the user presses GENERATE ~20 times (the controller reads the counters), 2 CORES, the stored
     CPU TEST. The model is re-fitted from those rows (and 0.8's 15), so a piece the device plays late (audio_late more
     than ~5 per 20 s) is over the budget and fitted. Target on the device: at most 1 in 10 pieces late, no audio lost,
     the tape styles keeping their tape.
   - gen_test's budget figures re-recorded; the two-core bound (≤ 4 % over) holds.
2. **The safety net after a section change**: a GEN piece's texture / arp levers (`safe_part`) are re-armed when a
   section loads and after SONG END (today `safe_clear` empties them and nothing sets them again).
3. **The user's own edits**: an edit of a GEN texture / arp track (steps, sound, engine) takes that track out of
   `safe_part`, so the safety net never silences a part the user rewrote.
4. **A cleared lever** (`safe_clear`) gets its release stamp, so it cannot flap once on the next overload.
5. **GEN's hint** shows on every way into GEN (HOME taps, SELECT, HOME + SELECT), not only after a HOME tap.
6. **SEQ dice, then OCT−** restores the track's level as well as its steps and length.
7. **Pitch bend**: KEYBOARD PLAYS SELECTED moving to another track zeroes the bend on the track it leaves.

## 2. Docs and pages

8. BREAK (dropped 2026-10-28) leaves the README (intro and roadmap) and the landing page (tagline and roadmap item).
9. The web installer's stale "two cores, experimental, not tried on a real unit" warning (EN / JA / ES) goes.
10. The printed cheat sheets (PNG / PDF) and `docs/navigation-map.png` re-rendered with the system Chromium
    (`/usr/bin/chromium`, headless; Playwright is absent and nothing may be installed). If a renderer cannot run with
    what is installed, report it and leave them.

## 3. Behind the scenes

11. Dead code out: `save_on` / `LZ_SAVE`, the unused knob-arc tables, the test-only `gs.grain_off` / `gs.cost_thin`
    (tests read them another way); a static assert tying the two NSONG definitions.
12. Tools: a clear message when segno is missing; `isascii` in build.py; an explicit `_LBL` list in ux_msg_scan.
13. Comments and docs tidy: the emulator.yml JIANT comment, baseline.md's SPDX header, long lines in flash-map.md,
    editor.c, felucca.c.
14. Golden hashes for the 45 renders that have none (only these lines added to tests/golden.txt).

## 4. Limits

- No storage format change; budgets ISR 300 / TIMER5 2170 / slicer 821 / tape 780; image ≥ 16 KiB free; the pin guard.
- Sound unchanged except where an item says so (item 1 may thin some GEN pieces more; the goldens of existing renders
  unchanged unless item 1 moves a GEN one, named).
- Plain English, no band names; JIANT only in the credits line. No downloads or installs.

## 5. Release

Docs and the 0.9.1 news entry; the whole-branch review; the device session (item 1's measurements, plus a look at the
hint and the dice level); merge and push on the user's go-ahead. After 0.9.1 is proven on the device, pin `web/STABLE`
(today empty: the public installer builds main) — asking the user first.
