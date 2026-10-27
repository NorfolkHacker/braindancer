# ACID FM

**IDM / braindance firmware for the M-VAVE FM-1**. ACID FM is becoming a firmware with virtual analog, FM, chopped
breakbeats you load yourself, and a generator that writes whole songs on the device (planned, see Status). Acid-coloured, in the spirit of Aphex Twin, Boards of Canada and
the Rephlex catalogue.

**[Install](https://norfolkhacker.github.io/acid-fm/webapp/installer/)** (Chrome or Edge, over USB) ·
**[Try it in the browser](https://norfolkhacker.github.io/acid-fm/emu/)** ·
**[Manual](https://norfolkhacker.github.io/acid-fm/manual/)**

## Status: 0.5

0.5 is SEQ, GLO and MIDI in the ACID look (piece 3c-1), on top of 0.4:

- SEQ is STEPS, STEP EXTRAS, PATTERN and MORE. STEP EXTRAS holds the chance, ratchet, condition and micro-timing of the
  picked step; with a step held, MORE's rows lock on it (the ACID lock card).
- GLO is LIVE, MIX, MIDI and MORE. LIVE is the performance page (rate, loop, thin, DJ filter, the keys' tricks); MIX is
  the six tracks' levels, pan and sends; MORE holds the rest (rails, tempo, system, undo).
- The MIDI page: per track MIDI in, out and channel; the clock INT, USB or TRS; clock out over USB; the keyboard plays
  the selected track. Older settings and projects load as before.
- SEQ TOOLS, REC and GLO layers (the held layers) in the ACID look, with the keys' names and states on the keys.
- Level bars in dB on LIVE, MIX and HOME.
- The manual is English only and ACID FM's own.

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
- SAVE, the MENU and the dialogs keep their inherited layouts in ACID colours until 3c-2.

Before that, 0.3 was the new look (piece 3a, UI foundations), on top of 0.2:

- One ACID palette (the inherited palettes and skins are gone), Inter Tight lettering with gradient and glow, shared widgets.
- HOME is TRACKS, VISUAL and GEN (HOME again or SELECT cycles; HOME+SELECT goes to GEN): TRACKS is six ridgelines with
  each track's level in dB, the four knob-controlled tracks in the knobs' colours; VISUAL is six rings pulsing with hits.
- The wordmark splash replaces the inherited one.

Before that, 0.2 was the trim, on top of 0.1 (the build, install and rollback pipeline, proven end to end):

- ANIMAL and WHEEL are retired: stored sounds that used them load as ANALOG, and T3 at power-on is PHASE.
- 4 songs; 128 KiB of flash reserved for breaks (an 8 KiB gap may join later), for breakbeats coming in a later version.
- The voice budget is measured on the FM-1: 12 voices at once on two cores, 8 per track.

The ACID FM features arrive in steps (see `docs/superpowers/specs/2026-10-24-acid-fm-design.md`):

1. Pipeline (0.1)
2. Trim (0.2): engines for IDM, the voice ceiling measured, room made for samples
3. The ACID screen theme (3a in 0.3: look, HOME, splash; 3b in 0.4: the sound sections; 3c-1 in 0.5: SEQ, GLO, MIDI; 3c-2 next)
4. BREAK: load breakbeats from the web page, chop and rearrange them
5. Sound: TAPE (wow, flutter, saturation), 303-style accent and slide, new FM and drum voices
6. The song generator: ACID, BRAINDANCE, DRILL'N'BASS, RETROBOC, AMBIENT, GLITCH

The manual is ACID FM's own; it grows as ACID FM's features land.

## Built on

ACID FM builds on open firmware for the FM-1: **[JIANT FM](https://github.com/juanjiant-bit/JIANT)** by Juan Jiant
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
