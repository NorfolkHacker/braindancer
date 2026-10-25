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
