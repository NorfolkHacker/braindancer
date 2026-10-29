# Braindancer rename (0.7) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rename the firmware from ACID FM to Braindancer everywhere a user sees it (device, USB name, files, docs, web
apps), keep every tool able to find a device still called ACID FM, and release it as 0.7.

**Architecture:** A string-level rename in three layers — firmware strings and USB descriptors; the tools and web code
that find, identify and package the firmware; the documents and web pages — guarded by tests that fail on any
user-facing "ACID FM" outside the recorded history. No behaviour, storage or protocol change.

**Tech Stack:** C (JieLi clang / host gcc via `tools/host-cc`), Python build tools, Node web tests.

**Spec:** `docs/superpowers/specs/2026-10-29-braindancer-rename.md`

## Global Constraints

- Spec §1: "A rename only: no feature changes, no storage or protocol changes, sound goldens unchanged."
- Spec §5: past news entries (0.1–0.6), past device-log entries, specs and plans already written, and code comments that
  record history keep "ACID FM". The manual's 0.7 news says "ACID FM is now Braindancer".
- Spelling: **Braindancer** in mixed-case text; **BRAINDANCER** where the face or style is upper case (device UI, USB).
- Naming rule kept: Braindancer is built on JIANT FM and Felucca; JIANT only in credits; GPL copyright lines unchanged;
  English only; no band names on the device (RETROBOC stays).
- The colour palette keeps its internal ACID token names (`UX_ACID` etc.); user-facing "the ACID look" becomes "the
  Braindancer look". Genre/sound names "ACID" (GEN genre, presets, recipes) are music words and stay.
- Every finder accepts **braindancer** AND keeps accepting acid fm / jiant / felucca.
- Image floor 16 KiB free (now 28,056 B); ISR budgets (300 / 2170 / 821) and the pin guard unchanged.
- Environment: `cd ~/synth/mvavefirmware && . .venv/bin/activate && export PATH=$PWD/.venv/bin:$PWD/build/node/bin:$PATH JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK`; `./build.sh`; `CC=tools/host-cc sh tests/run_tests.sh`.
- Process: foreground commands only; no `until/while pgrep` loops; branch `rename-braindancer`; commits end with a blank
  line + `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`; no push; no device access (Task 4 step 2 is the
  controller's with the user).

---

### Task 1: Device strings, USB names, splash

**Files:** `firmware/src/ux_splash.c` (`SPL_MARK` ~17), `ux_home.c` (~241, 258, 564), `ui_menu.c` (~280, 283),
`main.c` (~107 crash line), `editor.c` (~413 INFO version string), `usb.c` (`STR_PROD` both variants ~492-496; the
`STR1` comment), any other device string found by `git grep -n -I 'ACID FM' -- firmware/src` that is shown or sent
(not comments); tests (`tests/ui_test.c`, `tests/editor_test.c`, `tests/ui_render.c`, `tests/built_on_test.py`).

- [ ] Step 1 — failing tests:
  - `tests/built_on_test.py`: a check that no C string literal in `firmware/src/*.c` contains `ACID FM` (comments are
    not literals); that ABOUT's text contains "Braindancer" and "Built on JIANT FM and Felucca".
  - `tests/ui_test.c` `test_r1_name`: the splash draws `BRAINDANCER` and its ink fits inside x 8..231 (render the
    splash frame, find the leftmost / rightmost ink columns of the wordmark row); HOME's header word is `BRAINDANCER`
    and leaves room for the battery (the existing `lim` logic, ux_home.c ~241).
  - `tests/editor_test.c`: the INFO version string starts `BRAINDANCER v` (it was `ACID FM v`).
  - A USB descriptor test (where `STR_PROD` is tested, else a new check in `tests/ui_test.c` or `usb` host test):
    product string decodes to `BRAINDANCER`; loader build `BRAINDANCER UPD`; both ≤ the descriptor length limit.
  RED.
- [ ] Step 2 — implement: `SPL_MARK` = `"BRAINDANCER"` and the wordmark sized to fit (reuse the existing face; if it
  does not fit at the current size, use the next smaller wordmark size the build already has, or scale as the splash
  already scales its frames — no new font unless needed); HOME header word; ABOUT text ("Braindancer, IDM firmware for
  the MVAVE FM-1, by NorfolkHacker…", "Braindancer builds on open firmware for the FM-1: JIANT FM … Felucca …");
  crash line `BRAINDANCER CRASH`; editor INFO `"BRAINDANCER " FELUCCA_VERSION`; `STR_PROD` = UTF-16 `BRAINDANCER` (length
  byte 24) and the loader's `BRAINDANCER UPD` (length 32); regenerate the manual shots that change
  (`python3 tools/gen_manual.py build/ui_new/ACID`). GREEN; suite; commit `feat: the firmware is Braindancer on the device and over USB`.

### Task 2: Finding, identifying and packaging

**Files:** `web/fm1ota.js` (`PORT_RE` ~30), `web/fm1diag.js` (~37-55 version tests, ~218 report heading, `fwName` /
`kind` detection), `web/fm1backup.js` (only strings shown to the user), `web/fm1pkg.js` (if it names the product),
`tools/fm1_install.py` (port match and any name test), `tools/build.py` (package names ~11, ~506, ~537),
`.github/workflows/*` (artifact / file names), `web/test_web.mjs`, `tests/` checks of the package name.

- [ ] Step 1 — failing tests (`web/test_web.mjs`, and a Python test for `tools/fm1_install.py` / `tools/build.py` if
  one exists, else a small check in `tests/run_tests.sh`): `PORT_RE` matches "BRAINDANCER", "BRAINDANCER UPD", and
  still "ACID FM", "JIANT", "Felucca"; fm1diag recognises an INFO version "BRAINDANCER v0.7" as supported (same kind as
  ACID FM) and still "ACID FM v0.6"; the installer report heading is "Braindancer installer report";
  `fm1_install.py`'s port finder matches BRAINDANCER and ACID FM; a release build names `braindancer-X.Y.fwsc` and
  `braindancer-X.Y-app.bin`. RED.
- [ ] Step 2 — implement; keep every old pattern; workflow artifact names follow; GREEN (`node web/test_web.mjs`;
  suite); commit `feat: tools and installer find and package Braindancer (and still ACID FM)`.

### Task 3: Documents and web pages

**Files:** `README.md`, `BUILDING.md`, `LICENSING.md` (project-name mention only; licence text unchanged),
`web/landing/index.html`, `web/manual/*` (index.html title, manual.js, data.js current entries — not past news),
`web/editor.html` / installer / emulator page titles (`web/**/*.html`), `docs/MANUAL.md`, `tools/gen_cheatsheets.py` +
regenerated `docs/cheatsheet/*.html`, `.github/ISSUE_TEMPLATE/*`, `docs/acid-fm/` → `docs/braindancer/` (`git mv`) and
every link to it (README, device log references, tests, tools), `assets/ui-art` text if user-facing,
`tests/built_on_test.py`.

- [ ] Step 1 — failing test (`tests/built_on_test.py`): every user-facing file (README, BUILDING, landing, manual
  pages and data.js outside the `news` entries for 0.1–0.6, docs/MANUAL.md outside its history notes, cheat sheets,
  web page titles, issue templates) contains no "ACID FM" / "Acid FM" / "acid-fm" except: links/URLs that must still
  point at the forwarding repo, and explicitly allowed history lines; the credits line reads "Built on JIANT FM (Juan
  Jiant) and Felucca (Hügelton Instruments)" and the naming test now says Braindancer. RED.
- [ ] Step 2 — implement the wording; GitHub URLs in docs point at `github.com/NorfolkHacker/braindancer` and
  `norfolkhacker.github.io/braindancer/` (they go live when the controller renames the repo in Task 4); `git mv
  docs/acid-fm docs/braindancer` and fix references; regenerate cheat sheets and nav map / manual (`tools/gen_manual.py`,
  `tools/gen_nav_map.py`; PNGs only if a renderer is available); GREEN; suite; commit `docs: Braindancer`.

### Task 4: Version 0.7, device, GitHub

**Files:** `firmware/src/felucca.c` (`FELUCCA_VERSION "v0.7"`), `web/manual/data.js` (0.7 news, three items:
"ACID FM is now Braindancer: …", what changed on the device and over USB, the new website address), `web/manual/sw.js`
(cache bump), `README.md` Status 0.7, `docs/braindancer/device-log.md` (0.7 section, checks pending), version-pinned
generated files.

- [ ] Step 1 (implementer) — version + docs; suite; commit `release prep: Braindancer 0.7 (the rename)`; tree clean.
- [ ] Step 2 (controller + user) — backup (`softkey --port "ACID FM" --yes`, user runs the unbricker backup);
  `./build.sh --release 0.7`; install `tools/fm1_install.py build/braindancer-0.7.fwsc --port "ACID FM" --yes` (the
  device still says ACID FM until it restarts on 0.7); serial `status` shows `braindancer v0.7`; the user looks at the
  splash and ABOUT and checks the computer's MIDI port name (BRAINDANCER); log; merge ff to main; on the user's
  go-ahead: push, rename the GitHub repo to `braindancer` (`gh repo rename braindancer -R NorfolkHacker/acid-fm`),
  update the local remote, create `NorfolkHacker/acid-fm` with one Pages page forwarding to
  `https://norfolkhacker.github.io/braindancer/` (meta refresh keeping the path for webapp/installer, manual, emu), and
  update the memory notes.
