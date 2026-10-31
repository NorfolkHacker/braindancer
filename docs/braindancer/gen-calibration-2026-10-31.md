# GEN CPU model calibration against the FM-1 (0.8, 439fefd)

Inputs: `build/cal/measured.txt`, `build/cal/cal.log`. Conditions: two cores, CPU TEST stored at 74 %, AGE 0, 6 tracks.
Method: a host harness on a temporary worktree at 439fefd (removed afterwards), built with
`tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests` (main repo's `build/gen`; no generator changed since 0.8). A sweep
probe ran on a scratch copy of the piece6 working tree, with the model constants made runtime knobs. Nothing in the
main working tree was touched except this file.

## 1. Setup and cross-checks

- **CPU TEST as the device stores it.** `ui_menu.c cpt_poll` stores the mean load in % (`cpt_res = 74`) and
  `cpt_cores = 2` (it ran with `mc_two()`). On the host I set `cpt_res = 74; cpt_cores = 2; mc_host_v12 = 1`.
  - `gc_dev` = 74·100/63 = 117, ×1.50 = **175**.
  - `gc_budget` = 1030·100/175 = **588**. This matches the comments and the task-3 report.
- **Identity.** GEN screen defaults throughout (energy 50, form 1, amounts AUTO, complex 50, human 30). RANDOM is
  `gp.genre = 0`; EXPLORE is `GEN_EXPLORE`.
- **Names match the panel.** 4741 → "AMBIENT+AMBIENT 4741" (panel: "AMBIENT+AMB…"). The others:
  - 4493 GLITCH+BREAKS, 0036 AMBIENT, 0965 ACID+IDM, 8794 BREAKS+ACID.
  - 7625 RETROBOC, 2020 AMB TECHNO, 7033 IDM+BREAKS, 1751 AMBTECH+IDM.
  - 1174 BREAKS+IDM, 8790 BREAKS+ELECTRO, 7563 AMBIENT+D'N'BASS, 4459 ELECTRO+D'N'BASS, 1573 BREAKS+D'N'BASS.
- **0.8's state leak doesn't matter here.** 0.8 lets a piece depend on the piece before (fixed in 33ce4d6). I built
  every piece twice: once from power-on, and once chained in the device's order. Every cost and name was identical.
- **The breakdown is exact.** The harness recomputes `gc_cost(1)` term by term, and its sum equals `gc_cost(1)` for
  every piece.

## 2. The data

Measured rates are per 20 s (each primary window is 19.9 s). cpu is the mean of the start and end snapshots. Those two
differ by up to 25 points within one piece (for example 7625: 45 → 68), so a single cpu figure is ±~10.

| id | name | class | late | shed | cpu | 0.8 cost | c/b | main terms |
|---|---|---|---|---|---|---|---|---|
| 1751 | AMBTECH+IDM | BAD | 81 | 165 | 59 | 672 | 1.14 | no FM6; PHASE chord stabs (8×4 notes, gate 42) **ECHO ×2**; CLOUD |
| 1174 | BREAKS+IDM | BAD | 44 | 230 | 50 | 668 | 1.14 | FM6 mono bass 57/64; ANALOG stabs (10×4, gate 43) **ARPN ×2**; SPRING |
| 0036 | AMBIENT | BAD | 25 | 121 | 70 | 836 | 1.42 | FM6 lead; half-rate delay (+63); CLOUD; long tex |
| 8794 | BREAKS+ACID | BAD | 6 | 130 | 47 | 707 | 1.20 | FM6 mono bass; PHASE stabs (9×4, gate 31) **ARPN ×2**; idle VOICE lead (64) |
| 0965 | ACID+IDM | border | 7 | 40 | 43 | 743 | 1.26 | FM6 pad **ARPN ×1**; CLOUD |
| 8790 | BREAKS+ELECTRO | clean* | 9 | 20 | 56 | 660 | 1.12 | ANALOG chords (13×4, gate 100) **ARPN+ECHO** |
| 1573 | BREAKS+D'N'BASS | clean | 4 | 58 | 61 | 651 | 1.11 | ANALOG stabs **ECHO ×1** |
| 7625 | RETROBOC | clean | 2 | 74 | 57 | 828 | 1.41 | FM6 pad + FM6 arp (280 of FM6) |
| 2020 | AMB TECHNO | clean | 1 | 32 | 54 | 845 | 1.44 | FM6 pad + FM6 lead (265); CLOUD |
| 7033 | IDM+BREAKS | clean | 2 | 6 | 63 | 831 | 1.41 | FM6 pad (183); CLOUD |
| 7563 | AMBIENT+D'N'BASS | clean | 1 | 32 | 64 | 692 | 1.18 | no FM6, no chord comps |
| 4459 | ELECTRO+D'N'BASS | clean | 4 | 2 | 60 | 717 | 1.22 | no FM6; PHASE stabs with REL 104, no comps |
| 4741 | AMBIENT+AMBIENT | untimed | ~0 | ~1 | 55 | 865 | 1.47 | FM6 bass + FM6 pad; ARPN ×1; 74 BPM |
| 4493 | GLITCH+BREAKS | untimed | ~10 | ~103 | 58 | 778 | 1.32 | FM6 pad; no chord comps (the window includes the tail of 7848) |
| X7848 | EXPLORE | untimed | ~1 | ~23 | 71 | 583 | 0.99 | already fitted by EXPLORE's fitter |

\* 8790's late rate is 9, over the ≤ 5 line, but it was listed as clean.

- "ECHO / ARPN ×n" counts the chord track's NOTE steps of two or more notes that carry an ECHO or ARP NOTES step
  component. They are placed by `gen_post` when `gs.complex > 30`. The model doesn't see them at all.
- 7848's rate comes from 10 late and 245 shed over 213 s (p1 → p2). Per 20 s that is about 1 late and 23 shed. By the
  counters, 7848 is mild unless it played only a small part of that window.

<details><summary>Per-track breakdown (all 15 pieces)</summary>

```
(on = NOTE steps / LEN, ns = notes, tail = the release term in % of a step, v = voice estimate x100,
 ENG / VOX = the part's GC_ENG and GC_VOX*v/100; "--" = not playing)
== RANDOM 4741 AMBIENT+AMBIENT bpm 74 cost 865 c/b 1.471 CLOUD, delay half rate: base 295 rev 47 dlo 63
 T1 BASS FM6 on 48/64 ns 48 gate 60 tail 828 mono v 100 ENG 79 VOX 26
 T2 PAD FM6 on 2/64 ns 8 gate 100 tail 828 v 400 ENG 79 VOX 104   (ARPN x1)
 T3 ARP ANALOG on 2/12 v 28 ENG 27 VOX 5 | T4 DRUM -- | T5 LEAD PHASE on 3/24 v 99 ENG 45 VOX 14 | T6 TEX NOISE v 136 ENG 60 VOX 21
== EXPLORE 7848 bpm 113 cost 583 c/b 0.991 SPRING: base 295 rev 25
 T1 BASS LOFI v 72 ENG 27 VOX 12 | T2 PAD ANALOG on 2/64 ns 7 DIV 1/8 v 332 ENG 27 VOX 63 | T3 ARP -- | T4 DRUM v 187 ENG 70 VOX 20
 T5 LEAD LOFI DIV 16T v 100 ENG 27 VOX 17 | T6 TEX --
== RANDOM 4493 GLITCH+BREAKS bpm 109 cost 778 c/b 1.323 CLOUD: base 295 rev 47
 T1 BASS ANALOG v 98 ENG 27 VOX 18 | T2 PAD FM6 on 7/64 ns 24 gate 34 tail 1220 v 400 ENG 79 VOX 104 | T3 ARP ANALOG v 32 ENG 27 VOX 6
 T4 DRUM v 200 ENG 70 VOX 22 | T5 LEAD ANALOG v 118 ENG 27 VOX 22 | T6 TEX LOFI v 45 ENG 27 VOX 7
== RANDOM 0036 AMBIENT bpm 78 cost 836 c/b 1.422 CLOUD, delay half rate: base 295 rev 47 dlo 63
 T1 BASS ANALOG v 100 ENG 27 VOX 19 | T2 PAD ANALOG on 2/64 ns 6 v 306 ENG 27 VOX 58 | T3 ARP ANALOG on 33/64 v 46 ENG 27 VOX 8
 T4 DRUM v 178 ENG 70 VOX 19 | T5 LEAD FM6 on 8/64 tail 873 v 188 ENG 79 VOX 48 | T6 TEX ANALOG rel 110 v 117 ENG 27 VOX 22
== RANDOM 0965 ACID+IDM bpm 124 cost 743 c/b 1.264 CLOUD: base 295 rev 47
 T1 BASS PHASE v 64 ENG 45 VOX 9 | T2 PAD FM6 on 2/64 ns 8 tail 1388 v 260 ENG 79 VOX 67 (ARPN x1) | T3 ARP ANALOG v 16 ENG 27 VOX 3
 T4 DRUM v 200 ENG 70 VOX 22 | T5 LEAD -- | T6 TEX NOISE v 122 ENG 60 VOX 19
== RANDOM 8794 BREAKS+ACID bpm 135 cost 707 c/b 1.202 ROOM: base 295
 T1 BASS FM6 on 29/64 mono tail 1512 v 100 ENG 79 VOX 26 | T2 PAD PHASE on 9/64 ns 36 gate 31 v 84 ENG 45 VOX 12 (ARPN x2)
 T3 ARP ANALOG v 28 ENG 27 VOX 5 | T4 DRUM v 200 ENG 70 VOX 22 | T5 LEAD VOICE on 0/64 v 0 ENG 64 VOX 0 | T6 TEX LOFI rel 110 v 208 ENG 27 VOX 35
== RANDOM 7625 RETROBOC bpm 86 cost 828 c/b 1.408 ROOM: base 295
 T1 BASS ANALOG v 22 ENG 27 VOX 4 | T2 PAD FM6 on 1/64 ns 4 v 244 ENG 79 VOX 63 | T3 ARP FM6 on 14/64 v 227 ENG 79 VOX 59
 T4 DRUM v 200 ENG 70 VOX 22 | T5 LEAD ANALOG v 92 ENG 27 VOX 17 | T6 TEX VOICE v 96 ENG 64 VOX 22
== RANDOM 2020 AMB TECHNO bpm 118 cost 845 c/b 1.437 CLOUD: base 295 rev 47
 T1 BASS ANALOG v 63 ENG 27 VOX 11 | T2 PAD FM6 on 2/64 ns 8 v 252 ENG 79 VOX 65 | T3 ARP ANALOG v 28 ENG 27 VOX 5
 T4 DRUM v 150 ENG 70 VOX 16 | T5 LEAD FM6 on 5/64 v 162 ENG 79 VOX 42 | T6 TEX NOISE v 141 ENG 60 VOX 22
== RANDOM 7033 IDM+BREAKS bpm 127 cost 831 c/b 1.413 CLOUD: base 295 rev 47
 T1 BASS PHASE on 44/64 v 44 ENG 45 VOX 6 | T2 PAD FM6 on 3/64 ns 11 v 400 ENG 79 VOX 104 | T3 ARP PHASE on 54/64 v 48 ENG 45 VOX 7
 T4 DRUM v 200 ENG 70 VOX 22 | T5 LEAD ANALOG mono v 75 ENG 27 VOX 14 | T6 TEX NOISE v 67 ENG 60 VOX 10
== RANDOM 1751 AMBTECH+IDM bpm 118 cost 672 c/b 1.143 CLOUD: base 295 rev 47
 T1 BASS PHASE on 20/64 gate 100 mono v 100 ENG 45 VOX 15 | T2 PAD PHASE on 8/64 ns 32 gate 42 v 72 ENG 45 VOX 10 (ECHO x2)
 T3 ARP ANALOG v 45 ENG 27 VOX 8 | T4 DRUM v 200 ENG 70 VOX 22 | T5 LEAD ANALOG v 75 ENG 27 VOX 14 | T6 TEX ANALOG rel 103 v 109 ENG 27 VOX 20
== RANDOM 1174 BREAKS+IDM bpm 121 cost 668 c/b 1.136 SPRING: base 295 rev 25
 T1 BASS FM6 on 57/64 gate 40 mono v 100 ENG 79 VOX 26 | T2 PAD ANALOG on 10/64 ns 40 gate 43 v 96 ENG 27 VOX 18 (ARPN x2)
 T3 ARP ANALOG v 41 ENG 27 VOX 7 | T4 DRUM v 200 ENG 70 VOX 22 | T5 LEAD LOFI mono v 83 ENG 27 VOX 14 | T6 TEX LOFI v 25 ENG 27 VOX 4
== RANDOM 8790 BREAKS+ELECTRO bpm 130 cost 660 c/b 1.122 SPRING: base 295 rev 25
 T1 BASS ANALOG on 53/64 v 37 ENG 27 VOX 7 | T2 PAD ANALOG on 13/64 ns 52 gate 100 v 232 ENG 27 VOX 44 (ARPN + ECHO)
 T3 ARP ANALOG v 32 ENG 27 VOX 6 | T4 DRUM v 178 ENG 70 VOX 19 | T5 LEAD ANALOG mono v 85 ENG 27 VOX 16 | T6 TEX NOISE v 64 ENG 60 VOX 10
== RANDOM 7563 AMBIENT+D'N'BASS bpm 122 cost 692 c/b 1.177 SHIMMER: base 295 rev 32
 T1 BASS PHASE on 52/64 v 52 ENG 45 VOX 7 | T2 PAD ANALOG on 2/64 ns 8 rel 101 v 164 ENG 27 VOX 31 | T3 ARP ANALOG v 46 ENG 27 VOX 8
 T4 DRUM v 121 ENG 70 VOX 13 | T5 LEAD PHASE v 87 ENG 45 VOX 13 | T6 TEX NOISE v 123 ENG 60 VOX 19
== RANDOM 4459 ELECTRO+D'N'BASS bpm 146 cost 717 c/b 1.219 SHIMMER: base 295 rev 32
 T1 BASS ANALOG v 24 ENG 27 VOX 4 | T2 PAD PHASE on 7/64 ns 28 gate 32 rel 104 v 412 ENG 45 VOX 61 | T3 ARP LOFI mono v 28 ENG 27 VOX 4
 T4 DRUM v 200 ENG 70 VOX 22 | T5 LEAD ANALOG mono v 100 ENG 27 VOX 19 | T6 TEX NOISE v 151 ENG 60 VOX 24
== RANDOM 1573 BREAKS+D'N'BASS bpm 151 cost 651 c/b 1.107 SHIMMER: base 295 rev 32
 T1 BASS ANALOG v 21 ENG 27 VOX 3 | T2 PAD ANALOG on 8/64 ns 32 gate 44 v 88 ENG 27 VOX 16 (ECHO x1) | T3 ARP LOFI mono v 67 ENG 27 VOX 11
 T4 DRUM v 200 ENG 70 VOX 22 | T5 LEAD ANALOG mono v 97 ENG 27 VOX 18 | T6 TEX NOISE v 103 ENG 60 VOX 16
(no piece had the grain delay or PLAYHEADS)
```
</details>

## 3. Does cost / budget order the pieces the way the device does? **No.** On the 12 primary rows it is inverted.

### The ordering

The three cleanest-sounding heavy pieces are near the top of the model's ranking: 2020 (1.44), 7033 (1.41) and 7625 (1.41).
The two worst pieces are near the bottom: 1751 (1.14) and 1174 (1.14). No threshold on c/b separates the bad pieces from
the clean ones.

Spearman rank correlation on the 12 primary rows (0036 onward):

| quantity | vs late | vs shed | vs late + shed/10 | vs device cpu |
|---|---|---|---|---|
| 0.8 cost (≡ c/b) | −0.39 | −0.22 | −0.38 | +0.10 |
| cost without the FM6 parts | +0.14 | −0.33 | −0.03 | **+0.66** |
| host simulation: mean voices | −0.30 | −0.47 | −0.43 | **+0.71** |
| host simulation: mean render time | −0.37 | −0.31 | −0.46 | **+0.70** |
| PAD voice estimate | −0.32 | −0.68 | −0.57 | +0.26 |
| steps where a chord of 3+ notes starts with 2+ other notes | +0.48 | +0.17 | +0.38 | −0.17 |

### Two separate failures

**(i) The model's average is off for these pieces, mostly because of FM6.**
- A least-squares fit of the device's cpu % against the model's terms gives:
  `cpu ≈ 0.0869·(cost − FM6 parts) + 0.0156·(FM6 parts)`.
  - RMS error 5.0 points, against 8.2 for `k·cost`.
  - Bootstrap over the 12 pieces: the FM6 weight relative to the rest is 0.19 (80 % CI −0.03..0.33, 95 % CI −0.23..0.42).
  - So on the device, an FM6 part costs well under half of what the model charges.
  - The host confirms that the voices are really there: the FM6 pads hold 2.4–3.5 voices against estimates of 2.4–4.0.
    So the issue is the per-part and per-voice cost on the device, not the voice estimate.
  - Plausible reason (not verified): flash/XIP cache behaviour differs per engine. `build.py` notes that the render's
    flash layout moved the device from fine to 94 % on 0.9.9 → 0.9.10 with identical host counts.
- Without FM6, the slope gives the device factor for RANDOM/genre pieces: 0.0869 / 0.063 = ×1.38 over the model.
  - That is ×1.17 over the CPU TEST's own ratio (74/63 = 1.175).
  - Bootstrap: median 118, 80 % CI 113..121, 95 % CI 110..123.
  - `GC_GEN_X100` says ×1.50.
- EXPLORE pieces really are heavier per model unit:
  - EXPLORE 9641 runs at 1.45× the CPU TEST's ratio, 3423 at 1.55×, and 7848 at 1.64× (71 % at a model 583).
  - The ×1.50 factor was fitted on EXPLORE only (the 0.5.2 comment), and the task-3 report already suspected it doesn't
    carry over to the styles. The data confirms that.
  - Likely causes, unmodelled: MUTATE, own loops and DIVs, PLAYHEADS.

**(ii) late / shed are peak effects, and the model is an average-load model.**
- 1751 (59 % cpu, 81 late) and 1174 (50 %, 44 late) have lower averages than 7033 (63 %, 2 late) and 7563 (64 %, 1 late).
- On the device, `audio_late` is a single half over 100 %, and a shed is two halves in a row over 80 % (`audio.c
  shed_check`). Neither tracks the smoothed `cpu_pct`.
- The CPU TEST song shows the same thing:
  - Its average, 74 %, is only 4–11 points above the clean GEN pieces.
  - But it made 387 late and 996 shed in ~20–30 s, about 100× the clean pieces' rate.
  - It is the one piece made of 4-note chords with ECHO ×8 / ARP components on every step.
  - Model vs measured for the CPU TEST song: by construction it is 1000 thousandths. My forced-role re-estimate is 1101,
    because the components aren't counted. Device 74 % = ×1.175, so the average matches the model's definition. The
    late/shed rate is far beyond what that average implies.

### What explains the bad pieces?

**0036 is explained by its average.**
- It has the highest measured cpu (70 %) and is the heaviest once FM6 is reweighted.
- Its delay runs at half rate (GC_DLO +63), which the governor can't shed.
- With CLOUD and FM6 on top, it is simply over the 65 % line.

**1751, 1174 and 8794 are not explained by any average term.** One feature separates them, found after the fact:
- **Step components (ECHO or ARP NOTES) on the chord track's chord steps.** `gen_post` adds them at COMPLEX > 30.
- The pieces that have them:
  - 1751 (ECHO ×2), 1174 (ARPN ×2), 8794 (ARPN ×2): bad.
  - 8790 (ARPN + ECHO): late 9.
  - 0965 (ARPN): late 7.
  - 1573 (ECHO): late 4, shed 58.
  - 4741 (ARPN, 74 BPM): untimed, apparently clean.
- Of the 6 primary pieces with them, 3 are bad. The other 3 are the three noisiest of the rest: 0965 (late 7),
  8790 (late 9) and 1573 (shed 58). Of the 6 without them, only 0036 is bad, and it is explained by its average; the
  other 5 are the cleanest (late ≤ 4, and 4 of them with shed ≤ 32).
- Fisher exact test:
  - with the labels as given (4 bad vs 8 not): p ≈ 0.27;
  - with those three counted as borderline: p ≈ 0.008, but I chose that grouping after seeing the data.
  - I looked at about 30 features, so after correction even the second is roughly p ≈ 0.2.
- It is a lead, not a finding.
- The host shows the model is blind to these chords. Mean sounding voices on the chord track, host vs the model's
  estimate:

  | piece | host | model |
  |---|---|---|
  | 1751 | 2.20 | 0.72 |
  | 1174 | 2.40 | 0.96 |
  | 8794 | 2.38 | 0.84 |
  | 1573 | 2.37 | 0.88 |
  | 8790 | 3.58 | 2.32 |

  Chords without components are estimated well: 0036 3.08 vs 3.06, 2020 2.67 vs 2.52, 7625 2.44 vs 2.44, 4459 4.29 vs
  4.12. (7563 is the exception: 3.64 vs 1.64.)
- The average voices this adds (+1.5 voices, about +25 cost) are far too small to explain 80 late. The plausible
  mechanism is peaks:
  - an echoed or arpeggiated 4-note stab fires a burst of note-ons on top of a full voice pool (12 shared);
  - that brings steals, fading voices and attack work all in one half.
  - `voices_given_up` per 20 s was highest for exactly these three: 1174 299, 8794 240, 1751 218.
- I could not reproduce the peaks on the host. The host's per-half render time doesn't single them out. On x86 they
  don't spike; the device's flash-cache behaviour does not show up on the host.
- The gaps between windows agree that 1751 and 1174 stay bad: after 1751, +69 late in 21 s; after 1174, +25.

## 4. Proposed recalibration

Smallest set that orders the 12 primary rows correctly. Steps 1 and 2 are the calibration proper. Step 3 is the
speculative peak term.

### 1. Split the GEN factor into styles and WILD

| | now | proposed |
|---|---|---|
| RANDOM / genres / blends, two cores | `GC_GEN_X100` 150 | **118** (`GC_STY_X100`) |
| WILD | 150 | **155** (mean of the three EXPLORE device ratios 1.45 / 1.55 / 1.64) |

- Budget at CPU TEST 74 %, two cores: styles 588 → **751**, WILD 588 → **572**.
- With the CPU TEST unset, scale `GC_DEV2_X100` 190 by the same ratios: styles about **150** (budget 686–691), WILD about
  **196** (525).
- One core is unchanged. There is no one-core data.
- In 0.9 the identity stores `dev` (`gid_now` → `gc_dev()`) before the style is picked. So the split belongs where
  `gcf.dev` is set in `gen_make`, or `gc_dev()` needs to know the style.

### 2. FM6 halved

- `GC_ENG[ENGI_FM6]` 79 → **40**, `GC_VOX[ENGI_FM6]` 26 → **13**.
- Index 1 (DIGITAL, mapped as FM6) likewise.
- The fit says ×0.19, so halving is the cautious end.
- With 1 and 2, the model's average lands within ~±5 points of the device on most pieces. Predicted vs measured:
  7033 64.6/63, 0036 67.5/70, 1751 58.7/59. The misses are 0965 (58.5/43) and 8794 (57.2/46.5).
- Only 0036 comes out over (773 / 751).

### 3. The chord-component term, plus a fitter rung (speculative)

- `GC_CCOMP` = **70** for each chord-track NOTE step with two or more notes that carries ECHO or ARP NOTES (in `comp`
  or `comp2`).
- A new **first** rung in `gx_fit_piece`: over the budget → clear those components (the chord's step components only).
  That costs no part and no sound, so a piece that is over only because of them gets the lightest possible thinning.
- K = 70, because 1174 needs at least 68 (616 + 2K > 751).

### Result on the measured pieces (budget 751; WILD 572)

| id | class | c/b after 1+2 | after 1+2+3 | outcome |
|---|---|---|---|---|
| 1751 | BAD | 0.89 | **1.08** | over → chord comps cleared |
| 0036 | BAD | **1.03** | **1.03** | over → thinned (delay / texture …) |
| 8794 | BAD | 0.87 | **1.06** | over → comps cleared |
| 1174 | BAD | 0.82 | **1.01** | over → comps cleared |
| 8790 | clean* | 0.88 | 1.07 | over → comps cleared only (light thinning) |
| 0965 | border | 0.89 | 0.99 | within |
| 7033 | clean | 0.99 | 0.99 | within (the tightest: 740) |
| 4459, 1573, 2020 | clean | 0.95, 0.87, 0.95 | 0.95, 0.96, 0.95 | within |
| 7563, 7625 | clean | 0.92, 0.92 | 0.92, 0.92 | within |
| 4741 | untimed | 0.96 | 1.05 | comps cleared (apparently clean on the device) |
| 4493 | untimed (~10 late) | 0.91 | 0.91 | within: not caught |
| X7848 | "bad" | 1.02 (583 / 572) | 1.02 | over by 11 → thinned |

- Requirement (a) is met: every clean piece is within budget, or (8790) loses only its two chord components.
- Requirement (b) is met for all five listed bad pieces, but some margins are thin:
  - 1174 is over by 5 and 7033 is under by 11, a band of about 1.5 %;
  - 7848 is over by 11, and its own counters (≈1 late / 23 shed per 20 s) don't clearly show it as bad.
- **Without step 3**, (b) holds only for 0036 and 7848. 1751, 1174 and 8794 fit, and the safety net is left to handle
  them, as on 0.8.

## 5. Projected 600-piece sweep (the task-3 probe, rebuilt)

The probe is a copy of the gen_test block "(0.9) every piece fitted with TAPE counted". It ran on the piece6 working
tree's `gen.c`, with knobs for the factor, FM6, the component term and TAPE.
- Run with 0.8's numbers, it reproduces the task-3 report exactly: 377 / 600 over, 366 AGE cuts.
- "Lost" means the part was active before fitting and inactive after. I checked that without the fitter no part
  changes: 0 / 600.

| quarter (budget styles / WILD) | 0.8 model: over · tex-or-arp lost · AGE cut | proposal 1+2+3: over · tex-or-arp lost · AGE cut · chord comps cleared |
|---|---|---|
| one core, unset (326) | 150 · 149 · 92 | 150 · 149 · 92 · 52 (unchanged: no data) |
| two cores, unset (691 / 525) | 52 · 150 · 100 | **1** · 120 · **29** · 45 |
| one core, 74 % (490) | 147 · 147 · 82 | 147 · 147 · 82 · 44 (unchanged) |
| two cores, 74 % (751 / 572) | 28 · 144 · 92 | **1** · 94 · **13** · 39 |
| **all 600** | **377 · 590 · 366** | **299 · 510 · 216** |
| **two cores only (300)** | **80 · 294 · 192** | **2 · 214 · 42** |

### Two cores (the measured configuration)

- Over budget: 80 → 2 (one WILD piece each).
- AUTO cutting AGE: 64 % → 14 %.
- Texture or arp lost: still 214 / 300. **This is TAPE, not calibration.**
  - `GC_TAPE2` = 169 is charged whenever AGE > 0, and the fitter thins the music before it lowers AGE.
  - With TAPE's cost set to 0, texture-or-arp loss at two cores, 74 % falls to 17 / 150.
  - TAPE at 55 % (`GC_TAPE2` ≈ 93): 72 / 150, with AGE cut on 10.
- The only device datum on TAPE is in measured.txt: a GEN piece at AGE 0 / 50 measured cpu 42 / 50, so +8 points.
  - At the styles factor that is about 92 model units, about half of 169.
  - This is one snapshot pair at ±10-point noise, so it doesn't support a recalibration on its own. It is worth one
    targeted measurement before Task 3's ladder order is accepted.

### One core

- Still essentially all over budget. The one-core budget (`714·100/dev − 120`) was not exercised by any measurement here.
- Note the sweep's "one core, 74 %" quarter keeps a CPU TEST stored at 2 cores (`gen_reset` sets `cpt_cores = 2`).
  So its budget 490 is 0.8's two-core 117 applied to the one-core formula, not a real one-core calibration.

### Without the component term (steps 1+2 only)

- Identical over / lost / AGE figures, and no components cleared.
- So step 3 costs the sweep essentially nothing: it only clears components on about 13–30 % of two-core pieces.

## 6. How strong is the evidence

- **12 timed points** (0036 onward, 19.9 s each), plus 3 untimed. One device, one session, one CPU TEST value, AGE 0.
  The counters are cumulative and read by hand-timed snapshots. The cpu snapshots within one piece differ by up to 25
  points, and form-1 pieces change sections while playing.
- **Strong:** cost/budget does not rank the device's badness. It is negatively correlated (Spearman −0.38) and cannot be
  fixed by any margin or scale. That doesn't need precise numbers: the three FM6-heavy clean pieces sit above the two
  worst pieces under 0.8's model.
- **Moderate:**
  - The styles factor ≈ ×1.18. Bootstrap 95 % CI 110..123, but the bootstrap ignores the snapshot noise.
  - FM6 overweighted: the direction is clear, the size (0.2–0.5×) is not.
- **Weak / speculative:**
  - The chord-component term. It is a post-hoc feature among ~30 tried, explains 3 bad pieces, and its mechanism (peaks)
    could not be reproduced on the host.
  - K = 70 is set by a single piece (1174) with a 1 % margin.
  - WILD ×1.55 and 7848's "bad" label rest on three EXPLORE ratios. 7848's own rate is mild.
  - TAPE ~half of 169 rests on one snapshot pair.
- **What would settle it:**
  - Replay 1751, 1174 and 8794 with the chord track's step components removed (SEQ > COMP off on T2). If late/shed
    collapse, step 3 is right and K can be fitted.
  - 2–3 FM6-heavy clean pieces at 40 s each.
  - One GEN piece at AGE 0 vs 50, with 20 s windows.
  - Repeat each window twice to measure the device's run-to-run spread.

## 7. Cleanup

- Worktree `/home/norfolkhacker/synth/mvave-cal08` removed (`git worktree remove --force`, `git worktree prune`).
- `/tmp/cal08` deleted.
- Harness and sweep sources were not kept; they're described above. The harness is the gc_cost loop copied term by
  term, plus `mix_block` playback with per-track voice counts. The sweep is gen_test's TAPE sweep block with the knobs
  `cal_sty_x100`, `cal_wild_x100`, `cal_fm6_eng`, `cal_fm6_vox`, `cal_ccomp` with a rung, and `cal_tape_pct`.
