# Braindancer: the rename (release 0.7)

Date: 2026-10-29. Status: approved in brainstorming; awaiting spec review. Release: **0.7** (version numbering continues).

User decisions (2026-10-29): the firmware is renamed **Braindancer** (it was ACID FM; a later choice of "IDM" is
superseded) — based on the IDM / braindance music of the 90s. Everything user-facing is renamed in one release right after
0.6. The GitHub repo `NorfolkHacker/acid-fm` becomes `NorfolkHacker/braindancer`, and a new small `acid-fm` repo forwards
old website links. History keeps its old name.

## 1. Scope

A rename only: no feature changes, no storage or protocol changes, sound goldens unchanged.

## 2. Device

- Splash wordmark **BRAINDANCER** in the existing wordmark face, one line, fitted to 240 px (the size chosen so the word
  fits with the current side margins; a render test checks it fits).
- ABOUT: "Braindancer v0.7", the "Built on JIANT FM and Felucca" line and every credit kept.
- Every device string that names the firmware ("ACID FM") says "Braindancer" (upper case where the face is upper case:
  BRAINDANCER). The colour palette keeps its internal name (ACID tokens); user-facing text that called the look "the ACID
  look" says "the Braindancer look".
- USB product string **BRAINDANCER** (the MIDI port name on computers); the update loader's string **BRAINDANCER UPD**
  (USB string length limit). Manufacturer string unchanged.

## 3. Finding the device

- The web installer, `tools/fm1_install.py` and every name match accept **braindancer** and keep accepting acid fm, jiant
  and felucca (a device still on ACID FM is found).
- The unbricker (a separate program, not changed) is called with `--port "BRAINDANCER"` from then on.

## 4. Files and words

- Release package `braindancer-X.Y.fwsc` and folder `release-X.Y/` contents named after it (`tools/build.py`).
- `docs/acid-fm/` moves to `docs/braindancer/` (device log, genre research, mockups); links updated.
- README, landing page, manual (web and `docs/MANUAL.md`), cheat sheets, web editor / installer / emulator titles,
  `.github` issue templates and workflow names, `BUILDING.md`, `LICENSING.md`'s project-name mention: Braindancer.
- The naming tests follow: "Braindancer is built on JIANT FM and Felucca"; JIANT only in credits; English only; no band
  names on the device (RETROBOC stays — the user's genre name).

## 5. What keeps "ACID FM"

Past news entries (0.1–0.6), past device-log entries, specs and plans already written, and code comments that record
history ("ACID FM 0.5: ..."). The manual's 0.7 news says "ACID FM is now Braindancer". The `acid-fm` name also stays as
the forwarding repo.

## 6. GitHub (after the device check, on the user's go-ahead)

1. Rename the repo to `braindancer` (GitHub redirects repo links; Pages moves to norfolkhacker.github.io/braindancer/).
2. Create `NorfolkHacker/acid-fm` with GitHub Pages serving one page that forwards (meta refresh + link) to the new site,
   keeping the path where possible (installer, manual, emulator).
3. Point the local remote at the new URL.

## 7. Tests and release

- Full host suite green; a check that no user-facing file or device string says "ACID FM" outside the history in §5
  (extend `tests/built_on_test.py`); the splash render fits; the installer's name match tested for BRAINDANCER and ACID FM.
- Device (0.7): backup, `--release 0.7`, install (the tool finds the device as ACID FM, it returns as BRAINDANCER),
  status, look at the splash and ABOUT, check the computer's MIDI port name; log; merge and push on the user's go-ahead;
  then §6.
