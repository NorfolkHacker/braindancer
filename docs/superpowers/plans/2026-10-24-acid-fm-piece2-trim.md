# ACID FM piece 2 (trim) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ACID FM drops the ANIMAL and WHEEL engines, keeps 4 songs instead of 8 and reserves the freed song flash as
the sample area for breakbeats, finds out which other flash gaps are truly free, and runs as many voices as the FM-1
can handle, measured on the device.

**Architecture:** Engines are retired the way JIANT retired SAMPLE, GRAIN and SLICE (commit `3aff2d4`): code and
assets go, the engine numbers stay reserved, and any stored sound that names one loads as ANALOG's first preset. Songs
5..8 leave the storage map (the song index keeps its 8-row layout so stored indexes stay readable) and their sectors,
0xB8000..0xD7FFF, become `FL_SMP_LO..FL_SMP_HI`, guarded by compile-time checks; piece 4 writes there. `NVOICE` becomes
a build option so 8, 10 and 12 voices can be built, budgeted on the host and measured on the device with JIANT's
CPU TEST song.

**Tech Stack:** C (JieLi clang for the device, host gcc via `tools/host-cc` for the tests), Python build tools, JS web
editor and tests (Node in `build/node`).

**Spec:** `docs/superpowers/specs/2026-10-24-acid-fm-design.md` (§4 engines, §5 voices, §6.1 storage, §9 testing,
§10 item 2).

## Global Constraints

- Licence GPL-3.0-only. Never remove a JIANT or Felucca copyright line. A file we change gains
  `Portions Copyright (C) 2026 NorfolkHacker` under its existing copyright lines (C inside the header comment; Python /
  shell `#`; JS `//`; HTML in its header comment). Files with no copyright header get none.
- 6 tracks (`NPART 6`) unchanged. VOICE stays complete. JIANT's 43 genres stay (piece 6 replaces GEN).
- Engine numbers are append-only and never reused: ANIMAL = 14 and WHEEL = 7 stay reserved; `NENGINES` stays 15.
- Songs: 4. Sample area: `0xB8000..0xD7FFF` (128 KiB). Nothing in this piece writes to it.
- Voices: the measured ceiling (§5): the largest of 8 / 10 / 12 whose CPU TEST song plays with LATE 0 and a mean load
  under 60 %; `NVOICE` is set to it.
- No sound change except where an engine is removed or the voice count changes: every other golden render hash must
  stay the same. Golden / budget updates (`GOLDEN_UPDATE=1`, `BUDGET_UPDATE=1`) only for those intended changes, with
  the changed entries listed in the commit message.
- Environment for every task:
  `cd ~/synth/mvavefirmware && . .venv/bin/activate && export JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK PATH=$PWD/build/node/bin:$PATH`
  Build `./build.sh`; full suite `CC=tools/host-cc sh tests/run_tests.sh` (web groups need Node from `build/node`;
  the emulator group skips without emcc).
- Branch `piece2-trim` (already created from `main`, holds the spec update). Commit messages end with a blank line and
  `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Never commit on `main`, never push.
- Device steps are **USER STEPS** and run only after a full-flash backup exists for the build on the device
  (piece 1's backup at `~/fm1-backups/2026-10-24-jiant/` covers a return to JIANT; take a fresh one before the first
  piece 2 install, as in Task 5).

## File map

| File | Change |
|---|---|
| `firmware/src/eng_animal.c`, `firmware/src/eng_wheel.c`, `tests/animal_test.c` | deleted |
| `firmware/src/core.h` | `eng_gone()` takes ANIMAL and WHEEL; `NENG_SHOWN`; `NVOICE` overridable |
| `firmware/src/engines.c` | `ENGINES[]` 7 and 14 → `ENG_GONE`; `ENGINE_ORDER`; power-on part T3 |
| `firmware/src/params.c`, `project.c`, `midi_control.c`, `voice.c`, `icons.c`, `ui.c`, `ui_graph.c`, `ui_being.c`, `ui_engvis.c`, `gen.c`, `gen_tables.c`, `main.c` | references to the two engines removed / redirected |
| `firmware/src/song_chain.c`, `firmware/src/storage.c` | 4 songs; storage objects for songs 5..8 gone |
| `firmware/hal/fm1_flash.h` | `FL_SMP_LO` / `FL_SMP_HI`, compile-time guards |
| `tools/build.py` | `FELUCCA_NVOICE` passed through |
| tests: `project_test.c`, `mod_test.c`, `gen_test.c`, `midi_control_test.c`, `regress.c`, `cpu_vg.c`, `target_budget.py`, `ui_test.c`, `ui_render.c`, `storage_test.c`, `backup_test.c`, `golden.txt`, `cpu_baseline*.txt`, `target_budget.txt`; `web/test_web.mjs`, `web/editor.html`, `web/manual/data.js`, `web/manual/nav.js` | follow the changes |
| `docs/acid-fm/flash-map.md` | new: the measured flash map and the gap investigation |
| `docs/acid-fm/voices.md` | new: the voice-ceiling measurements |
| `docs/acid-fm/device-log.md` | piece 2 entries |
| `README.md`, `web/landing/index.html` | status: piece 2 |

---

### Task 1: Retire ANIMAL and WHEEL

**Files:** delete `firmware/src/eng_animal.c`, `firmware/src/eng_wheel.c`, `tests/animal_test.c`; modify the
firmware, test and web files listed in the file map that reference `ENGI_ANIMAL`, `ENGI_WHEEL`, `eng_animal`,
`eng_wheel`, `"ANIMAL"`, `"WHEEL"`.

**Interfaces:**
- Produces: `eng_gone(ENGI_ANIMAL) == eng_gone(ENGI_WHEEL) == 1`; `ENGINES[7]` and `ENGINES[14]` are `&ENG_GONE`
  (named `-` in INFO and `G_ENGSEL`); `NENG_SHOWN` is two smaller; the power-on T3 part is PHASE.

The template is JIANT's own retirement commit. Read it before anything else:

```bash
git show 3aff2d4 -- firmware/src/core.h firmware/src/engines.c firmware/src/params.c firmware/src/project.c \
  firmware/src/ui.c firmware/src/ui_input.c firmware/src/midi_control.c firmware/src/upreset.c tests/golden.txt
```

- [ ] **Step 1: Failing tests**

In `tests/project_test.c`, next to the existing checks that a stored SAMPLE / GRAIN / SLICE sound loads as ANALOG's
first preset (`grep -n "eng_gone\|ENGI_SLICE\|ENGI_GRAIN" tests/project_test.c` finds them), add the same check for a
stored part whose engine is `ENGI_ANIMAL` and one whose engine is `ENGI_WHEEL`, written the same way as those checks:
the part loads as `ENGI_ANALOG` with ANALOG's first preset's parameters, its engine motion dropped. Add checks that
`eng_gone(ENGI_ANIMAL)` and `eng_gone(ENGI_WHEEL)` are true and that neither number appears in `ENGINE_ORDER` (via
`eng_vis(n)` for every `n < NENG_SHOWN`). If the user-preset migration test (`upreset`) has a retired-engine case, add
ANIMAL and WHEEL there too.

Run `CC=tools/host-cc sh tests/run_tests.sh 2>&1 | grep -E "FAIL" | head`. Expected: the new checks FAIL.

- [ ] **Step 2: Retire them**

1. `core.h`: add `|| e == ENGI_ANIMAL || e == ENGI_WHEEL` to `eng_gone()`; update `NENG_SHOWN` (the comment says
   which engines are not shown; it becomes `NENGINES - !FELUCCA_FM4 - 7`) and its comment.
2. `engines.c`: `ENGINES[7]` and `ENGINES[14]` → `&ENG_GONE` with a comment `/* 7: reserved (WHEEL, retired in ACID
   FM) */` / `/* 14: reserved (ANIMAL, retired in ACID FM) */`; drop both from `ENGINE_ORDER`. Power-on parts
   (the 0.9.9 table, "T3 ANIMAL's bird"): T3 becomes PHASE with PHASE's first preset and the pattern T3 had; update
   the comment. The "parts at power-on through 0.9.8" table (what an old project's tracks load as) maps any ANIMAL /
   WHEEL entry to `eng_gone` handling (ANALOG's first preset).
3. Delete `eng_animal.c`, `eng_wheel.c`; remove their `extern const engine_t ...` declarations and every reference:
   icons (`icons.c`, and `tools/gen_aa_icons.py` if it lists them), parameter tables (`params.c`), the creature
   drawings (`ui_being.c`, `ui_engvis.c`, `ui_graph.c`, `ui.c`), MIDI program-change numbering (`midi_control.c`:
   numbering follows the display order, so it shifts; keep the code's existing rule), `voice.c`, `main.c`.
4. GEN (`gen.c`, `gen_tables.c`): JIANT's genres stay, but no role may pick a retired engine. Where a genre or EXPLORE
   picks ANIMAL or WHEEL, substitute: ANIMAL → NOISE, WHEEL → FM6 (the closest kept engines). Keep the tables' shape;
   change only the engine entries. Update `tests/gen_test.c` / `tests/dice_metrics.c` expectations that name them.
5. Tests: delete `tests/animal_test.c` and its line in `tests/run_tests.sh`; in `mod_test.c`, `midi_control_test.c`,
   `regress.c`, `cpu_vg.c`, `target_budget.py`, `ui_test.c`, `ui_render.c`, replace uses of the two engines (a
   WHEEL-based check becomes the same check on an engine that still has the parameter, or is removed if it only
   tested WHEEL; say which in the report).
6. Web: `web/editor.html` engine lists and pages for ANIMAL / WHEEL (the editor reads names from INFO; remove any
   hard-coded ANIMAL / WHEEL page), `web/test_web.mjs` expectations, `web/manual/data.js` (the engine sections for
   ANIMAL and WHEEL removed; add a line to the 0.1 news entry's successor: a new `0.2` entry "ANIMAL and WHEEL
   retired; 4 songs; the voice count measured" in the manual's existing bilingual format — the version itself is set
   in Task 6, write the entry now), regenerate `web/manual/nav.js` with `tools/gen_manual.py` after `./build.sh`.
7. Goldens: `GOLDEN_UPDATE=1 CC=tools/host-cc sh tests/run_tests.sh`, then check with `git diff tests/golden.txt`
   that the only changes are removed ANIMAL / WHEEL entries and the power-on renders that include T3. If anything else
   changed, stop and find out why. CPU baselines: `BUDGET_UPDATE=1` only if they moved because the engines left.

- [ ] **Step 3: Build, measure, test**

```bash
./build.sh 2>&1 | grep -E "^ *(ok|FAIL)|app |RAM code" ; CC=tools/host-cc sh tests/run_tests.sh 2>&1 | tail -6
grep -rnE "ENGI_ANIMAL|ENGI_WHEEL|eng_animal|eng_wheel|ENG_ANIMAL|ENG_WHEEL" firmware tests tools web | grep -v "core.h\|engines.c" 
```

Expected: build ok; ALL HOST TESTS PASSED; the grep finds only comments. Record the app size and RAM-code line
before (piece 1: app 568308 B) and after, in the report and the commit message.

- [ ] **Step 4: Commit**

```bash
git add -A firmware tests tools web
git commit -m "feat(engines): retire ANIMAL and WHEEL; their numbers stay reserved, stored sounds load as ANALOG

App <before> -> <after> B. T3 at power-on is PHASE. Goldens: <list>.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 2: Four songs and the reserved sample area

**Files:** `firmware/src/song_chain.c`, `firmware/src/storage.c`, `firmware/hal/fm1_flash.h`, any UI / editor /
backup code that counts songs (`grep -rn "NSONG" firmware web`), `tests/storage_test.c`, `tests/backup_test.c`,
`tests/project_test.c`, `web/editor.html`, `web/test_web.mjs`, `web/EDITOR_PROTOCOL.md`.

**Interfaces:**
- Produces: `NSONG 4u`; `NSONG_INDEX 8u` (rows in the stored index, unchanged layout); `FL_SMP_LO 0x000B8000u`,
  `FL_SMP_HI 0x000D8000u` in `fm1_flash.h`; songs 2..4 at 0xA0000..0xB7FFF exactly where JIANT kept them.

- [ ] **Step 1: Failing tests**

In `tests/storage_test.c` add:

```c
    /* (ACID FM 0.2) the sample area: no storage object's A or B sector lies in it, and it is where songs 5..8 were */
    {
        uint32_t o, bad_objs = 0;
        for (o = 0; o < OBJ_COUNT; o++) {
            uint32_t a = st_obj_off(o, 0), b = st_obj_off(o, 1);
            if ((a >= FL_SMP_LO && a < FL_SMP_HI) || (b >= FL_SMP_LO && b < FL_SMP_HI))
                bad_objs++;
        }
        bad += check("no storage object in the sample area 0xB8000..0xD7FFF", bad_objs == 0);
        bad += check("the sample area is 128 KiB, sector aligned",
                     FL_SMP_LO == 0xB8000u && FL_SMP_HI == 0xD8000u && (FL_SMP_LO % ST_SECTOR) == 0);
        bad += check("4 songs, the index keeps 8 rows", NSONG == 4u && NSONG_INDEX == 8u);
    }
```

`st_obj_off(obj, copy)` stands for storage.c's function that returns an object's sector offset for copy A (0) / B
(1). Find its real name (`grep -n "static uint32_t.*off\|OBJ_SONG1" firmware/src/storage.c`) and use it; if the
offsets are computed inline, add a small `static uint32_t st_obj_off(uint32_t obj, uint32_t copy)` in storage.c that
the existing code then uses (no behaviour change), and test through it. Match the file's existing `check` / `bad`
helpers (adapt the names to the file).

In `tests/project_test.c` (or the song test the suite has: `grep -ln "song_chain\|SONG_MAGIC" tests/*.c`): a stored
song index (version as written by JIANT) whose current song is 6 loads with the current song 1, rows 1..4 intact;
selecting song 5 is refused / clamps to 4 the way out-of-range values are handled in that code.

In `tests/backup_test.c`: restoring a complete backup that contains songs 5..8 restores songs 1..4 and skips 5..8
without writing flash outside the store (the test's flash stub records writes: assert none lands in
`FL_SMP_LO..FL_SMP_HI`).

Run the suite. Expected: these FAIL (no `FL_SMP_LO`, `NSONG` is 8).

- [ ] **Step 2: Implement**

1. `fm1_flash.h`, after `FL_VBANK_HI`:
   ```c
   /* (ACID FM 0.2) the sample area: songs 5..8's sectors (JIANT kept 8 songs, ACID FM 4), contiguous, for breakbeats
    * (piece 4 writes it; until then nothing does). Outside the app slot, so a firmware update never erases it. */
   #define FL_SMP_LO       0x000B8000u
   #define FL_SMP_HI       0x000D8000u
   ```
   Leave `FL_RANGE_OK` / `FL_STORE_OK` as they are: the app may not write the sample area yet (piece 4 adds it with
   its writer). Add `_Static_assert(FL_SMP_HI <= 0xD8000u, "the sample area ends below the song index");`.
2. `song_chain.c`: `#define NSONG 4u` (songs in use) and `#define NSONG_INDEX 8u` (rows in the stored index record).
   The index struct keeps `rows[NSONG_INDEX]` and `scene[NSONG_INDEX][...]`, so its size and version do not change.
   Every loop over songs, every song picker and every bound uses `NSONG`; a stored current song ≥ `NSONG` loads as
   song 1 (index 0). Comments say why (4 songs, the index layout kept).
3. `storage.c`: `OBJ_COUNT = OBJ_SONG1 + (NSONG - 1) * 4` (songs 2..4); the flash-map comment updated (songs 2..4 at
   0xA0000..0xB7FFF; 0xB8000..0xD7FFF the sample area). `_Static_assert` that the last song object's B sector ends at
   or below `FL_SMP_LO`. Keep `OBJ_FM6VB >= OBJ_COUNT`.
4. Backup / editor protocol: wherever songs are listed, archived or restored (`grep -rn "NSONG\|SONG_N\|song.*8" firmware/src/editor*.c web/editor.html web/fm1backup.js`), use 4; a backup holding songs 5..8 restores 1..4 and reports
   the rest as skipped (the existing "skipped" path, or a message in the editor's existing style). Document the
   change in `web/EDITOR_PROTOCOL.md` under a new heading `## ACID FM 0.2: four songs`.
5. UI: the song pages show songs 1..4.

- [ ] **Step 3: Build, test**

```bash
./build.sh 2>&1 | grep -E "FAIL|app " ; CC=tools/host-cc sh tests/run_tests.sh 2>&1 | tail -6
```

Expected: build ok, ALL HOST TESTS PASSED, goldens unchanged (songs do not change sound).

- [ ] **Step 4: Commit**

```bash
git add -A firmware tests web
git commit -m "feat(storage): 4 songs; songs 5..8's flash (0xB8000..0xD7FFF) reserved as the sample area

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 3: The flash map, measured

Finds out whether 0x93000..0x96FFF, 0xDA000..0xDBFFF and 0xE8000..0xFBFFF are free for ACID FM. Documentation and
evidence only: no firmware change.

**Files:** create `docs/acid-fm/flash-map.md`.

- [ ] **Step 1: Who writes each gap**

For each gap, collect evidence:
- Every address constant and write path in this tree (`grep -rnE "0x(000)?9[3-6][0-9A-F]{3}|0x(000)?D[AB][0-9A-F]{3}|0x(000)?(E[89A-F]|F[0-9A-B])[0-9A-F]{3}" firmware tools web`), `FL_RANGE_OK` / the loader's own window (`firmware/loader/`), and the JieLi flash table that `tools/fm1pkg_make.py:143` writes (the `VM` entry at 0x93000, 352256 B: what it tells the SPL / stock firmware).
- The piece 1 backup (`~/fm1-backups/2026-10-24-jiant/*/full-flash-before.bin`, readable): which sectors of each gap
  hold data, and what that data looks like (headers, magic numbers, strings: `python3 -I` with plain file reads and
  hexdumps; nothing executed from the file).
- Whether the official V15 firmware or JIANT's update loader writes there (the comments in `fm1_flash.h`,
  `storage.c`, `ota.c`, `firmware/loader/loader.c`).

- [ ] **Step 2: Write the map**

`docs/acid-fm/flash-map.md`: a table of the whole 1 MiB (start, end, size, owner, written by, in the backup: n of m
sectors used), then one section per gap with the evidence and a verdict: **free for ACID FM** (no code we ship writes
or reads it, and losing its content harms nothing we run), **not free** (name the user), or **unknown** (what would
settle it). End with the sample area's size: 128 KiB guaranteed plus each gap judged free.

- [ ] **Step 3: Commit**

```bash
git add docs/acid-fm/flash-map.md
git commit -m "docs: the FM-1 flash map under ACID FM, and which gaps are free

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

The controller brings any gap judged free to the user before piece 4 uses it (spec §6.1).

---

### Task 4: `NVOICE` as a build option, and the host budgets for 8, 10 and 12

**Files:** `firmware/src/core.h:12`, `tools/build.py:237-245`, `tests/run_tests.sh` (if it needs the flag passed to
host builds), create `docs/acid-fm/voices.md`.

**Interfaces:**
- Produces: `FELUCCA_NVOICE=N ./build.sh` builds with `NVOICE N`; `build/` packages named per build.

- [ ] **Step 1: The option**

`core.h`:

```c
#ifndef FELUCCA_NVOICE
#define FELUCCA_NVOICE 8         /* (ACID FM 0.2) the voice budget; tools/build.py FELUCCA_NVOICE; docs/acid-fm/voices.md */
#endif
#define NVOICE FELUCCA_NVOICE    /* voices per part, and the budget shared by all parts */
_Static_assert(NVOICE >= 1 && NVOICE <= 16, "FELUCCA_NVOICE: 1..16");
```

`tools/build.py`: add `"FELUCCA_NVOICE"` to the tuple of environment flags passed as `-D` (line ~237). If any stored
format, protocol table or array is sized by `NVOICE` (`grep -rn "NVOICE" firmware/src/*.h firmware/src/editor*.c
firmware/src/project.c web/EDITOR_PROTOCOL.md`), list it in the report: a stored size must not depend on the build
option (if one does, stop and report).

- [ ] **Step 2: Host builds and budgets**

For N in 8, 10, 12:

```bash
FELUCCA_NVOICE=$N ./build.sh 2>&1 | grep -E "FAIL|app |RAM code|pool"; cp build/felucca.fwsc build/acid-fm-nv$N.fwsc
```

and the host CPU budget at N (`tests/target_budget.py` / `tests/cpu_vg.py`: read how `run_tests.sh` calls them and
pass `-DFELUCCA_NVOICE=$N`; with valgrind missing, `cpu_vg` skips — say so). Record per N: app size, RAM / pool use,
RAM-code margin, the host instructions-per-sample of the CPU TEST song (as `tests/target_budget.txt` measures it).

Write `docs/acid-fm/voices.md`: the table above, then an empty table for the device measurements (N, CPU TEST mean
load %, PEAK, LATE, SHED, clicks heard yes/no), filled in Task 5.

- [ ] **Step 3: Test and commit**

`CC=tools/host-cc sh tests/run_tests.sh` at the default (8): ALL HOST TESTS PASSED, goldens unchanged.

```bash
git add firmware/src/core.h tools/build.py docs/acid-fm/voices.md tests/run_tests.sh
git commit -m "build: NVOICE is a build option (FELUCCA_NVOICE); host budgets for 8, 10 and 12 voices

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 5: USER STEPS — measure the voice ceiling on the FM-1 and set it

Run by the controller with the user. Each install is recoverable (piece 1's Task 6 proved the path; the ROM
bootloader and the backups remain).

- [ ] **Step 1: Fresh backup (USER STEP)**

As in piece 1: `fm1_unbrick.py softkey --port "ACID FM"`, then `sudo ... backup ~/fm1-backups/<date>-acid-fm-0.1`.
Record the sha256 and that both reads matched in `docs/acid-fm/device-log.md`. Power-cycle.

- [ ] **Step 2: For N = 8, then 10, then 12**

```bash
python3 tools/fm1_install.py build/acid-fm-nv$N.fwsc --yes && python3 tools/fm1_install.py --info
```

USER STEP on the device: SAVE › TOOLS › 6 CPU TEST, OCT+, PLAY, let it play 20 s, listen for clicks, STOP; then MENU ›
SYSTEM › ABOUT (OCT+): read CPU TEST %, AUDIO LOAD NOW / PEAK, LATE, SHED. Fill the row in `docs/acid-fm/voices.md`.
If N fails (LATE > 0, clicks, or mean ≥ 60 %), do not try a larger N.

- [ ] **Step 3: Set it**

The ceiling = the largest N that passed. Set `#define FELUCCA_NVOICE <ceiling>` in `core.h`. If it differs from 8:
`GOLDEN_UPDATE=1` and `BUDGET_UPDATE=1` runs, and the commit lists every changed golden (only renders with more than
8 simultaneous voices may change). Then `./build.sh` and the full suite: ALL HOST TESTS PASSED.

```bash
git add firmware/src/core.h tests/golden.txt tests/cpu_baseline*.txt tests/target_budget.txt docs/acid-fm/voices.md docs/acid-fm/device-log.md
git commit -m "feat(voices): NVOICE <ceiling>, measured on the FM-1 with the CPU TEST song

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 6: Version 0.2, docs, install and check

**Files:** `firmware/src/felucca.c` (`FELUCCA_VERSION "v0.2"`), `README.md` (status), `web/landing/index.html` (the
"Where it is" list), `web/manual/data.js` (the 0.2 entry from Task 1, finalised with the voice count),
`docs/acid-fm/device-log.md`, `docs/superpowers/plans/2026-10-24-acid-fm-piece2-deferred.txt` (new, if any items are
deferred).

- [ ] **Step 1: Version and docs**

From piece 1's backlog (`docs/superpowers/plans/2026-10-24-acid-fm-piece1-deferred.txt`, items tagged [piece 2]):
wrap the comments at `firmware/src/felucca.c:42` and `firmware/src/editor.c:412` to 120 columns.
`FELUCCA_VERSION "v0.2"`; tests that pin the version string follow it (Task 2 of piece 1 showed which: the nav maps,
the manual). README "Status" becomes 0.2 with one line per change (ANIMAL and WHEEL retired; 4 songs, 128 KiB
reserved for breaks; N voices); the landing page marks step 2 done (class `now` moves to step 3). Regenerate what
derives from the version (`tools/gen_nav_map.py --svg`, `tools/gen_manual.py`), as piece 1 did.

- [ ] **Step 2: Commit**

```bash
git add -A firmware/src/felucca.c README.md web docs tests
git commit -m "release prep: ACID FM 0.2 (trim): ANIMAL and WHEEL retired, 4 songs, the sample area, N voices

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 3: Release build, install, check (USER STEP to check)**

```bash
git status --short   # must be empty: Step 2's commit is in
./build.sh --release 0.2 && sha256sum build/release-0.2/acid-fm-0.2.fwsc && ./build.sh
python3 tools/fm1_install.py build/release-0.2/acid-fm-0.2.fwsc --yes && python3 tools/fm1_install.py --info
```

Expected: `done: the FM-1 runs FM-1_902`. USER STEP: ABOUT shows v0.2; PRESETS lists no ANIMAL or WHEEL; T3 at
power-on is PHASE; the song pages show 4 songs; a project saves and loads; the CPU TEST song plays clean. Log it in
`docs/acid-fm/device-log.md` (commit, sha256, identity, what the user confirmed).


Then log the install in `docs/acid-fm/device-log.md` and commit it (`docs: piece 2 device log`).

No tag and no GitHub release (user, 2026-10-24: the first tag is the first version that is genuinely ACID FM). The
user merges `piece2-trim` into `main` (fast-forward); pushing `main` updates the public installer, so push only after
the device check passed.
