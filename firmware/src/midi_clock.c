/* SPDX-License-Identifier: GPL-3.0-only
 * Adapted from MIDI clock contributions by ChanceTheMaker and keremimo (2026).
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 NorfolkHacker */
/* (ACID FM 0.5) a beat of clocks and one more: the tempo is measured over a whole beat, not six clocks */
#define MCK_TS 25u
/* Audio ISR state. Input timestamps keep USB/TRS jitter outside the render loop. (ACID FM 3c-1 task 8) npulse: the clocks
 * received from the source followed (24 a beat; 0 at START): GLO > MIDI's diamond lit a beat (ux_glo.c uxg_clk_lit) */
static struct {
    uint32_t pos, rendered, last_ms, start_ms, rem, interval_ms;
    uint32_t pulse_samples, interp_q8, tempo_ms, npulse;
    uint32_t ts[MCK_TS], est_q8, used_q8;        /* (ACID FM 0.5) the last clocks' times; the beat (ms, Q8) estimated, applied */
    uint8_t mode, have_pulse, tempo_valid, tempo_n, ts_n, ts_i;
} midi_clock;

static __attribute__((noinline)) void midi_clock_transport(uint32_t status, uint32_t ms)
{
    if (status == 0xFAu) {                         /* Start: step zero */
        midi_clock.pos = midi_clock.rendered = midi_clock.rem = 0;
        midi_clock.have_pulse = midi_clock.tempo_valid = 0;
        midi_clock.interval_ms = 0;
        midi_clock.start_ms = ms;
        midi_clock.npulse = 0;
        seq_start();
    } else if (status == 0xFBu) {                  /* Continue: preserve the step */
        midi_clock.pos = midi_clock.rendered;
        midi_clock.rem = 0;
        midi_clock.have_pulse = midi_clock.tempo_valid = 0;
        midi_clock.interval_ms = 0;
        midi_clock.start_ms = ms;
        if (!song.playing)
            motion_begin();
        song.playing = 1;
    } else if (status == 0xFCu) {
        seq_stop();
    }
}

/* (ACID FM 0.5) the beat (ms, Q8) from the last n clocks kept: the slope of a line through all of them (least squares),
 * so a few ms of jitter on each clock averages out instead of landing whole on the two ends */
static uint32_t mck_beat_q8(uint32_t n)
{   /* 32-bit only (no 64-bit divide in the ISR): the clocks kept span at most a beat at 40 BPM (66 ms a clock with some jitter) */
    uint32_t i, o = (midi_clock.ts_i + MCK_TS - n) % MCK_TS, t0 = midi_clock.ts[o], suu = n * (n * n - 1u) / 3u;
    int32_t suy = 0;                                    /* u = 2x - (n - 1): sum u = 0 */
    if (midi_clock.ts[(o + n - 1u) % MCK_TS] - t0 > 66u * (n - 1u))
        return 0;                                       /* a gap (the clock stopped a while): no tempo from it */
    for (i = 0; i < n; i++)
        suy += (2 * (int32_t)i - (int32_t)(n - 1u)) * (int32_t)(midi_clock.ts[(o + i) % MCK_TS] - t0);
    if (suy <= 0)
        return 0;
    return ((uint32_t)suy * (2u * 24u * 256u) + suu / 2u) / suu;   /* <= ~2.1e9 */
}

static __attribute__((noinline)) void midi_clock_pulse(uint32_t ms)
{
    uint32_t q;
    midi_clock.npulse++;
    if (!midi_clock.tempo_valid) {
        midi_clock.tempo_valid = 1;
        midi_clock.tempo_ms = ms;
        midi_clock.tempo_n = midi_clock.ts_n = midi_clock.ts_i = 0;
        midi_clock.est_q8 = 0;
        midi_clock.used_q8 = beat_samples() * 5120u / (FS / 50u);   /* the tempo playing now (ms Q8) */
    }
    midi_clock.ts[midi_clock.ts_i] = ms;
    midi_clock.ts_i = (uint8_t)((midi_clock.ts_i + 1u) % MCK_TS);
    if (midi_clock.ts_n < MCK_TS)
        midi_clock.ts_n++;
    if (++midi_clock.tempo_n >= 6u && midi_clock.ts_n > 6u) {
        /* (ACID FM 0.5, the device with a hardware sequencer over TRS) every quarter beat, the beat measured over the
         * clocks kept (a whole beat once there are 25; a line fitted through them all), smoothed, and applied only when it moves by more than 0.25 %:
         * six clocks at a millisecond's resolution (and a few ms of jitter) read 117..123 at a steady 120. A jump of
         * more than 4 % (the master's tempo changed) is taken at once. */
        uint32_t q = mck_beat_q8(midi_clock.ts_n);       /* the beat, ms Q8 */
        midi_clock.tempo_ms = ms;
        midi_clock.tempo_n = 0;
        if (q >= 240u * 256u && q <= 1560u * 256u) {    /* 250 .. 38 BPM (shown 240 .. 40); gaps, corrupt bursts rejected */
            int32_t d = (int32_t)(q - midi_clock.est_q8);
            if (midi_clock.est_q8 && (uint32_t)(d < 0 ? -d : d) > midi_clock.est_q8 / 25u) {
                /* the master moved: measure again from the last quarter beat only (the clocks before were the old
                 * tempo), then over more clocks as they come */
                midi_clock.ts_n = 7u;
                midi_clock.est_q8 = mck_beat_q8(7u);
            } else if (!midi_clock.est_q8 || midi_clock.ts_n < MCK_TS)
                midi_clock.est_q8 = q;                  /* (still filling the beat: each reading is the better one) */
            else
                midi_clock.est_q8 = (uint32_t)((int32_t)midi_clock.est_q8 + d / 8);
            d = (int32_t)(midi_clock.est_q8 - midi_clock.used_q8);
            /* a beat's worth of clocks reads to ~0.1 %: act on 0.25 %; fewer clocks (the start, after a jump) on 1 % */
            if (!midi_clock.used_q8 || (uint32_t)(d < 0 ? -d : d) > midi_clock.used_q8 / (midi_clock.ts_n < MCK_TS ? 100u : 400u)) {
                uint32_t old_beat = beat_samples(), new_beat;
                midi_clock.used_q8 = midi_clock.est_q8;
                new_beat = (FS / 50u) * midi_clock.used_q8 / 5120u;
                if (song.playing && new_beat != old_beat) {
                    uint32_t i, ratio = (new_beat << 12) / old_beat;
                    /* Preserve each track's fractional step phase when the master
                     * changes tempo. The next pulse then lands on its new boundary. */
                    for (i = 0; i < NTRK; i++) {
                        if (trk[i].seq_pos < (uint32_t)FS * 2u)
                            trk[i].seq_pos = (uint32_t)(((uint64_t)trk[i].seq_pos * ratio + 2048u) >> 12);
                        if (trk[i].seq_off)
                            trk[i].seq_off = (uint32_t)(((uint64_t)trk[i].seq_off * ratio + 2048u) >> 12);
                    }
                    if (clk_pos < (uint32_t)FS * 2u)   /* (the metronome's beat as the steps) */
                        clk_pos = (uint32_t)(((uint64_t)clk_pos * ratio + 2048u) >> 12);
                }
                midi_beat_samples = new_beat;
                song.g[G_BPM] = (int16_t)clamp((int32_t)((60000u * 256u + midi_clock.used_q8 / 2u) / midi_clock.used_q8), 40, 240);
            }
        }
    }
    if (song.playing) {
        if (midi_clock.have_pulse) {
            uint32_t interval = ms - midi_clock.last_ms;
            if (interval >= 8u && interval <= 80u)
                midi_clock.interval_ms = interval;
            q = beat_samples() + midi_clock.rem;
            midi_clock.pos += q / 24u;
            midi_clock.rem = q % 24u;
        }
        midi_clock.have_pulse = 1;
    }
    midi_clock.pulse_samples = beat_samples() / 24u;
    if (!midi_clock.interval_ms)
        midi_clock.interval_ms = beat_samples() * 1000u / ((uint32_t)FS * 24u);
    midi_clock.interp_q8 = midi_clock.pulse_samples * 256u / midi_clock.interval_ms;
    midi_clock.last_ms = ms;
}

static uint32_t midi_clock_advance(uint32_t now)
{
    uint32_t target, elapsed, offset, n;
    if (!midi_clock.have_pulse)
        return 0;
    elapsed = now - midi_clock.last_ms;
    /* Interpolate to the next pulse, never across it before it arrives. */
    if (elapsed > midi_clock.interval_ms)
        elapsed = midi_clock.interval_ms;
    offset = elapsed * midi_clock.interp_q8 >> 8;
    if (offset >= midi_clock.pulse_samples)
        offset = midi_clock.pulse_samples - 1u;
    target = midi_clock.pos + offset;
    n = (int32_t)(target - midi_clock.rendered) > 0 ? target - midi_clock.rendered : 0u;
    if (n > (uint32_t)FS / 8u)
        n = (uint32_t)FS / 8u;
    midi_clock.rendered += n;
    return n;
}

