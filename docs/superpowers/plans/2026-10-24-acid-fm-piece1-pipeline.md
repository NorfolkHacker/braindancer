# ACID FM piece 1 (pipeline) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A JIANT build renamed ACID FM that builds natively on this Linux machine, passes the host tests, is published with
its own GitHub Pages site (installer, emulator, manual, acid landing page), and is proven to install on the user's FM-1
and roll back.

**Architecture:** No sound or UI behaviour changes. Only identity changes: the USB names, the version and INFO strings,
the ABOUT page and its QR code, the installers' port filters (so they find a device called ACID FM), the release file
names, the README / LICENSING, and the CI workflow. The package identity stays `FM-1_9XY`, because the FM-1's update
protocol and both installers check the model from it.

**Tech Stack:** C (JieLi clang 4.0.1 for pi32v2, host gcc for the tests), Python 3 (build tools, installer CLI), plain
JS / HTML (Web MIDI installer), GitHub Actions + Pages, Emscripten (CI only).

**Spec:** `docs/superpowers/specs/2026-10-24-acid-fm-design.md` (§3 is this piece; §2 and §9's install-safety rules
apply).

## Global Constraints

- Licence GPL-3.0-only. Never remove a JIANT or Felucca copyright line. A file we change gains
  `Portions Copyright (C) 2026 NorfolkHacker` under its existing copyright lines (C: inside the header comment; Python /
  shell / YAML: a `#` line; JS: a `//` line; HTML: inside the existing header comment, or a new `<!-- -->` one).
- Product name: `ACID FM`. Version string for this piece: `v0.1`. Release package: `acid-fm-0.1.fwsc`.
- Package identity stays `FM-1_9XY` (dev builds `FM-1_900`, `--release 0.1` gives `FM-1_901`).
- USB: manufacturer `NorfolkHacker`, product `ACID FM`, update loader `ACID FM Update`. VID/PID unchanged (1209:0001).
- Repo: `~/synth/mvavefirmware`, remotes `upstream` (JIANT) and, from Task 7, `origin`
  = `https://github.com/NorfolkHacker/acid-fm` (public). Site: `https://norfolkhacker.github.io/acid-fm/`.
- Nothing may write the ROM bootloader or the head of the flash. No install on the device before Task 6's full flash
  backup exists and is verified.
- Git: work on branch `piece1-pipeline` (from `spec/acid-fm-design`). Commit identity is already set in the repo.
  Every commit message ends with:
  ```
  Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
  ```
- Steps marked **USER STEP** need the user (sudo, physical buttons, or an outward-facing action). Stop and ask for them;
  do not work around them.

## File map

| File | Change |
|---|---|
| `requirements-dev.txt` | new: pinned Python packages for build, tests, installer, QR tool |
| `.gitignore` | add `.venv/` |
| `firmware/src/usb.c` | USB strings |
| `firmware/src/felucca.c` | `FELUCCA_VERSION` → `v0.1` |
| `firmware/src/editor.c` | INFO version string `ACID FM v0.1` |
| `firmware/src/main.c` | crash screen title |
| `firmware/src/ui_menu.c` | ABOUT page text, project links, QR table |
| `tools/gen_about_qr.py` | new: makes the ABOUT QR table |
| `tools/build.py` | release file names `acid-fm-X.Y*` |
| `tools/fm1_install.py`, `web/fm1ota.js`, `web/fm1diag.js`, `web/editor.html` | accept the ACID FM names |
| `tests/usb_desc_test.c`, `tests/editor_test.c`, `tests/install_test.py`, `web/test_web.mjs`, `tests/about_qr_test.py`, `tests/run_tests.sh` | tests for the above |
| `.github/workflows/emulator.yml` | our site; no Android, no JIANT classics |
| `.github/workflows/android.yml`, `android/` | removed |
| `.github/FUNDING.yml`, `.github/ISSUE_TEMPLATE/*` | JIANT's links removed / pointed at acid-fm |
| `web/STABLE`, `web/PREVIEW`, `web/CLASSIC` | reset |
| `web/landing/index.html` | new acid landing page; `frecuencia.html`, `anim/` removed |
| `README.md` (JIANT's moved to `docs/JIANT-README.md`), `LICENSING.md` | ACID FM's |
| `docs/acid-fm/baseline.md`, `docs/acid-fm/device-log.md` | new: measurements and the install / rollback record |

---

### Task 1: Toolchain, SDK, Python env and a green baseline

Proves the unmodified JIANT tree builds and passes its tests here before anything changes.

**Files:**
- Create: `requirements-dev.txt`
- Modify: `.gitignore`
- Create: `docs/acid-fm/baseline.md`

**Interfaces:**
- Produces: `.venv/` (activate with `. .venv/bin/activate`), `~/.jieli/toolchain`, `~/fw-AC79_AIoT_SDK`; env vars used
  by every later task: `JIELI_TOOLCHAIN=$HOME/.jieli/toolchain`, `AC79_SDK=$HOME/fw-AC79_AIoT_SDK`.

- [ ] **Step 1: Branch**

```bash
cd ~/synth/mvavefirmware && git switch spec/acid-fm-design && git switch -c piece1-pipeline
```

- [ ] **Step 2: Pinned Python packages**

Create `requirements-dev.txt`:

```
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
# Build (tools/build.py: fonts, icons, tables; the versions the CI uses), the installer CLI, the ABOUT QR tool
Pillow==12.3.0
numpy==2.3.5
fonttools==4.61.1
mido==1.3.3
python-rtmidi==1.5.8
segno==1.6.1
```

Append to `.gitignore`:

```
.venv/
```

Run:

```bash
python3 -m venv .venv && . .venv/bin/activate && pip install -r requirements-dev.txt
```

Expected: all six install without error.

- [ ] **Step 3: JieLi toolchain**

```bash
sh tools/get_toolchain.sh "$HOME/.jieli" && ls "$HOME/.jieli/toolchain/pi32v2/bin/clang"
```

Expected: the path prints. (The download comes from JieLi's package server; if it resets, run it again.)

- [ ] **Step 4: The three AC79 SDK files, at the reference commit (as the CI does)**

```bash
git clone --filter=blob:none --no-checkout https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git "$HOME/fw-AC79_AIoT_SDK"
cd "$HOME/fw-AC79_AIoT_SDK" && git sparse-checkout set --no-cone /cpu/wl82/tools/uboot.boot /cpu/wl82/tools/cfg_tool.bin /cpu/wl82/tools/cfg/eq_cfg_hw.bin \
  && git checkout d179b4484759423312073f5fbb232501aa491047 && cd -
sha256sum "$HOME/fw-AC79_AIoT_SDK/cpu/wl82/tools/uboot.boot"
```

Expected: `4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4`.

- [ ] **Step 5: Baseline build**

```bash
export JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK
./build.sh 2>&1 | tee build/baseline-build.log | tail -15
```

Expected: every check line `ok`, no `FAIL`, last lines `app build/felucca.bin N B`, `package build/felucca.fwsc N B,
identity FM-1_900`, and no `warning: SDK ... differs`.

- [ ] **Step 6: Baseline tests**

```bash
sh tests/run_tests.sh 2>&1 | tee build/baseline-tests.log | tail -25
```

Expected: the summary reports no failures. The web tests are skipped if `node` is missing, and the emulator tests if
`emcc` is missing. Both run in CI. If any host test fails on the untouched tree, stop and report it: it is a JIANT
problem or an environment problem, not ours to "fix" by editing tests.

- [ ] **Step 7: Record the baseline**

Create `docs/acid-fm/baseline.md` with the real numbers from Steps 5–6:

```markdown
# Baseline: JIANT main as forked

- Upstream commit: <output of `git rev-parse upstream/main`>
- Built: 2026-10-24, Linux x86-64, JieLi clang 4.0.1, AC79 SDK d179b44
- App (`build/felucca.bin`): <N> B of the 0x8EA90 (584336 B) XIP slot
- Package (`build/felucca.fwsc`): <N> B, identity FM-1_900
- RAM code: <the build's RAM-code line, e.g. "RAM code 22812 B of 25600">
- Host tests: <passed count> passed, 0 failed; skipped: <web / emulator if node / emcc missing>
```

Take the app, package and RAM-code lines from `build/baseline-build.log` (`grep -E "app |package |RAM code"`).

- [ ] **Step 8: Commit**

```bash
git add requirements-dev.txt .gitignore docs/acid-fm/baseline.md
git commit -m "build: pinned Python env; baseline of JIANT main as forked

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 2: The firmware says ACID FM

USB names, version, INFO string, crash title. TDD against the existing descriptor and editor tests.

**Files:**
- Modify: `tests/usb_desc_test.c:129-139`
- Modify: `tests/editor_test.c` (INFO check, near line 165)
- Modify: `firmware/src/usb.c:486-493`
- Modify: `firmware/src/felucca.c:40-42`
- Modify: `firmware/src/editor.c:411`
- Modify: `firmware/src/main.c:106`

**Interfaces:**
- Produces: INFO version string `"ACID FM v0.1"` (the web tools key on its `ACID FM ` prefix in Task 4); USB product
  `ACID FM`, loader `ACID FM Update` (the installers' port filters in Task 4).

- [ ] **Step 1: Failing USB-string test**

In `tests/usb_desc_test.c`, replace the block that starts `{   /* (0.9.12) the names a host reads` with:

```c
    {   /* (ACID FM 0.1) the names a host reads: manufacturer "NorfolkHacker", product "ACID FM" (UTF-16LE), nothing at
         * index 2 (Windows keeps older names under index 2's name GUID), no serial */
        static const uint8_t PROD[16] = {16, 3, 'A', 0, 'C', 0, 'I', 0, 'D', 0, ' ', 0, 'F', 0, 'M', 0};
        static const uint8_t MANU[28] = {28, 3, 'N', 0, 'o', 0, 'r', 0, 'f', 0, 'o', 0, 'l', 0, 'k', 0, 'H', 0, 'a', 0,
                                         'c', 0, 'k', 0, 'e', 0, 'r', 0};
        const uint8_t *s;
        uint16_t sl;
        check("string 0: US English", get_desc(0x0300, &s, &sl) && sl == 4 && s[2] == 0x09 && s[3] == 0x04);
        check("string 1 (manufacturer): \"NorfolkHacker\"", get_desc(0x0301, &s, &sl) && sl == 28 && !memcmp(s, MANU, 28));
        check("string 4 (product): \"ACID FM\"", get_desc(0x0304, &s, &sl) && sl == 16 && !memcmp(s, PROD, 16));
        check("strings 2 (the old product) and 3: none", !get_desc(0x0302, &s, &sl) && !get_desc(0x0303, &s, &sl));
        check("no serial number (iSerial 0)", dev[16] == 0);
    }
```

Add `Portions Copyright (C) 2026 NorfolkHacker` to the file's header comment.

- [ ] **Step 2: Failing INFO-version test**

In `tests/editor_test.c`, directly after the `bad += check("INFO explicitly tags display capabilities ...` statement
(ends `LYR_LEN - 1u);`), add:

```c
    bad += check("INFO names the firmware \"ACID FM v...\" (the web tools key on the prefix)",
        !memcmp(&host_wire[5], "ACID FM v", 9));
```

Before trusting offset 5, confirm it: INFO's reply carries the version string from `ed_str(...)` in
`firmware/src/editor.c:411` as its first field. Find the reply header length the other INFO checks in this file use
(`grep -n "host_wire\[5\]" tests/editor_test.c` shows offset 5 is the first payload byte). If INFO's first payload
byte is not the string, use the offset at which `ed_str` writes it. Add the copyright line to the header.

- [ ] **Step 3: Run, expect failure**

```bash
sh tests/run_tests.sh 2>&1 | grep -E "FAIL|string 1|string 4|ACID FM" | head
```

Expected: FAIL on `string 1 (manufacturer)`, `string 4 (product)` and `INFO names the firmware`.

- [ ] **Step 4: Implement**

`firmware/src/usb.c`: replace `STR1` and both `STR_PROD` definitions and their comment with:

```c
static const uint8_t STR1[] = {28, 3, 'N', 0, 'o', 0, 'r', 0, 'f', 0, 'o', 0, 'l', 0, 'k', 0, 'H', 0, 'a', 0, 'c', 0,
                               'k', 0, 'e', 0, 'r', 0};   /* (ACID FM 0.1) the installers match /acid ?fm|jiant|felucca/ */
/* the product (USB_STR_PRODUCT): Windows names the MIDI port after it, macOS the device and its port; no interface
 * or jack has a name of its own (iInterface / iJack 0). ACID FM 0.1: "ACID FM"; JIANT 0.7.0..0.9.11: "JIANT FM",
 * 0.9.12..: "JIANT" (index 2 before 0.9.12); Felucca: "Felucca" */
#ifdef FELUCCA_LOADER
static const uint8_t STR_PROD[] = {30, 3, 'A', 0, 'C', 0, 'I', 0, 'D', 0, ' ', 0, 'F', 0, 'M', 0, ' ', 0, 'U', 0, 'p', 0,
                                   'd', 0, 'a', 0, 't', 0, 'e', 0};
#else
static const uint8_t STR_PROD[] = {16, 3, 'A', 0, 'C', 0, 'I', 0, 'D', 0, ' ', 0, 'F', 0, 'M', 0};
#endif
```

`firmware/src/felucca.c`:

```c
#ifndef FELUCCA_VERSION
#define FELUCCA_VERSION "v0.1" /* ACID FM's (a fork of JIANT 0.9.18, on Felucca 1.1.5.1): the splash, the menu, the console, the editor */
```

`firmware/src/editor.c:411`:

```c
        ed_str("ACID FM " FELUCCA_VERSION, 24);   /* (ACID FM 0.1; JIANT: "JIANT v..", Felucca "FELUCCA v..": the web tools take all three) */
```

`firmware/src/main.c:106`: `"JIANT CRASH"` → `"ACID FM CRASH"`.

Add the copyright line to each file's header.

- [ ] **Step 5: Find any other on-screen JIANT name**

```bash
grep -rnE "\"[^\"]*JIANT[^\"]*\"" firmware/src firmware/hal firmware/loader | grep -v "^\S*:\s*[0-9]*:\s*/\?\*"
```

Expected remaining hits: only `ui_menu.c` (Task 3 handles it) and `UI_JIANT_INDEX` / palette names (identifiers, not
shown text; piece 3 replaces the skins). Any other user-visible string that names JIANT as *this* firmware becomes
`ACID FM`. Strings that credit JIANT (credits, licences) stay.

- [ ] **Step 6: Build and test**

```bash
./build.sh 2>&1 | tail -4 && sh tests/run_tests.sh 2>&1 | tail -15
```

Expected: build ok; the three new checks pass; no other failure. If a golden render hash changes, stop: nothing here
may change the sound or the screens except the strings named above. A UI render that shows the version string may
legitimately change; check `build/ui_new/` PNGs show `v0.1` / `ACID FM` and only then accept it.

- [ ] **Step 7: Commit**

```bash
git add firmware/src/usb.c firmware/src/felucca.c firmware/src/editor.c firmware/src/main.c tests/usb_desc_test.c tests/editor_test.c
git commit -m "feat(identity): the firmware is ACID FM v0.1 over USB, in INFO and on the crash screen

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 3: ABOUT page and its QR code

**Files:**
- Create: `tools/gen_about_qr.py`
- Create: `tests/about_qr_test.py`
- Modify: `tests/run_tests.sh` (register the test next to `install_test.py`, line ~389)
- Modify: `firmware/src/ui_menu.c:10-19, 231-251, 289-293`

**Interfaces:**
- Produces: `python3 tools/gen_about_qr.py URL` prints the C table; `ABOUT_URL = "https://github.com/NorfolkHacker/acid-fm"`.

- [ ] **Step 1: Failing test**

Create `tests/about_qr_test.py`:

```python
#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""The ABOUT page's QR table (firmware/src/ui_menu.c ABOUT_QR) is the one tools/gen_about_qr.py makes for ABOUT_URL."""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ABOUT_URL = "https://github.com/NorfolkHacker/acid-fm"

src = (ROOT / "firmware/src/ui_menu.c").read_text()
m = re.search(r"static const uint32_t ABOUT_QR\[29\] = \{(.*?)\};", src, re.S)
if not m:
    sys.exit("FAIL: no ABOUT_QR[29] in ui_menu.c")
have = [int(w, 16) for w in re.findall(r"0x([0-9A-Fa-f]{8})u", m[1])]
out = subprocess.run([sys.executable, str(ROOT / "tools/gen_about_qr.py"), ABOUT_URL],
                     check=True, capture_output=True, text=True).stdout
want = [int(w, 16) for w in re.findall(r"0x([0-9A-Fa-f]{8})u", out)]
if len(want) != 29:
    sys.exit(f"FAIL: gen_about_qr.py made {len(want)} rows, not 29")
if have != want:
    sys.exit("FAIL: ABOUT_QR does not encode " + ABOUT_URL)
if ABOUT_URL not in src:
    sys.exit("FAIL: ui_menu.c does not name " + ABOUT_URL)
print("ok   ABOUT QR encodes", ABOUT_URL)
```

Register it in `tests/run_tests.sh`, on the line after the `installer CLI (fm1_install.py)` run:

```sh
run "ABOUT page QR code (gen_about_qr.py)" python3 tests/about_qr_test.py
```

- [ ] **Step 2: Run, expect failure**

```bash
python3 tests/about_qr_test.py
```

Expected: fails, because `tools/gen_about_qr.py` does not exist yet.

- [ ] **Step 3: The generator**

Create `tools/gen_about_qr.py`:

```python
#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""Print the ABOUT page's QR table (firmware/src/ui_menu.c ABOUT_QR) for a URL: QR version 3 (29 x 29), correction M,
no quiet zone (draw_about_qr draws its own), one row per word, the leftmost module in bit 0.

  python3 tools/gen_about_qr.py https://github.com/NorfolkHacker/acid-fm
"""
import sys

import segno


def rows(url):
    qr = segno.make_qr(url, version=3, error="m", boost_error=False)
    out = []
    for line in qr.matrix:
        word = 0
        for col, dark in enumerate(line):
            if dark:
                word |= 1 << col
        out.append(word)
    assert len(out) == 29 and all(len(line) == 29 for line in qr.matrix)
    return out


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    words = [f"0x{w:08X}u" for w in rows(sys.argv[1])]
    print(f"/* {sys.argv[1]}, QR version 3, correction M, generated by tools/gen_about_qr.py (segno).")
    print(" * One row per word, leftmost module in bit 0; the constant matrix needs no encoder or RAM. */")
    print("static const uint32_t ABOUT_QR[29] = {")
    for i in range(0, 29, 6):
        print("    " + ", ".join(words[i:i + 6]) + ",")
    print("};")


if __name__ == "__main__":
    main()
```

Sanity check that segno's matrix for the *old* URL reproduces JIANT's table (proves bit order and options match):

```bash
python3 tools/gen_about_qr.py https://github.com/juanjiant-bit/JIANT | head -4
```

Expected: the first words are `0x1FC4C67Fu, 0x10536541u, 0x1744955Du`, as in the current `ui_menu.c`. If they differ,
segno chose another mask: add `mask=` with the value that reproduces JIANT's table (try 0–7) and keep it.

- [ ] **Step 4: New table and ABOUT text**

Replace the comment and `ABOUT_QR` table at the top of `firmware/src/ui_menu.c` with the output of:

```bash
python3 tools/gen_about_qr.py https://github.com/NorfolkHacker/acid-fm
```

In `menu_document()`, replace the `if (draw) { ... }` block and the "THE PROJECT" lines down to and including
`y = menu_text("Free software, GPL-3.0-only: ...", y + 4, T_TEXT, draw);` with:

```c
    if (draw) {
        cv_text(6, y + 2, &AF_L, "ACID FM", T_THEME);
        cv_text(6, y + 38, &AF_S, "IDM firmware for the MVAVE FM-1", T_TEXT);
        cv_text(6, y + 58, &AF_M, FELUCCA_VERSION, T_THEME);
        cv_text_r(232, y + 61, &AF_S, __DATE__, T_MID, T_BG);
        cv_text(6, y + 82, &AF_S, "by NorfolkHacker", T_TEXT);
        cv_text(6, y + 100, &AF_S, "Free, GPL-3.0-only, no warranty", T_TEXT);
        cv_text(6, y + 118, &AF_S, "A fork of JIANT FM", T_TEXT);
        cv_text(6, y + 138, &AF_S, "github.com", T_MID);
        cv_text(6, y + 156, &AF_S, "/NorfolkHacker", T_MID);
        cv_text(6, y + 174, &AF_S, "/acid-fm", T_MID);
        draw_about_qr(158, y + 122);
    }
    y = menu_text("THE PROJECT", y + 216, T_THEME, draw);
    y = menu_text("Copyright (C) 2026 NorfolkHacker", y, T_TEXT, draw);
    y = menu_text("ACID FM is a fork of JIANT FM by Juan Jiant (Copyright (C) 2026 Juan Ignacio Antonucci), "
                  "itself a fork of Felucca by Hugelton Instruments.", y, T_TEXT, draw);
    y = menu_text("Installer, emulator, manual:", y + 4, T_TEXT, draw);
    y = menu_text("norfolkhacker.github.io/acid-fm", y, T_MID, draw);
    y = menu_text("JIANT FM: juanjiant-bit.github.io/JIANT", y + 4, T_MID, draw);
    y = menu_text("Free software, GPL-3.0-only: the full notices and the sources it builds on travel with the source.", y + 4, T_TEXT, draw);
```

At the end of `menu_document()`, replace `y = menu_text("github.com/juanjiant-bit/JIANT", y, T_MID, draw);` with:

```c
    y = menu_text("github.com/NorfolkHacker/acid-fm", y, T_MID, draw);
```

The `MENU_CREDITS` table stays as it is (it credits Felucca, JIANT and the rest). Add the copyright line to the file
header.

- [ ] **Step 5: Run**

```bash
python3 tests/about_qr_test.py && ./build.sh 2>&1 | tail -3 && sh tests/run_tests.sh 2>&1 | tail -15
```

Expected: `ok   ABOUT QR encodes https://github.com/NorfolkHacker/acid-fm`; build ok; all tests pass, including JIANT's UI
check that no text is clipped or overlapping (if "A fork of JIANT FM" or another line clips, shorten that line, not the
check). Look at the ABOUT render in `build/ui_new/` and scan the QR code on screen with a phone: it must open the repo URL.

- [ ] **Step 6: Commit**

```bash
git add tools/gen_about_qr.py tests/about_qr_test.py tests/run_tests.sh firmware/src/ui_menu.c
git commit -m "feat(identity): ABOUT page and QR code for ACID FM, crediting JIANT and Felucca

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 4: The installers and the editor find a device named ACID FM

Without this, the web installer, the CLI installer and the web editor ignore a renamed FM-1 (they only probe ports
whose names match their filter) and do not recognise its INFO.

**Files:**
- Modify: `tests/install_test.py:210-222`
- Modify: `web/test_web.mjs:1646-1647, 1672-1676, 1719-1726`
- Modify: `tools/fm1_install.py:39`
- Modify: `web/fm1ota.js:29`
- Modify: `web/fm1diag.js:39, 53`
- Modify: `web/editor.html` (its two `/(felucca|jiant)/i.test(p.name` port filters)

**Interfaces:**
- Consumes: USB product `ACID FM`, loader `ACID FM Update`, INFO `ACID FM v0.1` (Task 2).
- Produces: port filter `acid ?fm` in every tool, INFO prefix `ACID FM ` accepted as a JIANT-family firmware (INFO still
  carries JIANT's tag 0x4A, so `fwKind` reports kind `jiant`; the pages' wording is piece 3's).

- [ ] **Step 1: Failing CLI test**

In `tests/install_test.py`, after the `for name in ("JIANT", "JIANT FM", ...)` loop, add:

```python
    # (ACID FM 0.1) "ACID FM" / "ACID FM Update", as Linux, macOS and Windows show them
    for name in ("ACID FM", "2- ACID FM", "MIDIIN2 (ACID FM)", "ACID FM MIDI 1"):
        dev = FakeFM1(image, identity="FM-1_900", name=name, loader_name="ACID FM Update", after_name="ACID FM")
        rc, out, err = cli([p, "--yes"], dev)
        ok(rc == 0 and dev.bad == 0 and dev.upgrades == 2 and "done: the FM-1 runs FM-1_900" in out,
           f"install: a port named {name!r} -> 'ACID FM Update' -> 'ACID FM'")
    dev = FakeFM1(image, identity="ota-FM-1_900", name="ACID FM Update", after_name="ACID FM")
    rc, out, err = cli(["--info"], dev)
    ok(rc == 0 and "update loader" in out, "--info: 'ACID FM Update' (ota-FM-1_900) is the update loader")
    dev = FakeFM1(image, identity="FM-1_900", name="Acidlab Bassline")
    rc, out, err = cli(["--info"], dev)
    ok(rc != 0, "--info: other gear with 'acid' in its name is never probed")
```

(If `FakeFM1` or `cli` behave differently for an unmatched port, check how the existing tests in this file assert
"not found" (`grep -n "notfound\|rc == 1\|rc != 0" tests/install_test.py`) and assert the same way.)

- [ ] **Step 2: Failing web tests**

In `web/test_web.mjs`, add a profile after `jiantFm:` (line ~1647):

```js
  acidFm: { identity: "FM-1_900", name: "ACID FM", info: infoOf("ACID FM v0.1", 6, { jiant: true }), objs: JIANT_OBJS, after: "FM-1_015" },   /* (ACID FM 0.1) */
```

After the `x = await run("jiantFm"); ok(...)` pair, add:

```js
  x = await run("acidFm");
  ok(x.r === "ok" && x.c.state.fw.kind === "jiant" && x.c.state.fw.version === "ACID FM v0.1", "installer: Install over ACID FM (USB name 'ACID FM'): written");
```

In the port-name block, change `NAMES` and the editor-filter regex, and its message:

```js
  const NAMES = ["JIANT", "2- JIANT", "MIDIIN2 (JIANT)", "MIDIOUT2 (JIANT)", "JIANT MIDI 1", "JIANT 1", "JIANT FM", "JIANT Update",
    "Felucca", "Felucca Update", "2- Felucca", "ACID FM", "2- ACID FM", "MIDIIN2 (ACID FM)", "ACID FM MIDI 1", "ACID FM Update"];
  const edSrc = readFileSync(join(HERE, "editor.html"), "utf8");
  const edRes = [...edSrc.matchAll(/\/(felucca\|jiant\|acid \?fm)\/i\.test\(p\.name/g)].map((m) => new RegExp(m[1], "i"));
  ok(NAMES.every((n) => PORT_RE.test(n)) && !["IAC Driver Bus 1", "Microsoft GS Wavetable Synth", "Arturia KeyStep", "Acidlab Bassline"].some((n) => PORT_RE.test(n)),
    "installer: PORT_RE takes JIANT, JIANT FM, Felucca, ACID FM and their update names (not other gear)");
```

Leave the following `ok(edRes.length === 2 && ...)` line as it is.

- [ ] **Step 3: Run, expect failure**

```bash
python3 tests/install_test.py 2>&1 | grep -E "FAIL|ACID" | head
command -v node >/dev/null && node web/test_web.mjs 2>&1 | grep -E "FAIL|ACID" | head
```

Expected: the ACID FM cases fail (the ports are not probed). Without `node`, the web half is checked in CI (Task 7);
say so in the task report.

- [ ] **Step 4: Implement**

`tools/fm1_install.py:39`:

```python
PORT_RE = re.compile(r"fm-1|felucca|jiant|acid ?fm|ota|composite|sinco|usb-midi", re.I)   # never probe other gear
```

`web/fm1ota.js:29`:

```js
export const PORT_RE = /fm-?1|felucca|jiant|acid ?fm|skiff|ota|composite|sinco|usb-midi|m-?vave|cuvave/i;
```

`web/fm1diag.js`, lines 39 and 53: `/^(FELUCCA|JIANT) /i` → `/^(FELUCCA|JIANT|ACID FM) /i` (both places; line 44's
legacy v0.x check stays as it is). In the comment above `isJiantInfo`, add: `(ACID FM 0.1) ACID FM's INFO: the same
tag, the version "ACID FM v.."`.

`web/editor.html`: both `/(felucca|jiant)/i.test(p.name` → `/(felucca|jiant|acid ?fm)/i.test(p.name`.

Add the copyright line to each file.

- [ ] **Step 5: Run**

```bash
python3 tests/install_test.py 2>&1 | tail -3
command -v node >/dev/null && node web/test_web.mjs 2>&1 | tail -3
sh tests/run_tests.sh 2>&1 | tail -10
```

Expected: all pass.

- [ ] **Step 6: Commit**

```bash
git add tools/fm1_install.py web/fm1ota.js web/fm1diag.js web/editor.html tests/install_test.py web/test_web.mjs
git commit -m "feat(install): the installers and the editor find and recognise ACID FM

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 5: Release names, README, LICENSING, GitHub config and the Pages site

**Files:**
- Modify: `tools/build.py:428-470` (release names)
- Move: `README.md` → `docs/JIANT-README.md`; Create: `README.md`
- Modify: `LICENSING.md` (top)
- Delete: `.github/workflows/android.yml`, `android/`, `web/landing/frecuencia.html`, `web/landing/anim/`
- Modify: `.github/workflows/emulator.yml`, `.github/FUNDING.yml`, `.github/ISSUE_TEMPLATE/bug_report.yml`, `.github/ISSUE_TEMPLATE/config.yml`, `.gitignore`
- Modify: `web/STABLE`, `web/PREVIEW`, `web/CLASSIC`
- Create: `web/landing/index.html` (replaces JIANT's)

**Interfaces:**
- Produces: `./build.sh --release 0.1` → `build/acid-fm-0.1.fwsc`, `build/release-0.1/acid-fm-0.1.fwsc`,
  `build/release-0.1/acid-fm-0.1-app.bin`; the site layout `/` (landing), `/webapp/installer/`, `/webapp/editor/`,
  `/emu/`, `/manual/`.

- [ ] **Step 1: Release names**

In `tools/build.py` `main()`: `name = f"felucca-{a.release}.fwsc"` → `name = f"acid-fm-{a.release}.fwsc"`, and
`app = f"felucca-{a.release}-app.bin"` → `app = f"acid-fm-{a.release}-app.bin"`. Leave the dev names
(`felucca.bin`, `felucca.fwsc`) alone: the tests, the emulator build and the workflow read them. Update the
`--release` help and BUILDING.md's release paragraph to the new names. Add the copyright line.

Run:

```bash
./build.sh --release 0.1 2>&1 | tail -4 && ls build/release-0.1/
```

Expected: `package build/acid-fm-0.1.fwsc ... identity FM-1_901`; the release dir holds `acid-fm-0.1.fwsc`,
`acid-fm-0.1-app.bin`, `SHA256SUMS`, `LICENSE`, `LICENSING.md`, `LICENSES/`. Then `./build.sh` (dev build again, so
`build/felucca.fwsc` is current for the tests).

- [ ] **Step 2: README and LICENSING**

```bash
git mv README.md docs/JIANT-README.md
```

Create `README.md`:

```markdown
# ACID FM

**IDM / braindance firmware for the M-VAVE FM-1**: virtual analog, FM, chopped breakbeats you load yourself, and a
generator that writes whole songs on the device. Acid-coloured, in the spirit of Aphex Twin, Boards of Canada and
the Rephlex catalogue.

**[Install](https://norfolkhacker.github.io/acid-fm/webapp/installer/)** (Chrome or Edge, over USB) ·
**[Try it in the browser](https://norfolkhacker.github.io/acid-fm/emu/)** ·
**[Manual](https://norfolkhacker.github.io/acid-fm/manual/)**

## Status: 0.1

0.1 is JIANT FM 0.9.18 under a new name: the pipeline (build, tests, site, install, rollback) proven end to end.
The ACID FM features arrive in steps (see `docs/superpowers/specs/2026-10-24-acid-fm-design.md`):

1. Pipeline (0.1)
2. Trim: engines for IDM, the voice ceiling measured, room made for samples
3. The ACID screen theme
4. BREAK: load breakbeats from the web page, chop and rearrange them
5. Sound: TAPE (wow, flutter, saturation), 303-style accent and slide, new FM and drum voices
6. The song generator: ACID, BRAINDANCE, DRILL'N'BASS, BOC, AMBIENT, GLITCH

Until step 6 the manual and the on-device pages are JIANT's.

## Built on

ACID FM is a fork of **[JIANT FM](https://github.com/juanjiant-bit/JIANT)** by Juan Jiant, itself a fork of
**[Felucca](https://github.com/hugelton/Felucca)** by Hügelton Instruments, with SLOOP's song system. JIANT's own
README is kept in [docs/JIANT-README.md](docs/JIANT-README.md). Every third-party part and its licence is listed in
[LICENSING.md](LICENSING.md).

## Build

See [BUILDING.md](BUILDING.md). On Linux x86-64:

    python3 -m venv .venv && . .venv/bin/activate && pip install -r requirements-dev.txt
    export JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK
    ./build.sh && sh tests/run_tests.sh

## Safety

Installing firmware is at your own risk. Back up the full flash first
([MvaveFM1Unbricker](https://github.com/Quixotic7/MvaveFM1Unbricker) `backup`). The FM-1's ROM bootloader cannot be
overwritten, so a failed install can be recovered
([FM-1 Firmware Hub rescue](https://thefirmwarehub.com/#rescue)), and the official firmware can always go back on.

## Licence

GPL-3.0-only, as JIANT and Felucca. See [LICENSE](LICENSE) and [LICENSING.md](LICENSING.md).
```

At the top of `LICENSING.md`, after its first heading paragraph, insert:

```markdown
## ACID FM

ACID FM is a fork of JIANT FM (below), by NorfolkHacker: "Copyright (C) 2026 NorfolkHacker" where a file is mostly
ACID FM's, "Portions Copyright (C) 2026 NorfolkHacker" where ACID FM changed it; every earlier copyright line stays.
The licence is the same: GPL-3.0-only. ACID FM's own files so far: `tools/gen_about_qr.py`, `tests/about_qr_test.py`,
`requirements-dev.txt`, `web/landing/index.html`, `docs/acid-fm/`, `docs/superpowers/`. The ABOUT QR code is made with
segno (BSD-3-Clause) at build time; segno is not part of the firmware.
```

- [ ] **Step 3: Remove Android and JIANT's landing extras**

```bash
git rm -r -q .github/workflows/android.yml android web/landing/frecuencia.html web/landing/anim
```

Remove the three `android/probe/...` lines from `.gitignore`.

- [ ] **Step 4: GitHub config**

`.github/FUNDING.yml`: delete the file (`git rm .github/FUNDING.yml`): JIANT's funding links do not belong to this repo.
`.github/ISSUE_TEMPLATE/bug_report.yml` and `config.yml`: replace every `juanjiant-bit/JIANT` URL with
`NorfolkHacker/acid-fm`, every `juanjiant-bit.github.io/JIANT` with `norfolkhacker.github.io/acid-fm`, and "JIANT" as
the firmware's name with "ACID FM" (keep any line that credits JIANT). Check with
`grep -rn "juanjiant\|JIANT" .github` — expected: no hits except credits.

- [ ] **Step 5: Reset the release pointers**

`web/STABLE`, `web/PREVIEW`: empty files (`: > web/STABLE; : > web/PREVIEW`). With STABLE empty the public installer
builds `main`; Task 8 pins it to the commit proven on the device.
`web/CLASSIC`: keep its comment header, delete every commit line (those are JIANT's releases; offering them from our
installer would put JIANT on a device that asked for ACID FM). Add the line
`# ACID FM: no earlier versions yet.`

- [ ] **Step 6: The workflow**

In `.github/workflows/emulator.yml`:

1. Header comment: replace the first comment line's description with
   `# ACID FM's site on GitHub Pages: the landing page (web/landing) at the root, the installer, the editor, the browser emulator under emu/ and the manual.`
   and add `# Portions Copyright (C) 2026 NorfolkHacker` after the SPDX line.
2. Delete the three Android steps (`actions/setup-java@v4`, `gradle/actions/setup-gradle@v4`, `the Android probe`) and
   the comment above them.
3. In the `the front page and the emulator under emu/` step, delete
   `cp web/landing/frecuencia.html build/pages/frecuencia.html` and the two lines that copy `web/landing/anim` (its
   comment and the `cp -r`).

Nothing else in the workflow changes: it already builds the firmware, the emulator, the installer and the manual.

- [ ] **Step 7: The landing page**

Replace `web/landing/index.html` with:

```html
<!doctype html>
<!-- SPDX-License-Identifier: GPL-3.0-only
     Copyright (C) 2026 NorfolkHacker
     ACID FM's front page (GitHub Pages root): what it is, install / try / read, credits. -->
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ACID FM</title>
<meta name="description" content="ACID FM: IDM / braindance firmware for the M-VAVE FM-1">
<style>
  :root {
    --bg: #1A1C20; --bg-dark: #141518; --line: #303338; --text: #E8EAED; --mid: #9AA0A6;
    --acid: #B8F53A; --acid-light: #DCFA9D; --violet: #A64BFF; --violet-light: #D3A5FF;
    --accent: #FF503C; --slide: #4CB8FF; --queued: #FFC83C;
  }
  * { box-sizing: border-box; }
  body { margin: 0; background: var(--bg-dark); color: var(--text);
         font: 16px/1.55 ui-monospace, "SFMono-Regular", Menlo, Consolas, monospace; }
  main { max-width: 760px; margin: 0 auto; padding: 48px 16px 64px; }
  h1 { font-size: clamp(48px, 12vw, 96px); line-height: 1; margin: 0 0 8px; letter-spacing: -0.04em;
       background: linear-gradient(90deg, var(--acid), var(--slide), var(--violet), var(--accent), var(--queued), var(--acid));
       background-size: 300% 100%; -webkit-background-clip: text; background-clip: text; color: transparent;
       animation: cycle 8s linear infinite; }
  @keyframes cycle { to { background-position: 300% 0; } }
  @media (prefers-reduced-motion: reduce) { h1 { animation: none; } }
  .tag { color: var(--acid-light); margin: 0 0 32px; }
  .cards { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 12px; margin: 0 0 40px; }
  .card { display: block; padding: 18px; background: var(--bg); border: 1px solid var(--line); border-radius: 10px;
          color: var(--text); text-decoration: none; }
  .card:hover, .card:focus-visible { border-color: var(--acid); outline: none; }
  .card b { display: block; font-size: 20px; color: var(--acid); }
  .card:nth-child(2) b { color: var(--slide); }
  .card:nth-child(3) b { color: var(--violet-light); }
  .card span { color: var(--mid); font-size: 14px; }
  h2 { color: var(--acid); font-size: 18px; margin: 32px 0 8px; }
  ol, ul { padding-left: 22px; }
  li { margin: 4px 0; }
  .now { color: var(--queued); }
  .warn { border-left: 3px solid var(--accent); padding: 8px 14px; background: var(--bg); }
  a { color: var(--slide); }
  footer { margin-top: 48px; color: var(--mid); font-size: 13px; border-top: 1px solid var(--line); padding-top: 16px; }
</style>
</head>
<body>
<main>
  <h1>ACID FM</h1>
  <p class="tag">IDM / braindance firmware for the M-VAVE FM-1: virtual analog, FM, chopped breaks, songs that write themselves.</p>

  <div class="cards">
    <a class="card" href="webapp/installer/"><b>Install</b><span>Chrome or Edge, FM-1 on USB, about a minute</span></a>
    <a class="card" href="emu/"><b>Try it</b><span>the same firmware in your browser, with sound</span></a>
    <a class="card" href="manual/"><b>Manual</b><span>every page, knob and button</span></a>
  </div>

  <h2>Where it is</h2>
  <ol>
    <li class="now">Pipeline: 0.1 is JIANT FM under a new name, proven to install and roll back</li>
    <li>Trim: engines chosen for IDM, the voice ceiling measured, room made for samples</li>
    <li>The ACID screen theme</li>
    <li>BREAK: load breakbeats from this site, chop and rearrange them on the device</li>
    <li>Sound: TAPE wow and flutter, 303-style accent and slide, new FM and drum voices</li>
    <li>The song generator: ACID, BRAINDANCE, DRILL'N'BASS, BOC, AMBIENT, GLITCH</li>
  </ol>

  <h2>Before you install</h2>
  <p class="warn">At your own risk. Back up the full flash first with
    <a href="https://github.com/Quixotic7/MvaveFM1Unbricker">MvaveFM1Unbricker</a>. The FM-1's ROM bootloader cannot be
    overwritten: a failed install can be <a href="https://thefirmwarehub.com/#rescue">rescued</a>, and the official
    firmware can always go back on.</p>

  <footer>
    ACID FM is free software, GPL-3.0-only. Source: <a href="https://github.com/NorfolkHacker/acid-fm">github.com/NorfolkHacker/acid-fm</a>.<br>
    A fork of <a href="https://github.com/juanjiant-bit/JIANT">JIANT FM</a> by Juan Jiant, itself a fork of
    <a href="https://github.com/hugelton/Felucca">Felucca</a> by Hügelton Instruments. Not affiliated with M-VAVE.
  </footer>
</main>
</body>
</html>
```

- [ ] **Step 8: Build the site locally and look at it**

```bash
./build.sh >/dev/null && python3 web/make_site.py build/felucca.fwsc 0.1-local build/site-local \
  && cp web/landing/index.html build/site-local/index.html && python3 web/make_site.py manual build/site-local \
  && ls build/site-local build/site-local/webapp
```

Expected: `index.html`, `webapp/installer/`, `webapp/editor/`, `firmware/`, `manual/`. Serve it
(`python3 -m http.server -d build/site-local 8000`) and check http://localhost:8000/ shows the landing page, that
its Install and Manual links open, and that the layout holds at a 375 px wide window. (`emu/` is CI-only without
`emcc`, so its link 404s locally.)

- [ ] **Step 9: Tests and commit**

```bash
sh tests/run_tests.sh 2>&1 | tail -10
git add -A tools/build.py BUILDING.md README.md docs/JIANT-README.md LICENSING.md .github .gitignore web/STABLE web/PREVIEW web/CLASSIC web/landing android
git commit -m "build(site): ACID FM release names, README, licensing, GitHub config and the Pages site

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

Expected: all tests pass (if `tests/` or `web/test_*.mjs` reads `web/landing/frecuencia.html` or the anim folder, fix
the test's file list, not the deletion).

---

### Task 6: Full flash backup, install on the FM-1, roll back, install again

Proves the device path before anything is published. Record every result in `docs/acid-fm/device-log.md`.

**Files:**
- Create: `docs/acid-fm/device-log.md`

**Interfaces:**
- Consumes: `build/release-0.1/acid-fm-0.1.fwsc` (Task 5), `tools/fm1_install.py` with the ACID FM port filter (Task 4).
- Produces: the proven commit SHA (Task 8 pins `web/STABLE` to it); a verified flash backup outside the repo.

- [ ] **Step 1: USER STEP — full flash backup**

Ask the user to run (it needs root and the FM-1 in its ROM bootloader):

```bash
git clone https://github.com/Quixotic7/MvaveFM1Unbricker ~/MvaveFM1Unbricker
cd ~/MvaveFM1Unbricker && python3 -m venv .venv && .venv/bin/python fm1_unbrick.py setup
sudo modprobe sg
```

Then put the FM-1 into the bootloader as the Unbricker's README describes (the computer then sees a USB disk named
`UBOOT1.00`; `lsusb` / `lsblk` show it), and:

```bash
sudo .venv/bin/python fm1_unbrick.py backup ~/fm1-backups/2026-10-24-jiant
ls -l ~/fm1-backups/2026-10-24-jiant && sha256sum ~/fm1-backups/2026-10-24-jiant/*
```

Expected: the backup reads the 1 MiB flash twice and the reads match (the tool says so); the files are 1048576 B.
Power-cycle the FM-1: it boots JIANT as before. **Do not continue without this backup.**

Optionally also take the web editor's complete backup of the user data (projects, songs), with JIANT still on the
device: the rollback in Step 6 moves data written by 0.9.18 onto JIANT 0.9.9, which may not read it. The full-flash
backup stays the exact way back.

- [ ] **Step 2: Record the starting state**

```bash
. .venv/bin/activate
python3 tools/fm1_install.py --info
aconnect -l | grep -A1 -iE "jiant|acid"
```

If you want the editor's complete backup of projects and songs (see Step 1), take it now and note its file name in the
log. Create `docs/acid-fm/device-log.md`:

```markdown
# Device log

## 2026-10-24 — piece 1

- Backup: ~/fm1-backups/2026-10-24-jiant, sha256 <hash>, double read matched
- Editor backup of user data (optional): <file name, or none>
- Before: identity <from --info>, port <from aconnect>
```

- [ ] **Step 3: Rebuild the release on a clean HEAD**

The package that goes on the device must come from the commit that gets pinned, with no uncommitted changes:

```bash
git status --short
./build.sh --release 0.1
sha256sum build/release-0.1/acid-fm-0.1.fwsc
git rev-parse HEAD
./build.sh
```

Expected: `git status --short` prints nothing (if it does, stop and commit or stash first); the release build succeeds.
Record the `sha256sum` and the `git rev-parse HEAD` in `docs/acid-fm/device-log.md` now. The last `./build.sh` returns
the tree to the dev build; Steps 4 to 7 install the release package from `build/release-0.1/`.

- [ ] **Step 4: Install ACID FM 0.1**

```bash
python3 tools/fm1_install.py build/release-0.1/acid-fm-0.1.fwsc
```

Expected: the installer finds `JIANT FM`, then the update loader appears as `ACID FM Update` (the running JIANT stages
the loader from the package being installed), then the device restarts as `ACID FM`, and the run ends
`done: the FM-1 runs FM-1_901`.

- [ ] **Step 5: USER STEP — check it on the device**

```bash
python3 tools/fm1_install.py --info
aconnect -l | grep -A1 -iE "acid"
lsusb -v -d 1209:0001 2>/dev/null | grep -E "iManufacturer|iProduct"
```

Expected: identity `FM-1_901`; ALSA client `ACID FM`; `iManufacturer NorfolkHacker`, `iProduct ACID FM`. Ask the user to
confirm on the FM-1: the ABOUT page (MENU) shows ACID FM v0.1 and its QR code opens the repo URL on a phone; the
power-on song plays and sounds as it did on JIANT; keys and knobs respond; a project saves and loads.

- [ ] **Step 6: Roll back to JIANT**

Build JIANT's own public version (the commit in upstream's `web/STABLE`) in a separate worktree and install it with our
CLI:

```bash
JS=$(git show upstream/main:web/STABLE | tr -d ' \n') && echo "$JS"
git worktree add --detach build/jiant-stable "$JS"
(cd build/jiant-stable && python3 tools/build.py) | tail -2   # its own build/ inside the worktree
python3 tools/fm1_install.py build/jiant-stable/build/felucca.fwsc
python3 tools/fm1_install.py --info && aconnect -l | grep -iE "jiant|acid"
```

Expected: the install ends `done: the FM-1 runs FM-1_900`; its update loader appears as `JIANT Update`; the port is named `JIANT` or `JIANT FM` again; the user
confirms JIANT boots and plays.

- [ ] **Step 7: Install ACID FM 0.1 again**

```bash
python3 tools/fm1_install.py build/release-0.1/acid-fm-0.1.fwsc && python3 tools/fm1_install.py --info
git worktree remove build/jiant-stable
```

Expected: `done: the FM-1 runs FM-1_901`; the port is `ACID FM`.

- [ ] **Step 8: Log and commit**

Append to `docs/acid-fm/device-log.md`:

```markdown
- Installed acid-fm-0.1.fwsc (sha256 <sha256sum build/release-0.1/acid-fm-0.1.fwsc>), built from <git rev-parse HEAD>:
  FM-1_901, USB "ACID FM" / "NorfolkHacker"; ABOUT, sound, keys, save/load confirmed by the user
- Rolled back to JIANT <$JS> (FM-1_900): boots and plays
- Re-installed acid-fm-0.1.fwsc: FM-1_901
- Proven commit: <git rev-parse HEAD>
- Site / STABLE builds are dev builds (identity FM-1_900, felucca-v0.1-<sha>.fwsc) from the same sources; the
  device-proven release is FM-1_901.
```

```bash
git add docs/acid-fm/device-log.md
git commit -m "docs: piece 1 device log (backup, install, rollback, re-install)

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

If any install fails: stop. Do not retry blindly. The FM-1 can always be recovered through its ROM bootloader with
`sudo python3 ~/fm1_rescue.py` or `fm1_unbrick.py restore`, and the backup from Step 1 holds the exact earlier state.
Report what the installer printed.

---

### Task 7: USER STEP — publish the repo and the site

Outward-facing: creates a public repository. Ask the user for an explicit go-ahead before Step 1.

**Files:** none changed.

**Interfaces:**
- Produces: `origin` remote, `main` on GitHub, the Pages site live.

- [ ] **Step 1: Bring `main` up to the work (local, fast-forward only)**

The user merges locally, fast-forward only. Confirm with them, then:

```bash
git switch main && git merge --ff-only piece1-pipeline && git log --oneline -3
```

Expected: `main` now ends with this piece's commits, on top of the spec commits and JIANT's history.

- [ ] **Step 2: Create the public repo and turn on Pages (GitHub Actions source)**

Pages is enabled before the first push, so the push's workflow run can deploy.

```bash
gh repo create NorfolkHacker/acid-fm --public --description "ACID FM: IDM / braindance firmware for the M-VAVE FM-1 (a fork of JIANT FM)" --homepage "https://norfolkhacker.github.io/acid-fm/"
gh api -X POST repos/NorfolkHacker/acid-fm/pages -f build_type=workflow
```

Expected: the repo exists (empty) and the Pages call returns the site's JSON.

- [ ] **Step 3: Add the remote, push and watch the workflow**

```bash
git remote add origin https://github.com/NorfolkHacker/acid-fm.git
git push -u origin main
sleep 5; gh run list --repo NorfolkHacker/acid-fm --workflow emulator.yml --limit 1
```

Expected: the push succeeds, `git remote -v` lists `origin` and `upstream`, and the push starts the workflow. Only if no
run started, trigger it: `gh workflow run emulator.yml --repo NorfolkHacker/acid-fm`. Then wait for the run
(`gh run watch --repo NorfolkHacker/acid-fm <run id>`). Expected: the build and deploy jobs succeed. If the JieLi
toolchain or gitee download fails, re-run the workflow once (`gh run rerun <id>`); the workflow retries with backoff and
caches them after the first success.

- [ ] **Step 4: Check the live site**

```bash
for p in "" webapp/installer/ webapp/editor/ emu/ manual/; do printf "%-20s " "/$p"; curl -s -o /dev/null -w "%{http_code}\n" "https://norfolkhacker.github.io/acid-fm/$p"; done
```

Expected: `200` for all five. Ask the user to open https://norfolkhacker.github.io/acid-fm/ in Chrome and check the
landing page, that the installer page connects to the FM-1 (it should name the firmware ACID FM v0.1, or JIANT-family,
and offer the current build), and that the emulator plays. Do not install from the site in this step: Task 6 already
proved the device path.

- [ ] **Step 5: Record the evidence for the web tests**

The workflow (`emulator.yml`) builds and deploys; it does not run the web tests. The evidence that they pass is the
local Node run in Tasks 4 and 5 (`node web/test_web.mjs` and the other `web/test_*.mjs`, all passing, including the ACID FM
installer cases), plus the workflow's build and deploy jobs succeeding. Note both in the final report.

---

### Task 8: Pin the public installer to the proven build

**Files:**
- Modify: `web/STABLE`

- [ ] **Step 1: Pin**

Write the proven commit from `docs/acid-fm/device-log.md` (Task 6 Step 8) into `web/STABLE`, one line, the full SHA:

```bash
git switch -c pin-stable-0.1
printf '%s\n' "<proven commit SHA>" > web/STABLE
git add web/STABLE
git commit -m "site: the public installer offers 0.1 as proven on the device

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 2: Merge and push (user go-ahead)**

```bash
git switch main && git merge --ff-only pin-stable-0.1 && git push
```

Then watch the workflow run as in Task 7 Step 3 and confirm the public installer page shows the pinned build's version
(`0.1-<first 7 of the SHA>` or `v0.1-<sha>`, as `make_site.py` names it).

- [ ] **Step 3: Tag the release**

```bash
git tag -a v0.1 -m "ACID FM 0.1: JIANT FM 0.9.18 renamed; pipeline proven" <proven commit SHA>
git push origin v0.1
gh release create v0.1 --repo NorfolkHacker/acid-fm --title "ACID FM 0.1" \
  --notes "JIANT FM 0.9.18 under a new name: the build, the site, install and rollback proven. Back up your flash first." \
  build/release-0.1/acid-fm-0.1.fwsc build/release-0.1/acid-fm-0.1-app.bin build/release-0.1/SHA256SUMS \
  build/release-0.1/LICENSE build/release-0.1/LICENSING.md
```

Expected: the release page lists the five files. (Release builds carry Apache-2.0 SDK files; `LICENSE`,
`LICENSING.md` and `LICENSES/` are in the repo at the tag, which the release links to.)
