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
  (docs/braindancer/voices.md)

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

- Backup before: ~/fm1-backups/2026-10-27-acid-fm-0.4/fm1-recovery-20261027-202428/full-flash-before.bin, sha256
  3df19dd664bd8f03b094281be5e14c6a5ff9e9e95b6b071f8054532641718814 (1 MiB, 0 of 143 firmware sectors blank).
- First install (79d5acd), FM-1_905: status v0.5, cpu_pct 26, audio_max_us 1995, audio_late 0 at 20 s.
- Found on the device: following a Korg SQ-1's clock over TRS at 120, the FM-1 read 117..123 and re-tempoed four
  times a beat (audible drift). The inherited follower (Felucca 1.1.5) measured six clocks at a millisecond's
  resolution and applied each reading raw. Fixed in 9962f2d (a line fitted through a beat of clocks, smoothed,
  applied past 0.25 %; a jump over 4 % re-measured from the last quarter beat); test with +-3 ms jitter.
- Reinstalled (9962f2d), FM-1_905, fwsc sha256 ce2b5a70599975e6b848f66ac652ff75869bff3f61886924422382191d316a34:
  following the SQ-1 over TRS, 20 readings stopped and 16 playing all steady (120, then 121 after the user moved the
  SQ-1's tempo knob; the follower tracked the move and settled). The user: "steady". audio_late +0.5..2/s while
  playing (109 at start-up, not rising while stopped).
- CPU TEST song (worst case, 160 BPM), console over 10-16 s: TRACKS 6 on SEQ STEPS cpu 72-76 %, ~68 late blocks/s,
  ~91 voices shed/s, audio_max 4463 us; TRACKS 6 on HOME ~70 late/s (the same: not the page); TRACKS 4 on HOME cpu
  58-60 %, 3.8 late/s, 0 shed (0.3 measured 6-8/s on this song). No drops heard (user). The 6-track figure is the
  worst-case song past what the chip renders, absorbed by the voice governor; 0.5 is not heavier than before.

Still pending:

- No-manual checks: make step 5 a ratchet; make T5's notes play only every other bar; send T2 to MIDI channel 10;
  follow the clock (done above, over TRS); loop the last bar live and let it go: user feedback pending
- USB MIDI out (NOTES OUT KEYS) to a computer or device: deferred by the user ("I'll check midi to keys later")
- Merge ff to main and push, on the user's go-ahead: pending

## 2026-10-27 — ACID FM 0.5.1 (GEN genres retuned)

- Release built on 2099c43: acid-fm-0.5.1.fwsc, 610122 B, identity FM-1_905, sha256
  063a24b7873485fa16bf865363d21ef0d76ed87c3b0d3cd33caecf919e8c685b; installed over 0.5 (backup of 2026-10-27 covers it).
- Console: acid-fm v0.5.1, cpu_pct 25, idle and steady (audio_late carried over the update's restart, not rising).
- User tried the retuned genres (GLITCH HOP, AMB TECHNO, DRILL'N'BASS 155-172, RETROBOC 68-100 ...): "this is about
  right". RETROBOC stays the name (card RETRO).

## 2026-10-28 — ACID FM 0.5.2 (generated songs no longer glitch)

- Reported by the user: harsh digital glitches on some random GEN songs (EXPLORE 3423, then EXPLORE 9641; kicks
  distorting). Console on 0.5.1 while glitching: audio_lost ~2/s (3423, 2 s quarter-rate grain delay) and ~4.5/s (9641,
  short delay), late ~60/s, CPU 68-87 %. 3423 with DLY TIME short: lost 0 — the quarter-rate long delay costs ~10 % of
  the CPU and the governor, at its floor, could not shed it; 9641 was simply heavier than the device can render.
- Tried and dropped: the delay's quarter-rate line in RAM (no change on the device, ~2.2 lost/s); grain off (~1.8/s).
- Fix (branch fix-delay-cost): GEN keeps EXPLORE within a device-measured CPU budget (model x1.5) and keeps the
  quarter-rate delay off busy pieces (RANDOM and the genres get the delay rule only); a safety net in the audio engine
  falls back step by step under overload at the governor's floor (long delay -> short mode, then the GEN texture, then
  the arp), per-lever back-off, cleared whenever the music is replaced. Console counters dly_safety / safety_tex /
  safety_arp.
- Test build on the device: EXPLORE 9641 -> lost 0, late 0, CPU 49-52 %, max 2.2 ms; EXPLORE 3423 with the delay forced
  long -> lost 0, 4 late in 9.6 s, dly_safety 1. The user: "sounds clean".

## 2026-10-29 — piece 3c-2, ACID FM 0.6 (SAVE, MENU, dialogs, PLAYS)

- Backup before: ~/fm1-backups/2026-10-29-acid-fm-0.5.2/fm1-recovery-20261029-204515/full-flash-before.bin, sha256
  da69493722b3f50d578f926ba55413dc29234dd4ffccb4ebdf8665293660f09b (1 MiB, 0 of 143 firmware sectors blank).
- Release built on clean HEAD a11bb5f: acid-fm-0.6.fwsc, 610122 B, identity FM-1_906, sha256
  445db1b68ae3455de0744568e71e0222d0713f7c232aba7949e90582e006f286; installed over 0.5.2: `done: the FM-1 runs FM-1_906`.
- Console: acid-fm v0.6, idle cpu_pct 7, audio_late 0, audio_lost 0.
- No-manual tasks (save T3's sound to a user slot; make a song A-B-B-C; start a new project and get the old one back;
  make T5 play every other bar with PLAYS; turn the click on with a count-in): the user, "all seems ok".
- CPU TEST song (160 BPM, TRACKS 6), console over 8.6-10.6 s: SAVE › MORE lost 0, late ~39/s, cpu 74-75 %, max 4.2 ms;
  MENU › MORE lost 0, late ~39/s, cpu 75-76 %; a dialog open (CLEAR SEQ) lost 0, late ~38/s, cpu 74 %. (0.5 on the same
  test: ~68-70 late/s.) The safety net did not fire during the test (safety counters unchanged).
- Proven commit: a11bb5f.

## 2026-10-29 — Braindancer 0.7 (the rename)

- Backup before: ~/fm1-backups/2026-10-30-acid-fm-0.6/fm1-recovery-20261030-180449/full-flash-before.bin, sha256
  f60f69a80cc33a3b58bf3a60434f6d015e1f69f55ec83f49a64f7e10f71f3e45 (1 MiB, 0 of 143 firmware sectors blank).
- Release built on clean HEAD aec409f: braindancer-0.7.fwsc, 610126 B, identity FM-1_907, sha256
  f5f9061fb06e246b1aa5150f15c77b187886e0495129bbd0d87a652b61f6c1ae. `tools/fm1_install.py --port "ACID FM"` found the
  device as "ACID FM" (FM-1_906) and finished: `done: the FM-1 runs FM-1_907`.
- Console: `braindancer v0.7`, idle cpu_pct 7, audio_lost 0.
- The computer's MIDI port: "BRAINDANCER MIDI 1" (ALSA client "BRAINDANCER").
- The user checked the splash (BRAINDANCER types out and fits) and ABOUT (Braindancer v0.7, the address line fits): yes.
- Proven commit: aec409f.

## 2026-10-30 — Braindancer 0.8 (TAPE, acid, presets, kits)

Release prep done (version v0.8, cheat sheets, manual and navigation map regenerated); every device line below is
pending until the device session.

- Backup before: pending (softkey into UBOOT, then `fm1_unbrick.py backup`; path and sha256 to be recorded).
- Release built on clean HEAD: pending (braindancer-0.8.fwsc, size, identity, sha256).
- Install: pending (`tools/fm1_install.py ... --port "BRAINDANCER"`).
- Console `status`: pending (version `braindancer v0.8`, idle cpu_pct, `audio_lost`, `safety_tape` 0).
- No-manual task "make it sound like a worn tape": pending (expected path FX > MASTER > KNOB 4 AGE).
- Acid line with accents and slides on an ANALOG mono track (ACID SQUELCH): pending (the climb heard; slides glide
  without a new attack).
- CPU TEST with TAPE off: pending (late / lost blocks per second over ~10 s on SEQ STEPS, cpu %, `safety_tape`).
- CPU TEST with AGE 100 set before it: pending (same figures; which run stored `cpt_res`, pre-flight 14).
- GEN piece at AGE 50: pending (no lost blocks).
- Proven commit: pending.
