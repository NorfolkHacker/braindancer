# Braindancer piece 6: one GENERATE button, blends, TAPE in GEN, the AGE retune (release 0.9)

Date: 2026-10-31. Status: approved in brainstorming; awaiting spec review. Parent specs:
`docs/superpowers/specs/2026-10-24-acid-fm-design.md` (the generator), `docs/superpowers/specs/2026-10-30-braindancer-piece5-sound.md`
(§1 TAPE). Research: `docs/braindancer/genre-research.md`. Device findings that drive it:
`docs/braindancer/device-log.md` (0.8) and `docs/superpowers/plans/2026-10-30-braindancer-piece5-deferred.txt`.
Release: Braindancer **0.9**.

User decisions (2026-10-29 and 2026-10-31): GEN loses its genre choice; one press of GENERATE picks a style itself
and often blends two (the COMBINATOR, built in); one GEN page (ENERGY, FORM, COMPLEX, TAPE); TAPE: the style decides
(AUTO), the knob overrides; the last 8 pieces can be brought back; approach A (build on the existing genre templates,
the RANDOM / mutant code and the EXPLORE fitter; no per-role rewrite).

## 1. The GEN page

- Reached as now: HOME taps (TRACKS → VISUAL → GEN), HOME held + SELECT.
- **One page**, four knobs:
  - KNOB 1 **ENERGY** (as now).
  - KNOB 2 **FORM**: LOOP, PARTS, SONG (as now).
  - KNOB 3 **COMPLEX** (as now).
  - KNOB 4 **TAPE**: AUTO (default), OFF, 10..100 in tens.
- **OCT+ = GENERATE**: makes a new piece and plays it, as making a piece does now (UNDO marked first). The panel
  names it: the main style, `+` the partner if blended, the seed: **RETROBOC+AMBIENT 0421**; WILD for an EXPLORE pick.
- **PRESETS** (encoder): steps through the **recent pieces** (§4). Its first position is **NEW** (OCT+ generates);
  each turn back shows the next older piece's name; OCT+ on one rebuilds and plays it.
- **OCT−**: back to TRACKS (as now). **SELECT** does nothing on GEN (one page).
- Gone from the UI: the GENRE and SEED knobs, GEN pages 2 and 3 (DRUMS, BASS, CHORDS, ARP, LEAD, SWING, HUMAN). The
  generator uses their AUTO behaviour (what AUTO does today) for every piece.
- Every user-visible word in plain upper-case English; no band names (RETROBOC is ours and stays). HOME's existing
  hints that say GENERATE stay true.

## 2. GENERATE: picking and blending

- **The pool**: the 10 styles shown today (`GEN_SHOWN`): IDM, GLITCH HOP, BREAKS, DRILL'N'BASS, ACID, ELECTRO, MICRO,
  RETROBOC, AMBIENT, AMB TECHNO, with their screen names and the research tempos (`GEN_SHOWN_TEMPO`).
- **Each press**, from a fresh seed:
  - **≈ 10 %**: **WILD**, the EXPLORE generator (no template).
  - Otherwise a **main style** drawn evenly from the 10, never the same main style as the previous press.
  - Of those, **≈ 45 %** are **blends**: the main style mixed with a **partner** drawn from its compatible list below,
    using the existing mutant code (the main style sets tempo and feel).
- **Compatible partners** (main → partners):

  | Main | Partners |
  | --- | --- |
  | IDM | GLITCH HOP, MICRO, RETROBOC, AMBIENT, DRILL'N'BASS, ELECTRO |
  | GLITCH HOP | IDM, BREAKS, MICRO, ELECTRO |
  | BREAKS | GLITCH HOP, DRILL'N'BASS, ACID, ELECTRO |
  | DRILL'N'BASS | IDM, BREAKS |
  | ACID | BREAKS, ELECTRO, AMB TECHNO |
  | ELECTRO | IDM, GLITCH HOP, BREAKS, ACID |
  | MICRO | IDM, GLITCH HOP, AMB TECHNO |
  | RETROBOC | IDM, AMBIENT |
  | AMBIENT | IDM, RETROBOC, AMB TECHNO |
  | AMB TECHNO | ACID, MICRO, AMBIENT |

- **Identity**: a piece is fully determined by (main style or WILD, partner or none, seed, ENERGY, FORM, COMPLEX, the
  TAPE knob, the device's CPU factor). The same identity always builds the same piece on the same firmware.
- The result is ordinary music in the tracks: edited, saved and played as today.

## 3. TAPE in GEN and the CPU budget

- **AUTO AGE per style**: RETROBOC 60, AMBIENT 40, AMB TECHNO 25, IDM 20, MICRO 15; ACID, ELECTRO, BREAKS, GLITCH HOP,
  DRILL'N'BASS 0. A blend: the main style's value + half the partner's, at most 70. WILD: 0..50 from its seed. The TAPE
  knob on OFF sets 0, on an amount sets that amount, for every piece. GEN writes AGE through the existing AGE macro
  (`tape_age_apply`), so the five details follow it.
- **Every piece is fitted** with the full fitter EXPLORE uses today (`gx_fit_piece` order: shorten the delay, grain
  off, PLAYHEADS, chord notes, long releases, texture, reverb, arp, lead), not only `gen_fit_delay`.
- **TAPE is counted**: `gc_cost` adds TAPE's measured cost (`GC_TAPE1` / `GC_TAPE2`, re-measured if the AGE retune
  changes it) whenever the piece's AGE is above 0. The 0.8 rule "GEN ignores TAPE" is withdrawn.
- **Still over budget after thinning**: on AUTO, GEN lowers AGE step by step (to 0 at the last); an AGE set with the
  knob is kept, and the safety net stays the last resort.
- **Aim**: a generated piece does not need the safety net to pause TAPE (`safety_tape` stays 0 while it plays on the
  device), including the heavy-GEN case of the 0.8 device session.

## 4. Recent pieces

- The last **8** pieces' identities (§2) in RAM, newest first; a ninth drops the oldest. Bringing one back moves it to
  the front and does not duplicate it.
- Cleared at power-off; nothing is written to flash; saved projects are untouched.
- Bringing one back rebuilds it from its identity (same piece), marks UNDO first, and plays it.

## 5. The AGE retune

- A steeper AGE map (`tape_age_map`): at **50** a clearly worn tape (audible wobble, some hiss, a darker tone); at
  **100** properly mangled. AGE 0 stays off, bit-identical to 0.7 / 0.8.
- Measured as on the device (2026-10-31: SINE KEY A3 held, pitch track, non-harmonic noise, energy above 3 kHz): at
  AGE 50 the pitch wobble's standard deviation is at least **2×** AGE 0's on a steady tone, and the non-harmonic
  noise at least **6 dB** above AGE 0's; a host test pins the new map; the user judges 50 and 100 by ear on the device.
- Projects saved in 0.8 with AGE above 0 sound more worn in 0.9 (accepted).

## 6. Limits and compatibility

- Projects, presets and songs: unchanged formats; they load as they were (TAPE's sound at a given AGE as §5).
- GEN's remembered settings: the dropped fields (genre, seed, the page 2/3 amounts) are ignored on load; the plan
  checks how they are stored and keeps the storage append-only.
- The web editor: whatever it shows or sets of GEN follows the new page.
- Budgets: ISR 300, TIMER5 2170, slicer 821 unchanged; the image keeps ≥ 16 KiB free; RAM for §4 a few dozen bytes.
- Fitting every piece changes the piece a given style and seed makes compared with 0.8; the generator's golden values
  are re-recorded.

## 7. Testing

- Host:
  - GENERATE: same identity → same piece; partners only from §2's table; WILD about 10 % over many presses; never the
    same main style twice in a row; the panel name.
  - Budget: over hundreds of seeds and every style, the estimate including TAPE is within budget; AUTO lowers AGE only
    after the thinning; a knob-set AGE is never changed.
  - Recent pieces: 9 presses keep 8; stepping back rebuilds each exactly; a restart clears them.
  - GEN page: the four knobs and their values; OCT+, PRESETS, OCT−, SELECT; GENRE / SEED gone; HOME's hints.
  - AGE map: §5's figures on a host render of a held sine.
  - The full suite green; the budgets hold.
- Device (set up over USB SysEx where a scene is needed, the user presses and listens): about 20 GENERATE presses
  with the console counters read (no lost audio, `safety_tape` 0), recent pieces brought back, a RETROBOC piece
  sounds worn, AGE 50 and 100 judged by ear.

## 8. Docs and release

- The manual's GEN section rewritten (one page, GENERATE, blends, recent pieces, TAPE); the 0.9 news entry first;
  README, the cheat sheets' HTML and the landing page updated. The cheat-sheet PNG / PDF stay pending (no playwright).
- Release **0.9** after the whole-branch review and the device session; merge and push on the user's go-ahead.

## 9. Not in this piece

- A quicker way to reach TAPE / AGE outside GEN (deferred).
- Per-role mixing of styles (approach B), more styles (the research's alternates).
