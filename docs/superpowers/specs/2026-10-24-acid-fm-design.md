# ACID FM — design

Date: 2026-10-24. Status: approved in brainstorming, awaiting spec review.

## 1. What it is

ACID FM is an alternative firmware for the **M-VAVE FM-1**, made for writing IDM / braindance (Aphex Twin,
Boards of Canada, Rephlex) on the device alone: virtual analog, FM, chopped breakbeats loaded by the user, and an
on-device generator that writes whole songs. It is a fork of **JIANT FM** (`juanjiant-bit/JIANT`, itself a fork of
Felucca by Hügelton Instruments, with SLOOP's song system), restyled in the acid theme of SidAcid.

Licence: **GPL-3.0-only**, as JIANT and Felucca. JIANT's and Felucca's copyright lines stay; files we change gain a
"Portions Copyright (C) 2026 NorfolkHacker" line, files that are mostly ours a "Copyright (C) 2026 NorfolkHacker".
LICENSING.md is updated to list ACID FM's own parts.

## 2. Hardware limits (fixed; the firmware replaces JIANT in the same space)

| Resource | Size | Notes |
|---|---|---|
| CPU | JieLi AC791N, 2 cores, 360 MHz (320 option) | pi32v2; JieLi clang 4.0.1 only, so the firmware is C |
| Flash | 1 MiB (JEDEC 0x856014) | app area 0x4000..0x93000 (~572 KiB, XIP); data above it |
| RAM | 512 KiB (0x01C00000..0x01C80000) | `.data/.bss` 96 KiB, the pool ~320 KiB, RAM code ~25 KiB, stacks |
| Screen | colour LCD | JIANT's `lcd.c` / `gfx.c` |
| USB | Audio + MIDI + CDC | Web MIDI installs and editing, USB audio out |

Every design choice below stays inside these. Nothing may touch the ROM bootloader or the head of the flash
(SPL, chip key).

## 3. Repository and pipeline

- Repo: `~/synth/mvavefirmware`, a clone of JIANT with full history. Remotes: `upstream` = JIANT (merges of JIANT
  fixes stay possible), `origin` = `github.com/NorfolkHacker/acid-fm` (**public**: GitHub Pages on a free account and
  GPL-3.0 both require it). The Neologue repo ignores the folder.
- Build: JIANT's `build.sh` / `tools/build.py`, natively on Linux x86-64, with the JieLi toolchain
  (`tools/get_toolchain.sh`, `~/.jieli/toolchain`) and the AC79 SDK (`~/fw-AC79_AIoT_SDK`, tag
  `AC79NN_SDK_V1.2.1_2023-12-13`).
- Identity: product name **ACID FM**; release packages `acid-fm-X.Y.fwsc`; version string `vX.Y`; the USB product
  string `ACID FM`. Versions start at 0.1.
- GitHub Pages, `https://norfolkhacker.github.io/acid-fm/`, built and deployed by GitHub Actions on every push to
  `main` (adapted from JIANT's `.github/workflows/emulator.yml`): landing page, Web MIDI installer (Chrome / Edge),
  browser emulator (Emscripten build of the firmware sources), break uploader (§6.2), manual.
- JIANT's Android workflow is removed.

## 4. Engines

| Engine | Fate | Changes |
|---|---|---|
| ANALOG (VA) | kept | 303-style response to the existing step flags `SF_ACCENT` / `SF_SLIDE` (§4.1) |
| FM6 (6-op FM, msfa) | kept | factory patches re-voiced: warm EPs, bells, glassy pads, metallic basses |
| DRUM-X | kept | kits re-tuned towards 808 / 909 / drum-machine sounds |
| LOFI (bytebeat, crush) | kept | — |
| PHASE (CZ-style) | kept | — |
| NOISE | kept | — |
| VOICE | **slimmed** | formant / vowel pads and choirs only; the CMU dictionary, lyrics and syllable sequencing are removed. If what remains costs more than 16 KiB of flash, the user is asked before cutting further |
| ANIMAL | **removed** | number 14 stays reserved (stores name engines by number) |
| WHEEL | **removed** | number 7 stays reserved |
| BREAK | **new** | §6; takes a new number (15), appended |

Removed engines follow JIANT's existing retirement pattern (`eng_gone()`, `ENG_GONE`): their numbers are reserved,
a stored sound that names one loads as ANALOG's first preset.

### 4.1 Acid response (ANALOG)

- Accent (`SF_ACCENT`): raises level and filter envelope depth; consecutive accents build the cutoff up (the 303's
  accent sweep), decaying back when accents stop.
- Slide (`SF_SLIDE`): the next note glides at a fixed time (about 60 ms) without retriggering the envelopes.
- Applies in ANALOG's mono modes; poly modes ignore slide and keep accent as level + envelope depth.

### 4.2 TAPE (master effect, new)

On the master, after JIANT's master chain, before the output: **WOW** (slow pitch drift, 0.3–2 Hz), **FLUTTER**
(fast, 6–15 Hz), **SAT** (soft saturation), **HISS** (filtered noise, gated by the master level), and **AGE**, one
macro that sets all four together. Off by default except in the generator's BOC style. A short modulated delay line
(about 20 ms) does the pitch effects. Saved with the project, automatable like JIANT's DJ FILTER.

## 5. Voices

- `NVOICE` goes from 8 to **6**: the melodic engines' shared voice budget.
- DRUM-X and BREAK no longer take voices from that budget: each track running one has its own fixed voices
  (DRUM-X as JIANT plays it; BREAK two, §6.3).
- JIANT's averaged overload guard stays as the safety net.
- `NVOICE` stays one constant; raising it later is a measured decision (§9).

## 6. BREAK sampler

### 6.1 Storage

- One sample area in flash. Felucca kept user samples in 0xA0000..0xDBFFF (240 KiB); JIANT gave that region to its
  songs (`storage.c`). Piece 2 re-lays the data region: the app shrinks by what §4 and §8 remove and the data region
  starts lower; songs keep the space they need (measured); the rest is the sample area. **Target: at least 240 KiB.**
  If the measured layout gives less, the user chooses between fewer song slots and less break space. The final
  bounds go into `fm1_flash.h`; the build fails if the app grows into them.
- Up to **8 breaks**. Format: mono **IMA ADPCM, 4 bits**, at 32000, 22050, 16000 or 11025 Hz (≈ 16 / 11 / 8 /
  5.5 KB a second).
- Each break has a header: name (8 chars), sample rate, length in samples, original BPM, bars, up to **32 slice
  markers** (sample offsets), and for each marker the ADPCM decoder state (predictor, step index) at that point, so
  a slice starts at once with no decoding from the start. Header CRC-16/XMODEM, as the rest of the flash records.
- Playback reads straight from the flash (XIP / the data window), no copy in RAM.
- Writes are atomic per break: a break is written, read back and checked, then its directory entry is committed. A
  failed or cut-off upload leaves the previous breaks as they were.

### 6.2 Upload (web, on the Pages site)

- Accepts WAV (any rate, mono or stereo, 8/16/24-bit, float). Mixes to mono, resamples with a proper low-pass to the
  chosen rate, normalises, encodes ADPCM in the browser.
- Waveform view with slice markers: equal slices (4 / 8 / 16 / 32) or detected transients; markers can be dragged,
  added and removed (SidAcid's slice editor as the model).
- BPM: guessed from length and bars (bars default 1 or 2), editable.
- Quality choice per break and a **storage meter** of the whole area. A break that does not fit is never sent: the
  page offers a lower rate, trimming, or deleting another break.
- Transfer: Felucca's sample SysEx commands restored (`SMP_BEGIN` / `SMP_WRITE` / `SMP_END` / `SMP_ERASE` /
  `SMP_INFO`, ids 11–15 in `web/EDITOR_PROTOCOL.md`), extended to carry the header and markers; documented in the
  protocol file.
- The same page lists, renames, deletes and downloads (decoded to WAV) the breaks on the device.

### 6.3 The BREAK engine

- Any track can run it. A step's note picks the slice: C3 = slice 1, C#3 = slice 2 … (up to 32).
- Per step, through JIANT's existing step data: pitch (the note's octave offset and a per-step tune), **reverse**,
  **ratchet / stutter** (x2..x8 within the step), gate, chance.
- Track parameters: BREAK (which of the 8), OFFSET (shifts every slice index), PITCH (varispeed), DECAY, CHOKE
  (on: a new slice cuts the last; off: up to two overlap), CRUSH (bit depth and sample-rate reduction), and JIANT's
  shared filter and envelopes.
- Tempo: slices are triggered on the song's grid, so a chopped break follows any BPM. **FIT** (per track, on/off)
  re-pitches the break by song BPM / break BPM. No time-stretching (too costly here).
- Decoding: ADPCM to 16-bit with linear interpolation for pitch; at most two BREAK voices per track; their cost is
  counted by the overload guard.

### 6.4 Rearranging on the device

One-press actions on a BREAK track, each one undo step, also used by the generator:

- **CHOP**: re-orders slices, keeping the downbeats' slices on the downbeats.
- **STUTTER**: adds ratchets and repeated slices.
- **REVERSE**: sprinkles reversed slices.
- **EDITS**: a small library of classic jungle / drill'n'bass edit patterns, applied to the break's slices.
- JIANT's **MUTATE** works on BREAK tracks as on the others.

## 7. Generator (on the device)

Replaces JIANT's GEN (HOME + SELECT, as in JIANT). Pure C, deterministic.

- **Seeded as songgen:** one seed draws every value in a fixed order; any value the user sets (STYLE, BPM, KEY,
  SCALE, BARS, DENSITY) replaces only its own, so setting one never changes the others. Same seed + same settings
  + same loaded breaks = the same song.
- **Styles** (JIANT's 43 genres are removed):

  | Style | BPM | Character |
  |---|---|---|
  | ACID | 120–135 | ANALOG 303 lines with slide / accent, 808/909 DRUM-X |
  | BRAINDANCE | 125–150 | rhythm switches, squelchy bass, playful leads, breaks |
  | DRILL'N'BASS | 160–180 | heavy BREAK chops, ratchets, reverses |
  | BOC | 85–100 | detuned pads (ANALOG / FM6), VOICE choirs, TAPE on, sparse dusty breaks |
  | AMBIENT | 85–92 | slow FM6 / ANALOG pads, soft beats |
  | GLITCH | 90–170 | odd metres, polyrhythm, LOFI bytebeat |
  | EXPLORE | any | JIANT's rule-breaking mode, kept |

- **Arrangement:** intro → build → main A → breakdown → main B → outro, 32–256 bars (rounded to 8), written into
  the song chain (JIANT's, from SLOOP). Tracks enter and leave per section; parts vary between sections by mutation.
- **Breaks:** if any are loaded, the drum role uses one (chosen by the seed) with generated chops (§6.4); otherwise
  DRUM-X.
- Respects the 6-voice budget when it assigns poly parts.
- The result is an ordinary project: playable, editable, savable. It gets a name from the seed (adjective + noun,
  as songgen).

## 8. Acid UI

- One skin, **ACID**, replaces COLORS, HUD and AIR (their code and tables are removed). Menu > SCREEN > SKIN is
  removed with them.
- Palette (SidAcid `theme.rs`): background `#1A1C20` / `#141518`, lines `#303338`, ACID `#B8F53A` (primary),
  ACID LIGHT `#DCFA9D`, VIOLET `#A64BFF` (secondary), VIOLET LIGHT `#D3A5FF`, ACCENT `#FF503C` (accent steps,
  warnings), SLIDE `#4CB8FF` (slides), QUEUED `#FFC83C` (queued / pending), knob body `#3A3E44`, knob track `#555A60`.
- HOME: a music-reactive, colour-cycling scope / kaleidoscope in the palette (Llamasoft style) replaces JIANT's
  beings. Engine pages: a plain scope or waveform replaces each engine's creature (the creature code is removed).
- JIANT's navigation, page structure and fonts stay.
- The Pages site and the manual use the same palette.

## 9. Testing and safety

- JIANT's host suite (`tests/run_tests.sh`), with the golden render hashes and CPU baselines updated only for
  intended sound changes (`GOLDEN_UPDATE=1`, `BUDGET_UPDATE=1`).
- New host tests:
  - ADPCM: the firmware decoder against the web encoder on shared vectors (bit-exact).
  - Sample store: fill to the limit, refuse an oversize break, survive a cut-off upload, round-trip header and
    markers.
  - BREAK engine: slice starts mid-break decode identically to decoding from the start; reverse, ratchet, choke.
  - Generator: same seed + settings → same song hash; every style within its BPM range and the voice budget.
  - Voices: never more than `NVOICE` melodic voices; DRUM-X / BREAK never steal them.
  - UI: every screen in the ACID skin rendered and checked for clipped or overlapping text (JIANT's existing check).
  - Build: the flash budget check (app below the sample area) and RAM-code reach (JIANT's existing check).
- CPU: measured on the device after each piece (JIANT's MENU > ABOUT / profiler). Target: under 60 % in the
  heaviest generated song. If 6 voices measure cheap, raising `NVOICE` is proposed to the user, not done silently.
- Install safety: before the first install the full flash is backed up (JIANT's web backup or Quixotic7's
  MvaveFM1Unbricker). Installs write only the app area. Way back: `~/fm1_rescue.py` (mask-ROM bootloader) or the
  official V15. Every build is played in the browser emulator before it goes on the device.

## 10. Build order

One spec → plan → implementation cycle per piece:

1. **Pipeline:** fork, ACID FM identity, native Linux build, tests passing, Pages site on `acid-fm`, a proven
   install of the rebranded build on the user's FM-1 and a proven rollback.
2. **Trim:** remove ANIMAL and WHEEL, slim VOICE, remove the 43 genres' tables, `NVOICE` 6; measure freed flash,
   RAM and CPU; re-lay the flash data region and set the sample area's final bounds (§6.1). (The skins and
   creatures go in piece 3, so the final bounds are confirmed after it.)
3. **ACID UI** (§8).
4. **BREAK sampler and uploader** (§6).
5. **Sound:** TAPE, the acid response, FM6 and DRUM-X re-voicing (§4).
6. **Generator** (§7).

## 11. Out of scope

- Time-stretching breaks; recording samples on the device.
- A web-side song generator.
- More than 6 tracks.
- Keeping JIANT's skins, creatures, ANIMAL, WHEEL or the VOICE dictionary.
