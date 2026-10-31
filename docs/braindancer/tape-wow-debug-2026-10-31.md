# TAPE WOW "no pitch effect on device": debug findings (2026-10-31)

Build: branch piece6-generate @ 5bc0fce (Braindancer 0.9). Method: superpowers systematic-debugging, evidence only, no fix applied.

## Verdict

**WOW is not broken, either on the device or on the host.** On the device it produces exactly what the host
produces, segment by segment, to within 0.02 cents. The "no effect" reading comes from the **measurement**:

1. meas.py analyses a fixed window from 0.8 to 3.3 s. With a fresh TAPE entry, that window lies entirely inside WOW's
   **first segment**. That segment is deterministic, lasts 3.33 s and moves the pitch by only -0.47 cents, as a constant
   offset that the median subtraction then removes.
2. The SINE KEY preset is not a pure sine. It has a 220.00 Hz partial and a 220.75 Hz partial at -2.9 dB, so it beats
   at 0.75 Hz. At every beat null the phase jumps, which reads as a -8 to -13 cent spike every ~1.33 s. These spikes
   dominate a 170 ms FFT tracker's sd (2.4 cents "off") and hide a slow offset of a few cents.

There is also a real **design weakness** behind the user-visible complaint ("WOW seems inaudible"); it is not a code
defect. Every time TAPE enters the chain, the first 3.33 s of WOW are almost flat, and WOW's full size is modest:
piecewise-constant pitch offsets of about 0.5 to 11 cents.

## 1. Device recordings (read-only)

Trackers:
- (a) Hilbert instantaneous frequency of the fundamental, band-passed with a Gaussian of 8 Hz (or 40 Hz) around 220 Hz.
  Samples where the band envelope is below half its median (the beat nulls) are masked. The result is reduced to 50 ms
  medians, or to power-weighted means per WOW segment.
- (b) STFT on harmonic 5, with a 1 s Hann window, 8x zero-padding and a 50 ms hop.

### Signal facts

| clip | onset (2 % of peak) | rms L / R (dBFS) | peak L | partials 210–230 Hz |
|---|---|---|---|---|
| dev_off_10s | 308.35 ms | -28.98 / -28.56 | -18.65 | 220.000, 220.749 (-2.9 dB), 219.25 (-20), 218.49 (-27) |
| dev_wow100_10s | 309.35 ms | -29.00 / -28.58 | -18.64 | smeared ±2 Hz by the wow |
| dev_age100_10s | 312.25 ms | -28.28 / -27.86 | -17.99 | smeared, plus FLUTTER sidebands |
| all_off / wow_only (5 s) | 304.44 / 309.35 ms | -29.34 / -29.35 | -18.67 / -18.64 | as dev_off |
| age09c_0/50/100 (5 s) | 302.5 / 305.5 / 310.9 ms | -29.32 / -28.99 / -28.64 | | |

- The note runs from about **0.31 s to 10.42 s**, not 0.8 to 10.8 s. The 5 s clips end at about 3.92 s.
- Level: WOW changes nothing (0.02 dB). AGE 100 is +0.7 dB, from SAT 85 and HISS. R sits about 0.4 dB above L in every
  clip. The L–R lag is 0 in every clip.
- Alignment: TAPE adds a fixed TAPE_D0 = 176 samples (3.99 ms). The onset differences between clips, however, spread
  from 1 to 8 ms, and two TAPE-off clips differ from each other by 1.9 ms. MIDI and USB jitter therefore swamp the
  delay, so onset alignment cannot prove TAPE was in or out of the chain.

### Pitch: WOW is there, and not tiny

Tracker (a), 50 ms medians, beat nulls masked, absolute cents against 220 Hz:
- dev_off: a flat +2.3, dipping toward the nulls every 1.33 s (the preset's beat). sd 0.75 c.
- dev_wow100: clear **steps**, the shape a linearly ramped delay produces, in which each segment's pitch is constant.
  sd 3.73 c, p-p 15.6 c. After subtracting dev_off: sd 3.51 c, p-p 14.2 c.
- dev_age100: sd 5.32 c, p-p 20 c (WOW plus FLUTTER).

Tracker (b), STFT on h5: off sd 0.84 c (p-p 2.4), WOW 100 sd 3.45 c (p-p 14.5), AGE 100 sd 6.97 c (p-p 26.4).

Pitch offset in each WOW segment, as a power-weighted mean of the instantaneous frequency. For the device it is
dev_wow100 minus dev_off; times are from note-on (= TAPE's first block, see §3):

| segment | time (s) | host WOW 100 | device WOW 100 |
|---|---|---|---|
| 0 | 0.00–3.33 | -0.48 | -0.47 |
| 1 | 3.33–5.00 | +2.56 | +2.55 |
| 2 | 5.00–7.50 | -1.93 | -1.92 |
| 3 | 7.50–8.09 | +7.51 | +7.50 |
| 4 | 8.09–8.92 | -4.66 | -4.66 |
| 5 | 8.92–9.42 | +8.30 | +8.28 |

Device and host agree to 0.02 c in every segment. The WOW signal on the device is therefore **present and
bit-for-bit what the code computes**.

- Size: per-segment offsets of 0.5 to 8.3 c in this 10 s; sd 3.5 c, p-p 14 c.
- Spectrally: a 0.15–1 Hz square-ish pitch wave, made of half-cycles of 0.5 to 3.33 s.

The window that meas.py uses (0.8–3.3 s, tracker (a)):
- dev_off: mean +1.78 c, sd 0.69.
- dev_wow100: mean +1.31 c, sd 0.67.

The only difference is the first segment's -0.47 c constant, and the median removes it. This explains "WOW 100 sd 2.42
vs 2.44".

The older 5 s clips:
- wow_only.wav has the same shape as all_off.wav, offset by -0.46 c. That is the same first segment, and the clip
  holds nothing else.
- age09c_50 (WOW 100 via AGE) shows the same thing.
- age09c_100 is the same, plus FLUTTER.

FLUTTER on dev_age100 (5–25 Hz band, 40 Hz Hilbert): rms **7.29 c, peak at 15.06 Hz**. The host gives exactly the
same figure, 7.29 c at 15.06 Hz. Device off: 0.27 c.

dev_age100's WOW segments do **not** match a fresh host render, while its FLUTTER does. Its steps are about ±1 to 10 c,
in the same range, but in another order. This fits TAPE having stayed in the chain from the previous run: WOW's random
sequence continues on entry, and the rng is reseeded only on entry. It does not indicate a defect.

## 2. Host render

Harness: /tmp/wowdbg/h.c, now deleted. It included tests/tape_test.c with its main renamed, and ran an 8000-amplitude
220 Hz sine through `tape_begin` and `tape_block(CTL)` with the same loop as `tape_tone_s`, for 11 s. It was built with
`tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests`. FS is 44100, CTL is 32 and int is 32 bits.

Logged WOW segments (block, wpos in Q16, wseg):

| segment | start | target | samples | duration |
|---|---|---|---|---|
| 1st | 0 | +40.3 samples (2643040 Q16) | 147000 | **3.33 s** |
| 2nd | | -68.4 | 73500 | |
| 3rd | | +54.3 | 110250 | |
| 4th | | -58.4 | 25941 | |
| 5th | | +40.1 | 36750 | |
| 6th | | -65.7 | 22050 | |

Predicted pitch per segment: -1731 × Δdelay / wseg cents. That gives -0.47, +2.56, -1.93, +7.5, -4.6 and +8.3, which
matches the measured values.

Same tracker (50 ms medians, 0.3–10.5 s):

| setting | sd (c) | p-p (c) | per-segment offsets (c) |
|---|---|---|---|
| WOW 100 | 3.61 | 14.9 | -0.48, +2.56, -1.93, +7.51, -4.66, +8.30, -5.90 |
| AGE 50 (100 55 45 78 50) | 3.73 | 17.0 | -0.38, +2.63, -1.95, +7.76, -4.91, +8.36, -6.16 |
| AGE 100 (100 100 85 100 90) | 3.67 | 15.7 | WOW as above; FLUTTER 7.29 c rms at 15.06 Hz |

FLUTTER at AGE 50 is 3.06 c rms at 10.9 Hz. AGE 50 and 100 share WOW 100 and the same seed, so their WOW is
identical. tests/tape_test.c test_age_09 gives AGE 50 sd 4.77 c, using 20 s of 50 ms zero-crossing windows starting at
0.5 s. Over 20 s it includes more of the large later segments, and it counts the flutter that the zero-crossing
windows alias, so it agrees with this.

## 3. Code review (firmware/src/fx.c)

- **Q formats.**
  - wow_a = WOW × 184 (Q8 samples), at most 18400.
  - mag = wow_a × (128..255) >> 8, at most 18328 (71.6 samples).
  - wpos and wstep are Q16 samples. `mag << 8` is at most 4.69 M, so there is no int32 overflow.
  - `wstep * n` is at most about 14 k.
  - wsm is a one-pole with a block-rate time constant of about 23 ms. m1 = wsm >> 8 is Q8.
  - The `_Static_assert` at fx.c:389–391 holds: 176 + 71 + 3 + 3 < 256.
  - There are no width issues on the int32 device compiler. The host has int32 as well, and the per-segment agreement
    to 0.02 c confirms this.
- **wseg and FS.** `FS * 10u / (3u + r % 18u)` gives 441000/3..20, that is 0.5–3.33 s per segment
  (fx.c:516). wpos is **not** reset per block. It persists in `tape` and is cleared only on entry, by the
  `memset(&tape…)` at fx.c:469, which also reseeds rng to 0x1d872b41 (fx.c:471). It does not saturate. Each segment
  aims at an absolute ±mag target from wherever wpos is.
- **The dq ramp and the read offsets** (fx.c:526–534).
  - dq = TAPE_DIVB(end − start) is a truncating /32. `fm[c]` carries the remainder, so the next block starts exactly
    where this one ended. The algebra checks out.
  - TAPE_DIVB is valid only for n = 32. The `_Static_assert(CTL == 32)` at fx.c:393 guards it.
  - On the device, audio.c:340 calls `audio_block(o + 2u * b, CTL)` → `mix_block(out, n)`, so n is always 32, the same
    as on the host.
- **USB capture path.** audio.c:83 calls `mix_block(out, n)`, which runs TAPE at fx.c:2775, after CLIP and before
    the leveler and master_out. audio.c:91 then calls `uac_tap(out, n)` (usb.c:1420), "the USB audio input: the same
    master output". USB IN (device to host) therefore carries the signal after TAPE. `uac_in_fill` (audio.c:78) is the
    opposite direction, host to device.
- **WOW's clock freezes in silence.**
  - When everything is at rest, `mix_hush()` → `mix_silent` returns before `tape_block` (fx.c:2704), so no WOW segment
    advances.
  - A WOW set over SysEx before the note therefore enters TAPE fresh, with the fixed seed.
  - The note then starts at the beginning of the deterministic first segment. The device's segment boundaries line up
    with the host's when measured from note-on, which shows this.

### The weakness (not a defect; the cause of "inaudible" in short tests)

fx.c:513–518, on the first segment after every entry (fx.c:469–471):

```c
tape.wseg = (uint32_t)FS * 10u / (3u + r % 18u);
tape.wstep = (((tape.wsg ? mag : -mag) << 8) - tape.wpos) / (int32_t)tape.wseg;
```

- With the fixed seed, the first `r % 18` is 0. The first segment is therefore the **longest possible** one, 3.33 s.
- It starts from wpos = 0, so it covers **half a swing**, 40 samples, giving -0.47 c at WOW 100.
- Every entry into the chain repeats this: after a SysEx set, after a project load, or after AGE goes from 0 to above 0.
- Until 3.33 s of non-silent audio have passed, WOW does essentially nothing.

### Scale against the spec

The spec says that at AGE 50, WOW 100 is "a clearly worn tape (audible wobble)".

What the code delivers:
- Constant-pitch steps of ±0.5 to 11.2 c per half-cycle. The ceiling is 2 × 71.6 samples over 0.5 s = 0.65 %, which
  is 11.2 c. The comment at fx.c:419 says "8.7 cents at most".
- Corners smoothed over about 23 ms.
- Typically sd 3.5 c, p-p 14 to 15 c over 10 s.

That is a real but subtle, step-like drift rather than a smooth wobble. On a pure tone, steps of 2 to 5 c are near the
pitch-change threshold, and only the short 0.5–0.8 s segments (7 to 8 c) are clearly audible. TAPE_LEN (256) and
TAPE_WOW_Q cap the size.

There is also a minor wording issue: the comments (fx.c:513 and ux_section.c:811, "0.3 .. 2 Hz") describe segment
rates. A full wobble cycle is 0.15–1 Hz.

## 4. Conclusion

| question | answer |
|---|---|
| broken on device? | **No.** It matches the host per segment to ±0.02 c, and FLUTTER matches too (7.29 c at 15.06 Hz). |
| broken everywhere? | **No.** The code does what its arithmetic says. There is no overflow, the ramps are correct and n is 32 everywhere. |
| working but small? | **Yes, partly.** sd ≈ 3.5 c, p-p ≈ 14 c, segment offsets 0.5–8.3 c over 10 s, at most 11.2 c. The first 3.33 s after any entry give only -0.47 c. |
| measurement flawed? | **Yes.** meas.py's 0.8–3.3 s window sits inside that first segment, its median subtraction removes the constant offset, and the preset's 0.75 Hz beat-null spikes dominate a 170 ms tracker's sd. |

AGE 100's noise rise to -26.5 dB is HISS 100 (and SAT 85), as designed.

### Proposed fixes (not applied)

1. **Measurement.**
   - Analyse at least 10 s and skip the first 3.4 s after TAPE first sees audio.
   - Or use per-segment means of the Hilbert instantaneous frequency, with beat nulls masked, as above.
   - Or use a pure-sine preset with no detuned partial.
   - Subtract an off recording taken with the same note-on.
2. **The first segment** (the user-visible "WOW does nothing at first"), at fx.c:516. When the segment starts from rest,
   make it short so the first swing is heard at once, for example:
   ```c
   tape.wseg = (uint32_t)FS * 10u / (tape.wpos ? 3u + r % 18u : 12u + r % 9u);   /* (from rest: 0.5 .. 0.83 s) */
   ```
   At WOW 100 the first offset becomes about 2 to 5.6 c instead of 0.47 c (half a swing, 36 to 72 samples, over 0.5 to 0.83 s). A test should pin the first 1 s, for
   example "WOW 100: |offset| ≥ 2 c within the first second".
3. **Optional, a design decision.** If "clearly worn" needs more than about ±8 to 11 c, double TAPE_LEN (512, 11.6 ms
   per side) and TAPE_WOW_Q. Separately, consider a smoothed (cosine-shaped) delay path instead of linear ramps, so the
   pitch wobbles rather than steps.

Scratch (/tmp/wowdbg) was deleted. The repo is unmodified, and this file lives in the git-ignored .superpowers/.
