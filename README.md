# ACID FM

**IDM / braindance firmware for the M-VAVE FM-1**. ACID FM is becoming a firmware with virtual analog, FM, chopped
breakbeats you load yourself, and a generator that writes whole songs on the device (planned, see Status). Acid-coloured, in the spirit of Aphex Twin, Boards of Canada and
the Rephlex catalogue.

**[Install](https://norfolkhacker.github.io/acid-fm/webapp/installer/)** (Chrome or Edge, over USB) ·
**[Try it in the browser](https://norfolkhacker.github.io/acid-fm/emu/)** ·
**[Manual](https://norfolkhacker.github.io/acid-fm/manual/)**

## Status: 0.3

0.3 is the new look (piece 3a, UI foundations), on top of 0.2:

- One ACID palette (JIANT's palettes and skins are gone), Inter Tight lettering with gradient and glow, shared widgets.
- HOME is TRACKS, VISUAL and GEN (HOME again or SELECT cycles; HOME+SELECT goes to GEN): TRACKS is six ridgelines with
  each track's level in dB, the four knob-controlled tracks in the knobs' colours; VISUAL is six rings pulsing with hits.
- The wordmark splash; JIANT's signature is removed.
- The other screens keep JIANT's layouts in ACID colours until 0.4 and 0.5.

Before that, 0.2 was the trim, on top of 0.1 (JIANT FM 0.9.18 under a new name, the pipeline proven end to end):

- ANIMAL and WHEEL are retired: stored sounds that used them load as ANALOG, and T3 at power-on is PHASE.
- 4 songs; 128 KiB of flash reserved for breaks (an 8 KiB gap may join later), for breakbeats coming in a later version.
- The voice budget is measured on the FM-1: 12 voices at once on two cores, 8 per track.

The ACID FM features arrive in steps (see `docs/superpowers/specs/2026-10-24-acid-fm-design.md`):

1. Pipeline (0.1)
2. Trim (0.2): engines for IDM, the voice ceiling measured, room made for samples
3. The ACID screen theme (3a in 0.3: look, HOME, splash; 3b and 3c next)
4. BREAK: load breakbeats from the web page, chop and rearrange them
5. Sound: TAPE (wow, flutter, saturation), 303-style accent and slide, new FM and drum voices
6. The song generator: ACID, BRAINDANCE, DRILL'N'BASS, BOC, AMBIENT, GLITCH

The manual still describes JIANT FM's features, which 0.1 keeps; it is rewritten as ACID FM's features land.

## Built on

ACID FM is a fork of **[JIANT FM](https://github.com/juanjiant-bit/JIANT)** by Juan Jiant, itself a fork of
**[Felucca](https://github.com/hugelton/Felucca)** by Hügelton Instruments, with SLOOP's song system. JIANT's own
README is kept in [docs/JIANT-README.md](docs/JIANT-README.md). Every third-party part and its licence is listed in
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
