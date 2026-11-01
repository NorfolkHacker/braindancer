# GEN CPU model, re-fitted on the FM-1 (Braindancer 0.9.1)

## 1. Setup

- Firmware: `0.9.1-cal` (package `braindancer-0.9.1-cal.fwsc`), built on the clean HEAD f3f96f4; the rows are committed in
  876b413 (`gen-calibration-0.9.1.csv`, device log in `device-log.md`).
- 2 CORES, TRACKS 6, CPU TEST ratio 117 (budget 746 a style's piece, 569 WILD's under 0.9's constants), NEW PROJECT, GEN
  defaults (ENERGY 50, FORM PARTS, COMPLEX 50, TAPE AUTO). Each window is 20 s of a GENERATE press played.
- A row is **late** when the FM-1 played it with more than 5 late halves per 20 s (`late20 > 5`). No audio was lost and no
  clicks were heard on any piece.

## 2. The data

- `docs/braindancer/gen-cal-rows.csv`: 0.8's 15 pieces (the 0.8 calibration's breakdown) and the 20 counted pieces of this
  session (G01..G21 without G11, whose window began on the previous piece; the AGE-set windows are not fitted). 21 of the 35 rows
  are late (13 of the 20 new ones).
- The 0.9.1 rows' parts (engine and voices), the fixed term, TAPE and the chord components are **rebuilt on the host** from each
  piece's identity (`tests/gen_terms.c`, `tools/gen_cal.py ids / rows`), not read from the device.
- **The rebuild is approximate.** Only 4 of the 20 pieces reproduce the device's `gen_cost` exactly (G03, G05, G14, G20); the
  other 16 are within about 1 % (-16 .. +9 on ~700): the AGE-0 pieces are lighter on the device than on the host, the pieces
  with AGE above 0 heavier. The cause was not found. History dependence is ruled out: the 21 identities rebuilt in the
  session's order, with GENERATE's defaults, and fresh each, give identical costs; and a section's TAPE is global (set once,
  after the fit: `gen.c` `tape_age_apply`), the same in every section. The rows file carries the host-rebuilt terms.

| Tag | Piece | AGE | cpu % (end) | late /20 s | shed /20 s | device cost | host cost (rebuilt) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| G01 | BREAKS 1634 | 0 | 66 | 15.0 | 78.2 | 647 | 650 |
| G02 | DRILL'N'BASS+IDM 5749 | 0 | 65 | 34.1 | 95.3 | 722 | 726 |
| G03 | MICRO 1625 | 0 | 48 | 0.0 | 8.0 | 688 | 688 (exact) |
| G04 | GLITCH HOP+ELECTRO 9025 | 0 | 63 | 40.1 | 127.4 | 738 | 745 |
| G05 | IDM 0932 | 0 | 62 | 2.0 | 10.0 | 668 | 668 (exact) |
| G06 | ACID+AMB TECHNO 8285 | 12 | 43 | 11.0 | 55.1 | 676 | 678 |
| G07 | DRILL'N'BASS 8567 | 0 | 62 | 17.0 | 38.1 | 696 | 703 |
| G08 | MICRO 2995 | 15 | 64 | 13.0 | 73.2 | 737 | 736 |
| G09 | RETROBOC 3716 | 60 | 68 | 5.0 | 67.2 | 716 | 707 |
| G10 | WILD 8988 | 0 | 68 | 2.0 | 40.1 | 524 | 535 |
| G12 | ACID 8309 | 0 | 52 | 29.1 | 113.4 | 716 | 718 |
| G13 | RETROBOC+IDM 1146 | 70 | 50 | 7.0 | 116.3 | 722 | 716 |
| G14 | IDM+ELECTRO 5940 | 0 | 53 | 18.1 | 146.4 | 686 | 686 (exact) |
| G15 | AMB TECHNO+ACID 8384 | 0 | 55 | 10.0 | 111.3 | 730 | 746 |
| G16 | ACID+ELECTRO 1485 | 0 | 56 | 31.1 | 170.5 | 742 | 745 |
| G17 | ELECTRO+BREAKS 5234 | 0 | 49 | 1.0 | 1.0 | 702 | 703 |
| G18 | AMBIENT 6748 | 40 | 69 | 0.0 | 30.1 | 743 | 737 |
| G19 | AMB TECHNO+AMBIENT 7920 | 0 | 54 | 6.0 | 8.0 | 736 | 737 |
| G20 | IDM+DRILL'N'BASS 7721 | 0 | 69 | 7.0 | 19.1 | 733 | 741 (exact) |
| G21 | RETROBOC+IDM 4073 | 70 | 65 | 12.0 | 176.6 | 744 | 745 |

## 3. The fit

Objective (changed from the plan, by the controller): lateness was inaudible on every piece (no audio lost), so the music
must not be thinned for it. The fit takes the best score (a late row within the budget counts 2, a clean row over it 1) **subject
to** clean rows over the budget at most a quarter of the clean rows; the late rows within the budget are a report only
(`tests/gen_test.c` prints them and asserts only the clean-over bound). The two bounds of the plan together cannot hold on
these rows. Reported side by side, from `tools/gen_fit.py` (`build/cal/fit-091.txt`):

```
gen_fit: 35 rows, 21 late
  0.9's  {'sty': 118, 'wild': 155, 'e6': 40, 'v6': 13, 'k': 70}: score 34, late within 16, clean over 2, margin -0.129
  best, unconstrained {'sty': 126, 'wild': 150, 'e6': 65, 'v6': 12, 'k': 50}: score 10, late within 1, clean over 8, margin -0.167
  fitted (clean over at most a quarter of the clean rows) {'sty': 122, 'wild': 150, 'e6': 20, 'v6': 24, 'k': 70}: score 13, late within 5, clean over 3, margin -0.113; GC_DEV2_X100 155, GC_DEV2W_X100 190
  adopted {'sty': 122, 'wild': 150, 'e6': 30, 'v6': 18, 'k': 70}: score 15, late within 6, clean over 3, margin -0.103
  0.8 4741       clean late20    0.0  0.9 792/746  fit 807/725  adopted 797/725
  0.8 7848       clean late20    1.0  0.9 583/569  fit 583/588  adopted 583/588
  0.8 4493       LATE  late20   10.0  0.9 687/746  fit 711/725  adopted 697/725
  0.8 0036       LATE  late20   25.0  0.9 773/746  fit 774/725  adopted 772/725
  0.8 0965       LATE  late20    7.0  0.9 740/746  fit 749/725  adopted 743/725
  0.8 8794       LATE  late20    6.0  0.9 795/746  fit 786/725  adopted 790/725
  0.8 7625       clean late20    2.0  0.9 688/746  fit 700/725  adopted 691/725
  0.8 2020       clean late20    1.0  0.9 713/746  fit 718/725  adopted 714/725
  0.8 7033       clean late20    2.0  0.9 740/746  fit 764/725  adopted 750/725
  0.8 1751       LATE  late20   81.0  0.9 812/746  fit 812/725  adopted 812/725
  0.8 1174       LATE  late20   44.0  0.9 756/746  fit 747/725  adopted 751/725
  0.8 8790       LATE  late20    9.0  0.9 800/746  fit 800/725  adopted 800/725
  0.8 7563       clean late20    1.0  0.9 692/746  fit 692/725  adopted 692/725
  0.8 4459       clean late20    4.0  0.9 717/746  fit 717/725  adopted 717/725
  0.8 1573       clean late20    4.0  0.9 721/746  fit 721/725  adopted 721/725
  G01            LATE  late20   15.0  0.9 650/746  fit 650/725  adopted 650/725
  G02            LATE  late20   34.1  0.9 726/746  fit 750/725  adopted 736/725
  G03            clean late20    0.0  0.9 688/746  fit 688/725  adopted 688/725
  G04            LATE  late20   40.1  0.9 745/746  fit 790/725  adopted 764/725
  G05            clean late20    2.0  0.9 668/746  fit 668/725  adopted 668/725
  G06            LATE  late20   11.0  0.9 678/746  fit 669/725  adopted 673/725
  G07            LATE  late20   17.0  0.9 703/746  fit 703/725  adopted 703/725
  G08            LATE  late20   13.0  0.9 736/746  fit 736/725  adopted 736/725
  G09            clean late20    5.0  0.9 707/746  fit 705/725  adopted 705/725
  G10            clean late20    2.0  0.9 535/569  fit 535/588  adopted 535/588
  G12            LATE  late20   29.1  0.9 718/746  fit 742/725  adopted 728/725
  G13            LATE  late20    7.0  0.9 716/746  fit 712/725  adopted 713/725
  G14            LATE  late20   18.1  0.9 686/746  fit 728/725  adopted 703/725
  G15            LATE  late20   10.0  0.9 746/746  fit 747/725  adopted 745/725
  G16            LATE  late20   31.1  0.9 745/746  fit 736/725  adopted 740/725
  G17            clean late20    1.0  0.9 703/746  fit 703/725  adopted 703/725
  G18            clean late20    0.0  0.9 737/746  fit 741/725  adopted 737/725
  G19            LATE  late20    6.0  0.9 737/746  fit 735/725  adopted 735/725
  G20            LATE  late20    7.0  0.9 741/746  fit 741/725  adopted 741/725
  G21            LATE  late20   12.0  0.9 745/746  fit 745/725  adopted 745/725
```

- 0.9's constants: score 34, 16 late within, 2 clean over.
- Best, unconstrained (sty 126, wild 150, FM6 65 / 12, k 50): score 10, 1 late within, **8 clean over of 14**: refused (the
  music would be thinned for pieces that played clean).
- Best under the bound (sty 122, wild 150, FM6 20 / 24, k 70): score 13, 5 late within, 3 clean over. Its FM6 weighs 116 at
  400 voices (0.9: 92) and `gen_test`'s fit-floor checks fail with it (EXPLORE not under its budget, a later section over
  its piece, AUTO's AGE check): the generator cannot always reach the budget with parts that heavy.
- **Adopted** (sty 122, wild 150, FM6 30 / 18, k 70): the best score among the grid's points under the bound that also pass
  all of `gen_test`'s checks (the top 40 by score were built and run): score 15, 6 late within, 3 clean over. WILD's factor
  rests on two rows (G10, 0.8 7848): it is under-determined; 150 is the nearest to 0.9's 155 at the same score.

## 4. What changed in gen.c

`GC_GEN_X100` 118 to 122, `GC_WILD_X100` 155 to 150, `GC_DEV2_X100` 150 to 155, `GC_DEV2W_X100` 196 to 190, FM6
(`GC_ENG`/`GC_VOX` 1 and 12) 40 / 13 to 30 / 18, `GC_CCOMP` 70 kept. The budget at CPU TEST ratio 117: 725 a style's piece
(was 746), 588 WILD's (was 569). `gc_cost` is split into `gc_terms` and `gc_sum` (same value). Figures `gen_test` prints that
moved (no bound loosened): EXPLORE budgets 569 / 525 became 588 / 542; the device factor unset x1.50 / x1.96 became x1.55 /
x1.90; `GT_X1` 41 to 42; two cores AUTO AGE sweep: 2 of 300 over became 0 of 300 (AGE lowered on 136 became 142); EXPLORE +
RANDOM over the budget at the floor 20 of 600 became 15; the 200 PARTS / SONG pieces: 6 sections over became 4; samples of
the random stream that shift with the fit (MICRO HOUSE lead sounds, the press-before counts). `tests/golden.txt` is unchanged.

## 5. How strong it is

35 rows, one device, one session; `cpu` is noisy (the same piece at AGE 70 read 65 % and 74 %). The new pieces' cost is the host's
rebuild, within about 1 % of the device's. The model does not separate late from clean near the budget (G17 at 702 clean,
G02 at 726 late): the adopted point has 10 fewer late rows within the budget than 0.9's constants for 1 more clean row over it; it is a
better fit of a weak signal, not a predictor of lateness.

## 6. TAPE

The pair on G21 (RETROBOC+IDM 4073, AGE 70 on AUTO against AGE 0 set live): cpu at the end of the window 65 % and 74 % with AGE
70, 61 % with AGE 0; late 12.0 / 8.0 against 0.0; shed 176.6 / 72.2 against 26.1. TAPE costs: cpu +8.5 % (the mean of the AGE
windows, 69.5, over AGE 0); that is 8.5 x 1030 x 100 / (65 x 142) = 95 (142 = 117 x 122 / 100): `GC_TAPE2` 92 to 95,
`GC_TAPE1` 76 to 78 (x 139 / 169). The late/shed counts agree in direction, the cpu readings are noisy (+4 .. +13 %), one piece:
a small move.
