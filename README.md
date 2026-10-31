# Braindancer

*Formerly ACID FM.*

**IDM / braindance firmware for the M-VAVE FM-1**. Braindancer is becoming a firmware with virtual analog, FM and a generator that
writes whole songs on the device. Acid-coloured, in the spirit of Aphex Twin, Boards of Canada and the Rephlex
catalogue.

**[Install](https://norfolkhacker.github.io/braindancer/webapp/installer/)** (Chrome or Edge, over USB) ·
**[Try it in the browser](https://norfolkhacker.github.io/braindancer/emu/)** ·
**[Manual](https://norfolkhacker.github.io/braindancer/manual/)**

## Status: 0.9

0.9 is about the generator (piece 6), on top of 0.8:

- **GENERATE**: GEN is one page now (ENERGY, FORM, COMPLEX, TAPE) and **OCT+ GENERATE** picks a style itself, out of ten.
  About one press in ten is WILD (no style); about 45 % of the rest are **blends** of the style with a partner that goes
  well with it, named on the panel, for example RETROBOC+AMBIENT 0421. It never picks the same style twice in a row.
  The GENRE and SEED knobs and GEN pages 2 and 3 are gone.
- **Recent pieces**: PRESETS steps from NEW back through the last 8 pieces, and OCT+ RECALL brings one back and sets the four
  knobs to its values. GENERATE and RECALL are refused while a song (ARRANGE) is playing (STOP THE SONG FIRST). The list is cleared at power-off; nothing of GEN
  is saved in projects or settings.
- **TAPE in GEN**: on AUTO each style sets AGE (RETROBOC 60, AMBIENT 40, AMB TECHNO 25, IDM 20, MICRO 15, the others 0; a
  blend gets the main style's plus half the partner's, at most 70; WILD 0 to 50). RETROBOC and AMBIENT keep their tape and
  the music is thinned around it; for the other styles the tape gives way before the music is thinned, when a piece is too heavy. A TAPE amount
  set with the knob is never changed. OFF and 10 to 100 % set AGE for every piece.
- **Every piece is fitted** to what the FM-1 can play, with a budget calibrated on the device (2 CORES); one core is fitted
  too but not calibrated. A new piece unmutes every track.
- **The AGE retune**: 50 is clearly worn (audible wobble, some hiss, a darker tone) and 100 is mangled and darker. Projects
  from 0.8 with AGE above 0 sound more worn.
- Projects, presets and settings load as before.

Before that, 0.8 was about the sound (piece 5), on top of 0.7:

- **TAPE**, a new master effect after CLIP, PUNCH and DUCK: WOW, FLUTTER, TAPE SAT, HISS and TAPE TONE, with one **AGE** knob that
  sets them together. FX › MASTER is now CLIP · PUNCH · DUCK · AGE; DUCK TIME moved to FX › MORE › MASTER and TAPE has its
  own group in FX › MORE. It is off at first, saved with the project (a 0.7 project loads with TAPE off), AGE can be
  automated, and it adds about 4 ms of delay to everything while on. Under overload it is the first of the safety's
  last-resort steps, after the voices are thinned (your settings are never changed, and FX › MASTER shows TAPE PAUSED · CPU
  while it is off).
- **The 303 acid response** (ANALOG, MONO or LEGATO): accents that follow each other climb the cutoff and relax after a
  plain step; a slide is a short glide (about 58 ms) with no new attack, whatever GLIDE is. Notes played on the keys keep
  the old accent.
- **20 new factory presets** (ANALOG ACID SQUELCH, ACID SQUARE, ACID LEAD, DETUNE PAD, DUSTY LEAD, RUBBER BASS, BRAIN BLIPS;
  FM6 GLASS EP, METAL BASS, TIN BELL, MUSIC BOX; PHASE RESO SWEEP, DIGI CHOIR, CHIME KEYS; LOFI DUSTY KEYS, WOBBLE LEAD,
  CRUSH BASS; VOICE DREAM CHOIR, AIR CHOIR, LOW HUM PAD), added at the end of each list. GEN still picks from the earlier ones.
- **DRUM-X kits**: 909 CLUB (shorter, clickier kick, noisier snare, bright short hats, a ride on BELL), 808 BOOM (long low
  kick, tonal snare, soft hats, long tom, a cowbell on BELL), and crunchier lanes in LOFI DUST, TECHNO RAW and BREAKS.
- The web editor has the six TAPE globals (G 96 to 101); see `web/EDITOR_PROTOCOL.md`.
- Still built on JIANT FM and Felucca.

Before that, 0.7 was the rename: ACID FM is now **Braindancer**, after braindance, the 90s IDM of Aphex Twin and the Rephlex catalogue.
It is a rename only: no new features, and projects, presets and settings load as before.

- The splash and ABOUT say BRAINDANCER, and the FM-1 shows up over USB as a MIDI device called BRAINDANCER (formerly ACID FM).
- The website is now https://norfolkhacker.github.io/braindancer/ (the installer, emulator and manual); the old links
  forward there. The code is at github.com/NorfolkHacker/braindancer.
- The installer, the tools and the web editor find a device on Braindancer or ACID FM.
- Still built on JIANT FM and Felucca.

Before that, 0.6 was SAVE, the MENU and the dialogs in the ACID look (piece 3c-2), on top of 0.5.2:

- SAVE is SOUND, SONG, PROJECT and MORE. SOUND lists the 32 user slots; SONG puts the sections on top and the song's
  rows under them as chips; PROJECT shows four cards, a dot on the one with changes not stored, and warns on LOAD only
  when there are some; the tools are in MORE.
- The MENU is AUDIO, CLICK, CONTROLS and MORE; ABOUT and PROFILE are restyled.
- One full-screen dialog for every confirmation: it says what is lost and how to get it back.
- PLAYS (SEQ › MORE › TRACK): a track plays only on some loops; you can change it while the music plays.
- The clock rule: under a USB or TRS clock, GEN, the demo and the web editor leave the tempo alone; CPU TEST uses the
  internal clock and puts CLOCK back.
- The web editor has a per-track MIDI page. The example names are in English. Projects, presets and settings from 0.5
  load as before.

Before that, 0.5.2 fixes harsh glitches on some generated songs. A few EXPLORE pieces asked for more than the FM-1's processor can
render (a 2-second delay over a busy six-track piece, for one), and the audio skipped. GEN now keeps EXPLORE pieces within
a CPU budget measured on the device, and the long delay is only used where there is room for it; under a sudden overload
the sound engine falls back step by step (the long delay to its short mode, then a generated piece's texture, then its
arp) instead of skipping, and comes back once the load eases.


0.5.1 retunes GEN's genres (names, tempo ranges and swing, from [research into the real styles](docs/braindancer/genre-research.md):
GLITCH HOP and AMB TECHNO replace BROKEN BEAT and DRIFT; DRILL'N'BASS 155–172, ELECTRO 124–138, RETROBOC 68–100) and makes
an external clock (USB or TRS) hold a steady tempo.

0.5 is SEQ, GLO and MIDI in the ACID look (piece 3c-1), on top of 0.4:

- SEQ is STEPS, STEP EXTRAS, PATTERN and MORE. STEP EXTRAS holds the chance, ratchet, condition and micro-timing of the
  picked step; with a step held, MORE's rows lock on it (the ACID lock card).
- GLO is LIVE, MIX, MIDI and MORE. LIVE is the performance page (rate, loop, thin, DJ filter, the keys' tricks); MIX is
  the six tracks' levels, pan and sends; MORE holds the rest (rails, tempo, system, undo).
- The MIDI page: per track MIDI in, out and channel; the clock INT, USB or TRS; clock out over USB; the keyboard plays
  the selected track. Older settings and projects load as before.
- SEQ TOOLS, REC and GLO layers (the held layers) in the ACID look, with the keys' names and states on the keys.
- Level bars in dB on LIVE, MIX and HOME.
- The manual is English only and Braindancer's own.

Before that, 0.4 was the sound sections in the ACID look (piece 3b), on top of 0.3:

- EDIT, ENV, ARP, SCL, LFO and FX in the ACID look: a picture of the sound, four knobs, and MORE for the rest, with
  names in plain words. The old creatures are removed.
- The held layers (EDIT, MACRO, FX, SCL) in the same look; with a step held, MORE's rows lock on that step too.
- GEN shows ten IDM genres (IDM, BROKEN BEAT, BREAKS, DRILL'N'BASS, ACID, ELECTRO, MICRO, RETROBOC, AMBIENT, DRIFT),
  plus EXPLORE and RANDOM; techno and house are hidden (a song saved with one generates as before).
- At power-on PLAY plays a short demo (beat, acid line, pad, 124 BPM); NEW PROJECT still starts empty.
- The first time 0.4 starts, MENU › TRACKS switches to 6 once (the demo uses T5 and T6); a 4 picked after that stays.
- HOME TRACKS: KNOB 1-3 are three tracks' levels, KNOB 4 the BPM.
- Steadier timing: the screen draws more slowly when the audio is busy.
- SAVE, the MENU and the dialogs keep their inherited layouts in ACID colours until 0.6.

Before that, 0.3 was the new look (piece 3a, UI foundations), on top of 0.2:

- One ACID palette (the inherited palettes and skins are gone), Inter Tight lettering with gradient and glow, shared widgets.
- HOME is TRACKS, VISUAL and GEN (HOME again or SELECT cycles; HOME+SELECT goes to GEN): TRACKS is six ridgelines with
  each track's level in dB, the four knob-controlled tracks in the knobs' colours; VISUAL is six rings pulsing with hits.
- The wordmark splash replaces the inherited one.

Before that, 0.2 was the trim, on top of 0.1 (the build, install and rollback pipeline, proven end to end):

- ANIMAL and WHEEL are retired: stored sounds that used them load as ANALOG, and T3 at power-on is PHASE.
- 4 songs; 128 KiB of flash reserved for breaks (an 8 KiB gap may join later), for breakbeats coming in a later version.
- The voice budget is measured on the FM-1: 12 voices at once on two cores, 8 per track.

The Braindancer features arrive in steps (see `docs/superpowers/specs/2026-10-24-acid-fm-design.md`, written when it was still ACID FM):

1. Pipeline (0.1)
2. Trim (0.2): engines for IDM, the voice ceiling measured, room made for samples
3. The ACID screen theme (3a in 0.3: look, HOME, splash; 3b in 0.4: the sound sections; 3c-1 in 0.5: SEQ, GLO, MIDI; 3c-2 in 0.6: SAVE, MENU, dialogs, PLAYS)
4. Sound (0.8): TAPE (wow, flutter, saturation), 303-style accent and slide, new presets and drum kits
5. The song generator (0.9): GENERATE picks a style, blends, recent pieces, TAPE in GEN, the AGE retune

The manual is Braindancer's own; it grows as Braindancer's features land.

## Built on

Braindancer builds on open firmware for the FM-1: **[JIANT FM](https://github.com/juanjiant-bit/JIANT)** by Juan Jiant
and **[Felucca](https://github.com/hugelton/Felucca)** by Hügelton Instruments, with SLOOP's song system. The
upstream README is kept in [docs/JIANT-README.md](docs/JIANT-README.md). Every third-party part and its licence is listed in
[LICENSING.md](LICENSING.md).

## Build

See [BUILDING.md](BUILDING.md). On Linux x86-64:

    python3 -m venv .venv && . .venv/bin/activate && pip install -r requirements-dev.txt
    export JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK
    ./build.sh && CC=tools/host-cc sh tests/run_tests.sh

(`CC=tools/host-cc` is for GCC 14+ hosts; see BUILDING.md.)

## Safety

Installing firmware is at your own risk. Back up the full flash first
([MvaveFM1Unbricker](https://github.com/Quixotic7/MvaveFM1Unbricker) `backup`). The FM-1's ROM bootloader cannot be
overwritten, so a failed install can be recovered
([FM-1 Firmware Hub rescue](https://thefirmwarehub.com/#rescue)), and the official firmware can always go back on.

## Licence

GPL-3.0-only, as JIANT and Felucca. See [LICENSE](LICENSE) and [LICENSING.md](LICENSING.md).
