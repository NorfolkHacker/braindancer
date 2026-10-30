# Braindancer piece 5: TAPE, the 303 acid response, IDM presets, DRUM-X kits (release 0.8)

Date: 2026-10-30. Status: approved in brainstorming; awaiting spec review. Parent specs:
`docs/superpowers/specs/2026-10-24-acid-fm-design.md` (§4.1 acid response, §4.2 TAPE, §5 voices), the UI spec
`docs/superpowers/specs/2026-10-24-acid-fm-ui-design.md` (§0, §4.1). Context: the 0.5.2 overload fix (GEN CPU budget,
the audio safety ladder in `audio.c fx_safety_tick`). Release: Braindancer **0.8**.

User decisions (2026-10-30): all four sound jobs in this piece; TAPE's AGE knob on FX › MASTER (DUCK TIME moves to
FX › MORE), its details as FX › MORE rows; TAPE is the first rung of the safety net under overload.

## 1. TAPE (master effect)

- Position: the master chain, after CLIP / PUNCH / DUCK, before the output.
- **WOW**: slow irregular pitch drift (a smoothed random LFO, 0.3–2 Hz), through a short modulated delay line (≈ 20 ms,
  interpolated reads).
- **FLUTTER**: fast small pitch movement (6–15 Hz), a little different left and right.
- **SAT**: soft tape saturation (the existing soft-clip curve, with gain makeup so the level stays put).
- **HISS**: filtered noise whose level follows the music's level (an envelope follower; no hiss over silence).
- **TONE**: a gentle high-cut (one-pole, tape roll-off).
- **AGE**: one macro 0–100 that sets the five together (0 off; ≈ 50 the RETROBOC sound; 100 a worn cassette). Turning
  a detail row after AGE overrides that detail until AGE is turned again.
- Controls: FX › MASTER becomes CLIP · PUNCH · DUCK · **AGE** (DUCK TIME moves to FX › MORE). FX › MORE gains a
  **TAPE** group: WOW, FLUTTER, SAT, HISS, TONE.
- Off by default (AGE 0 and every detail 0 = bypassed, output bit-identical to 0.7). Saved with the project as new song
  globals (appended, append-only storage; older projects load with TAPE off). Automatable and lockable like DJ FILTER.
- Overload: TAPE is the **first** rung of the safety ladder (before the long-delay rung): it fades out over a few ms when
  the governor is at its floor and the audio is behind, and fades back after the calm the other rungs use (per-rung
  back-off as in 0.5.2). GEN counts TAPE's cost in its CPU budget.

## 2. The 303 acid response (ANALOG)

- Mono and legato modes:
  - **Accent sweep**: an accented step adds level and filter-envelope depth; consecutive accents accumulate a cutoff
    offset (the 303 climb), which decays back when a step without accent plays.
  - **Slide**: a fixed ≈ 60 ms glide into the next note without retriggering the envelopes.
- Poly modes: accent as level + envelope depth; slide ignored (as now).
- No switch: old projects using accents / slides in mono mode get the more 303-like response; nothing else changes.

## 3. Sounds

- **New factory presets**, an IDM set of 16–24 across ANALOG, FM6, PHASE, LOFI and VOICE: squelchy acid basses and
  leads, detuned pads and choirs (RETROBOC), FM bells / glassy keys / metallic basses, dusty wobbly leads. Added after the
  existing presets (stored preset numbers keep naming the same sounds).
- **DRUM-X kits** re-tuned towards 808, 909 and crunchy drum-machine sounds: values only, the kit format unchanged.
- No band names in preset names.

## 4. Limits

- CPU: TAPE is a fixed per-sample cost while on; measured on the device with CPU TEST (TAPE on) and covered by the
  target budget tests (`tests/target_budget.py`): the ISR / TIMER5 / slicer budgets may rise only by TAPE's own cost,
  which is recorded. The acid response is a few operations per step.
- Image: ≈ 28 KB free now; floor 16 KiB free. Presets and kits are the largest cost.
- Storage append-only (new song globals for TAPE); presets appended; kits change values only.
- English only; the naming rule (Braindancer built on JIANT FM and Felucca) kept.

## 5. Testing

- TAPE: with every control 0 the output is bit-identical to 0.7 (golden renders unchanged); a test tone through TAPE
  shows the wow / flutter rate and depth in range, the hiss following the input level (none on silence), TONE's
  roll-off, SAT's level makeup; AGE's mapping; the safety rung fades TAPE out and back (no clicks, no flapping).
- Acid: consecutive accents raise the cutoff step by step and relax after; slides glide in ≈ 60 ms without envelope
  retrigger (mono / legato); poly unchanged.
- Presets / kits load; old preset numbers unchanged; render scenes of FX › MASTER and FX › MORE › TAPE lint-clean.
- Device (0.8): backup, install, status; "make it sound like a worn tape" (no manual); an acid line with accents and
  slides; CPU TEST with TAPE on (late / lost blocks); log; merge and push on the user's go-ahead.
