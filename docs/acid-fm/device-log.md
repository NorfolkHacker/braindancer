# Device log

## 2026-10-24 — piece 1

- Backup: ~/fm1-backups/2026-10-24-jiant/fm1-recovery-20261024-130125/full-flash-before.bin (MvaveFM1Unbricker
  `backup`), sha256 8531fc3aedf14ef1bddb4cca34a46a6d168d1dfe061d73a6a644e255fb9c5349, both reads identical; flash ID
  0x856014; the FM-1 entered UBOOT1.00 with `fm1_unbrick.py softkey --port "JIANT FM"`
- Before: identity FM-1_909 (JIANT 0.9.9-beta4a), port "JIANT FM"
- Release rebuilt on a clean HEAD dcb092c2a7a90f3a8f36bf14a60d62c40d79535a: acid-fm-0.1.fwsc, 610122 B, identity
  FM-1_901, sha256 0f5a56111a62f264123a8581c0b10b8cfc2093d18afe3b96b0fa2e37b957157d
- Installed acid-fm-0.1.fwsc with tools/fm1_install.py: `done: the FM-1 runs FM-1_901`; port "ACID FM",
  USB iManufacturer "NorfolkHacker", iProduct "ACID FM". The user confirmed on the device: splash, ABOUT
  (MENU > SYSTEM > ABOUT), sound, keys and knobs, save / load
- Rolled back to JIANT 813869db36ca5f08955c97682249498bf7a41c4c (upstream web/STABLE, 0.9.9-beta4a, built locally):
  `done: the FM-1 runs FM-1_909`, port "JIANT FM"
- Re-installed acid-fm-0.1.fwsc: `done: the FM-1 runs FM-1_901`, port "ACID FM"
- Proven commit: dcb092c2a7a90f3a8f36bf14a60d62c40d79535a
- Site / STABLE builds are dev builds (identity FM-1_900, felucca-v0.1-<sha>.fwsc) from the same sources; the
  device-proven release is FM-1_901.

## 2026-10-24 — piece 2, voice ceiling (Task 5)

- Backup before the piece 2 installs: ~/fm1-backups/2026-10-24-acid-fm-0.1/fm1-recovery-20261024-163836/full-flash-before.bin,
  sha256 226283d774d398764d1776a95c1f4eef28a575a7ff8b3cfbd6379815086db771, both reads identical (device on ACID FM 0.1)
- Installed build/acid-fm-vc12.fwsc (T 12, NVOICE 8; dev identity FM-1_900): `done: the FM-1 runs FM-1_900`
- CPU TEST after a power cycle: PEAK 100 %, LATE 113, no clicks heard. T stays 12; 14 and 16 not run
  (docs/acid-fm/voices.md)

## 2026-10-24 — piece 2, ACID FM 0.2 (Task 6)

- Release built on a clean HEAD d1f235b97dc867dd5bf0aaf2f416d1a783057f4d: acid-fm-0.2.fwsc, 610122 B, identity FM-1_902,
  sha256 e401c493a5ce47f63e21be726f99fbcb48e58095e5140dcde9af1f8fb46447f0
- Installed over the vc12 test build: `done: the FM-1 runs FM-1_902`; console `status`: acid-fm v0.2, CORES 2
- Right after the install's restart the console showed audio_late 143 (all within the first 16 s, none after); after a
  power cycle: audio_late 0, audio_max_us 863 over 2 minutes idle — the lates belong to the post-install restart only
- The user confirmed on the device: splash v0.2, T3 PHASE at power-on, no ANIMAL / WHEEL in PRESETS, 4 projects in
  SAVE › LIBRARY › PROJECTS, save / load
- Proven commit: d1f235b97dc867dd5bf0aaf2f416d1a783057f4d

## 2026-10-25 — piece 3a, ACID FM 0.3 (UI foundations, HOME, splash)

- Backup before: ~/fm1-backups/2026-10-25-acid-fm-0.2/fm1-recovery-20261025-004847/full-flash-before.bin, sha256
  bf6aa66d9c294595d184aa5bee3efe093a443a7dba3d7bb4df64a4d1709ac3cf, both reads identical
- Release built on clean HEAD 7a2f647a2d41835acb111b671c876abfdf18bf42: acid-fm-0.3.fwsc, 610122 B, identity FM-1_903,
  sha256 e0d13c9251cc1a5edf7f7179dbcf30e405ea1cb05fdbd39277bab4d7518be415; installed over 0.2: `done: the FM-1 runs FM-1_903`
- Console: acid-fm v0.3, CORES 2; idle cpu_pct settles to 7 % (as 0.2)
- No-manual check (spec §0), passed first try: loudest track read from HOME (drums), T5 level turned down from HOME,
  HOME → VISUAL → back, play state visible. Found: power-on patterns are empty, so PLAY is silent with no hint
  (logged as a priority for piece 6 / 3c)
- Generated song (GEN): cpu 44 %, audio_max 1907 us, audio_late 0, shed 0
- CPU TEST song (worst case), late audio blocks per second over ~17 s: MENU 6.2 (baseline), HOME VISUAL 7.0,
  HOME TRACKS 8.2; no clicks heard. VISUAL adds nothing over TRACKS (requirement met); TRACKS +2/s over a static page
  under the worst case → reduce in 3b
- Proven commit: 7a2f647a2d41835acb111b671c876abfdf18bf42

## 2026-10-26 — piece 3b, ACID FM 0.4 (sound sections, genres, demo)

- Backup before: ~/fm1-backups/2026-10-26-acid-fm-0.3/fm1-recovery-20261026-182811/full-flash-before.bin, sha256
  f13cc80e07185dcab5b9f8bdaf338c2109431f76c0afb9f5f2bf1699a8862cff, both reads identical
- Release built on clean HEAD 041f010: acid-fm-0.4.fwsc, 610122 B, identity FM-1_904, sha256
  d7f985b92c09db844924b6d71b920caa3fd12c4597596f1b7641ab0484329677; installed over 0.3: `done: the FM-1 runs FM-1_904`
- Console after install: acid-fm v0.4, CORES 2, idle cpu_pct 6 %, bpm 124 (the power-on demo loaded)
- User checks, all passed ("all seems good"): first 0.4 boot turns TRACKS 6 on once and PLAY plays the demo (beat,
  acid line, pad); no-manual tasks (T5 brighter, delay on T3, scale to minor, PLAY on a fresh boot); TRACKS 4 survives
  a reboot; CPU TEST song on MENU, TRACKS, an EDIT page and LFO with no clicks; fast NOISE DUST turns while playing;
  GEN RANDOM names come from the shown genre list
- Console under the CPU TEST song (worst case, ~11 min uptime): cpu_pct 50, audio_max 3277 us, audio_lost 0; the
  governor sheds voices as designed (per-page late blocks/s not read this time)
- Proven commit: 041f010

## 2026-10-27 — piece 3c-1, ACID FM 0.5 (SEQ, GLO, MIDI page)

Pending (Step 2, run with the user; nothing below has been done yet):

- Backup before (`softkey --port "ACID FM" --yes`, then `fm1_unbrick.py backup`): pending
- Release build `./build.sh --release 0.5` on the clean HEAD, then `tools/fm1_install.py build/acid-fm-0.5.fwsc --port "ACID FM" --yes`: pending
- Serial `status` (version v0.5, cpu_pct, audio_late): pending
- No-manual checks: make step 5 a ratchet; make T5's notes play only every other bar; send T2 to MIDI channel 10; follow
  the clock from USB; loop the last bar live and let it go: pending
- CPU TEST song, late audio blocks per second on SEQ STEPS, GLO LIVE with tricks, and the MIDI page: pending
- A DAW or second device on USB sending clock and notes: pending
- Merge ff to main and push, on the user's go-ahead: pending
