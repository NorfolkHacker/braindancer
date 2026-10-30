/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 NorfolkHacker */
/* (Braindancer 0.8) TAPE, the master effect (src/fx.c tape_begin / tape_block, globals G_TAGE .. G_TTONE) through the
 * real sources (ui_test.c's).
 *   build/host/tape_test
 * 1. off: every TAPE global 0: the sequenced four-part mix of tests/djf_test.c bit for bit 0.7's (DJF_CENTRE_090), and
 *    tape_block never runs (out of the chain).
 * 2. a tone through it (tape_begin + tape_block alone): WOW's drift (cents, its rate), FLUTTER's (its rate, left and
 *    right apart), SAT's level kept and its peaks held, the line's delay exact, TONE's roll-off, HISS under the music
 *    and none over silence.
 * 3. in and out of the chain without a click; out again within its fade, and not run after.
 * 4. at rest after a note: tape_idle, its states exactly 0, the silence path taken.
 * 5. stored: no block when all 0 (the bytes as before), 'T' 'P' the last block, the round trip, old projects, 255
 *    clamped, NEW PROJECT, UNDO. */
#include <stdint.h>
#include <stdio.h>
static unsigned tape_runs;
static int32_t tape_in_any;                             /* (what tape_block saw: its input not all 0) */
#define TAPE_COUNT()                                                                                       \
    do {                                                                                                   \
        uint32_t z_;                                                                                       \
        tape_runs++;                                                                                       \
        for (z_ = 0; z_ < n; z_++) tape_in_any |= mix_l[z_] | mix_r[z_] | wet[z_];                         \
    } while (0)
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static void ck(const char *what, int ok)
{
    printf("tape: %-110s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}
#define D0 ((int32_t)(TAPE_D0 >> 8))                    /* the line's centre read, samples */

/* ---- 1. off ---- */
#define DJF_CENTRE_090 0x8b898fe6u
static uint32_t centre_render(void)                     /* (tests/djf_test.c's scene) */
{
    static int32_t o[2u * CTL];
    uint32_t b, i, h = 2166136261u, k;
    ui_power_on();
    song.master_q12 = 4096;
    trk_live = NTRK;
    for (k = 0; k < 4u; k++) {
        track_t *t = &trk[k];
        set_engine_of(t, k == 3u ? ENGI_DRUM : k == 1u ? ENGI_FM6 : ENGI_ANALOG);
        t->engine = t->eng_req;
        apply_preset_to(t, k);
        t->p[P_MUTE] = 0; t->p[P_SLEN] = 16; t->p[P_DLY] = 50; t->p[P_REV] = 60; t->p[P_CHOR] = 30;
        for (i = 0; i < 16u; i++) {
            step_t *s = &t->step[i];
            memset(s, 0, sizeof *s);
            s->time = (i + k) % 3u ? ST_NOTE : ST_REST;
            s->vel = 100;
            if (k == 3u) s->hit = (uint8_t)(1u << (i % 4u));
            else { s->n = 2; s->note[0] = (uint8_t)(40 + 12 * k + i % 7u); s->note[1] = (uint8_t)(s->note[0] + 7u); }
        }
    }
    song.g[G_CLIP] = 30;
    transport_req = 1; ui_input();
    for (b = 0; b < 2400u; b++) {
        memset(o, 0, sizeof o);
        mix_block(o, CTL);
        for (i = 0; i < 2u * CTL; i++)
            h = (h ^ (uint32_t)o[i]) * 16777619u;
    }
    transport_req = 2; ui_input();
    return h;
}
static void test_off(void)
{
    uint32_t h, k, z = 1;
    char what[140];
    tape_runs = 0;
    h = centre_render();
    for (k = G_TAGE; k <= G_TTONE; k++)
        z &= song.g[k] == 0;
    snprintf(what, sizeof what, "off (every TAPE global 0): the sequenced four-part mix bit for bit 0.7's (%08x)", h);
    ck(what, z && h == DJF_CENTRE_090);
    ck("off: tape_block never ran, TAPE out of the chain", tape_runs == 0u && !tape.on);
}

/* ---- 2. a tone through it ---- */
#define NMAX (44100u * 20u + 64u)
static int32_t in_l[NMAX], out_l[NMAX], out_r[NMAX];
static void tape_reset(void)
{
    uint32_t k;
    for (k = G_TAGE; k <= G_TTONE; k++)
        song.g[k] = 0;
    memset(&tape, 0, sizeof tape);
    memset(tape_buf, 0, sizeof tape_buf);
    fx_safe = 0;
}
static void tape_g(int32_t wow, int32_t flut, int32_t sat, int32_t hiss, int32_t tone)
{
    song.g[G_TWOW] = (int16_t)wow; song.g[G_TFLUT] = (int16_t)flut; song.g[G_TSAT] = (int16_t)sat;
    song.g[G_THISS] = (int16_t)hiss; song.g[G_TTONE] = (int16_t)tone;
}
/* a sine of hz, amplitude amp (the mix's scale: 32768 the output's full scale), for nblk blocks through TAPE as
 * mix_block runs it (tape_begin, tape_block while in the chain), from a fresh state with the globals set; the input in
 * in_l, the output in out_l / out_r; silent_from: the input 0 from that block on */
static void tape_tone_s(double hz, double amp, uint32_t nblk, uint32_t silent_from)
{
    uint32_t b, i;
    double ph = 0;
    memset(&tape, 0, sizeof tape);
    for (b = 0; b < nblk; b++) {
        for (i = 0; i < CTL; i++) {
            int32_t x = b < silent_from ? (int32_t)lrint(amp * sin(ph)) : 0;
            ph += 2.0 * M_PI * hz / FS;
            mix_l[i] = mix_r[i] = in_l[b * CTL + i] = x;
            wet[i] = 0;
        }
        tape_begin();
        if (tape.on)
            tape_block(CTL);
        for (i = 0; i < CTL; i++) {
            out_l[b * CTL + i] = mix_l[i] + wet[i];
            out_r[b * CTL + i] = mix_r[i] + wet[i];
        }
    }
}
static void tape_tone(double hz, double amp, uint32_t nblk) { tape_tone_s(hz, amp, nblk, nblk); }

/* the pitch in cents against hz0, window by window (w samples) from sample s0: the rising zero crossings, interpolated */
static uint32_t cents_of(const int32_t *y, uint32_t n, uint32_t s0, uint32_t w, double hz0, double *c)
{
    uint32_t k = 0, s;
    for (s = s0; s + w <= n; s += w) {
        double t0 = -1, t1 = 0;
        uint32_t i, m = 0;
        for (i = s + 1u; i < s + w; i++)
            if (y[i - 1u] < 0 && y[i] >= 0) {
                double t = (double)(i - 1u) + (double)-y[i - 1u] / (double)(y[i] - y[i - 1u]);
                if (t0 < 0) t0 = t;
                t1 = t;
                m++;
            }
        c[k++] = m > 1u ? 1200.0 * log2((double)(m - 1u) * FS / (t1 - t0) / hz0) : 0;
    }
    return k;
}
static double peak_abs(const double *c, uint32_t n)
{
    double m = 0;
    uint32_t i;
    for (i = 0; i < n; i++)
        if (fabs(c[i]) > m) m = fabs(c[i]);
    return m;
}
/* the deviation's swings: the times between its sign changes (hysteresis h), as rates (1 / the time); slowest, fastest */
static uint32_t swings(const double *c, uint32_t n, double dt, double h, double *slow, double *fast)
{
    int sg = 0;
    uint32_t i, last = 0, k = 0;
    *slow = 1e9; *fast = 0;
    for (i = 0; i < n; i++) {
        int s = c[i] > h ? 1 : c[i] < -h ? -1 : 0;
        if (!s || s == sg) continue;
        if (sg) {
            if (k) {
                double r = 1.0 / ((double)(i - last) * dt);
                if (r < *slow) *slow = r;
                if (r > *fast) *fast = r;
            }
            k++;
            last = i;
        }
        sg = s;
    }
    return k;
}
static double dominant_hz(const double *c, uint32_t n, double dt)   /* the strongest rate 1..40 Hz */
{
    double best = 0, bh = 0, hz, mean = 0;
    uint32_t i;
    for (i = 0; i < n; i++) mean += c[i] / n;
    for (hz = 1.0; hz <= 40.0; hz += 0.05) {
        double re = 0, im = 0;
        for (i = 0; i < n; i++) {
            re += (c[i] - mean) * cos(2.0 * M_PI * hz * i * dt);
            im += (c[i] - mean) * sin(2.0 * M_PI * hz * i * dt);
        }
        if (re * re + im * im > best) { best = re * re + im * im; bh = hz; }
    }
    return bh;
}
static double corr(const double *a, const double *b, uint32_t n)
{
    double ma = 0, mb = 0, sab = 0, saa = 0, sbb = 0;
    uint32_t i;
    for (i = 0; i < n; i++) { ma += a[i] / n; mb += b[i] / n; }
    for (i = 0; i < n; i++) {
        sab += (a[i] - ma) * (b[i] - mb);
        saa += (a[i] - ma) * (a[i] - ma);
        sbb += (b[i] - mb) * (b[i] - mb);
    }
    return sab / sqrt(saa * sbb + 1e-30);
}
static double rms_db(const int32_t *y, uint32_t s0, uint32_t n)
{
    double s = 0;
    uint32_t i;
    for (i = s0; i < n; i++) s += (double)y[i] * y[i];
    return 10.0 * log10(s / (n - s0) + 1e-30);
}
static int32_t peak_of(const int32_t *y, uint32_t s0, uint32_t n)
{
    int32_t m = 0;
    uint32_t i;
    for (i = s0; i < n; i++)
        if (abs(y[i]) > m) m = abs(y[i]);
    return m;
}
static double gain_db(int32_t tone, double hz)          /* TONE's: a sine's level out against in, settled */
{
    uint32_t nb = 1500u, n = nb * CTL, s0 = 300u * CTL;
    tape_reset();
    tape_g(0, 0, 0, 0, tone);
    tape_tone(hz, 8000, nb);
    return rms_db(out_l, s0, n) - rms_db(in_l, s0 - D0, n - D0);
}
#define NBW (44100u * 8u / CTL)                  /* (8 s; WOW's swings: 20 s) */
#define NBW20 (44100u * 20u / CTL)
static double cw[NMAX / 220u], cw2[NMAX / 220u], cr[NMAX / 220u];
static void test_tone_through(void)
{
    char what[180];
    uint32_t nb = NBW, n = nb * CTL, s0 = 11025u, nw, k, i, n20 = NBW20 * CTL;
    double p100, p30, slow, fast, dom, dom1, cc;
    /* WOW */
    tape_reset();
    tape_g(100, 0, 0, 0, 0);
    tape_tone(1000, 8000, NBW20);
    nw = cents_of(out_l, n20, s0, 882u, 1000, cw);
    p100 = peak_abs(cw, nw);
    k = swings(cw, nw, 0.02, 0.3, &slow, &fast);
    tape_reset();
    tape_g(30, 0, 0, 0, 0);
    tape_tone(1000, 8000, NBW20);
    p30 = peak_abs(cw2, cents_of(out_l, n20, s0, 882u, 1000, cw2));
    snprintf(what, sizeof what, "WOW 100 (20 s): peak %.1f cents (3..45), %u swings at %.2f .. %.2f Hz (0.25..2.2); WOW 30: %.1f cents",
             p100, k, slow, fast, p30);
    ck(what, p100 >= 3.0 && p100 <= 45.0 && k >= 8u && slow >= 0.25 && fast <= 2.2 && p30 < p100);
    /* FLUTTER */
    tape_reset();
    tape_g(0, 100, 0, 0, 0);
    tape_tone(1000, 8000, nb);
    nw = cents_of(out_l, n, s0, 221u, 1000, cw);
    (void)cents_of(out_r, n, s0, 221u, 1000, cr);
    dom = dominant_hz(cw, nw, 221.0 / FS);
    p100 = peak_abs(cw, nw);
    cc = corr(cw, cr, nw);
    tape_reset();
    tape_g(0, 1, 0, 0, 0);
    tape_tone(1000, 8000, 50);
    dom1 = (double)tape.flut_inc[0] * FS / 4294967296.0;
    snprintf(what, sizeof what, "FLUTTER 100: at %.2f Hz (13..16), peak %.1f cents (<= 15), left / right correlation %.2f (< 0.95); "
             "FLUTTER 1: %.2f Hz", dom, p100, cc, dom1);
    ck(what, dom >= 13.0 && dom <= 16.0 && p100 > 1.0 && p100 <= 15.0 && cc < 0.95 && dom1 > 5.9 && dom1 < 6.3);
    /* SAT */
    {
        double q = 131072.0 * pow(10.0, -15.0 / 20.0), dl;
        int32_t pd, po;
        tape_reset();
        tape_g(0, 0, 100, 0, 0);
        tape_tone(220, q, 600);
        dl = rms_db(out_l, 200u * CTL, 600u * CTL) - rms_db(in_l, 200u * CTL - D0, 600u * CTL - D0);
        tape_reset();
        tape_g(0, 0, 100, 0, 0);
        tape_tone(220, 131071, 600);
        pd = peak_of(in_l, 200u * CTL, 600u * CTL);
        po = peak_of(out_l, 200u * CTL, 600u * CTL);
        snprintf(what, sizeof what, "SAT 100: a -15 dB sine's level %+.2f dB (within 1.5), a 0 dB sine's peak %d under the dry's %d", dl,
                 po, pd);
        ck(what, fabs(dl) <= 1.5 && po < pd);
    }
    /* the line's delay, exact */
    {
        int32_t m = 0, m1 = 0;
        tape_reset();
        tape_g(0, 0, 0, 1, 0);
        tape_tone(440, 8000, 200);
        for (i = 100u * CTL; i < 200u * CTL; i++) {
            if (abs(out_l[i] - in_l[i - D0]) > m) m = abs(out_l[i] - in_l[i - D0]);
            if (abs(out_l[i] - in_l[i - D0 - 1]) > m1) m1 = abs(out_l[i] - in_l[i - D0 - 1]);
        }
        snprintf(what, sizeof what, "HISS 1 alone: the dry delayed exactly %d samples (%.2f ms; residual %d, a sample off %d)", D0,
                 D0 * 1000.0 / FS, m, m1);
        ck(what, m <= 16 && m1 > 100);
    }
    /* TONE */
    {
        double g300 = gain_db(100, 300), g10k = gain_db(100, 10000), h50 = gain_db(50, 10000), h0 = gain_db(0, 10000);
        snprintf(what, sizeof what, "TONE 100: 300 Hz %+.2f dB (0.5), 10 kHz %+.1f dB (<= -8); TONE 50 10 kHz %+.1f between TONE 0's %+.1f "
                 "and 100's", g300, g10k, h50, h0);
        ck(what, fabs(g300) <= 0.5 && g10k <= -8.0 && h50 < h0 - 0.5 && h50 > g10k + 0.5);
    }
    /* HISS */
    {
        double r12, r18, t12;
        uint32_t nbh = 600u, a = 200u * CTL, zero = 0, last = 0;
        static int32_t res[600u * CTL];
        tape_reset();
        tape_g(0, 0, 0, 100, 0);
        tape_tone(1000, 32768.0 * pow(10.0, -12.0 / 20.0), nbh);
        for (i = a; i < nbh * CTL; i++) res[i] = out_l[i] - in_l[i - D0];
        r12 = rms_db(res, a, nbh * CTL);
        t12 = rms_db(in_l, a, nbh * CTL);
        tape_reset();
        tape_g(0, 0, 0, 100, 0);
        tape_tone(1000, 32768.0 * pow(10.0, -18.0 / 20.0), nbh);
        for (i = a; i < nbh * CTL; i++) res[i] = out_l[i] - in_l[i - D0];
        r18 = rms_db(res, a, nbh * CTL);
        tape_reset();
        tape_g(0, 0, 0, 100, 0);
        tape_tone_s(1000, 32768.0 * pow(10.0, -12.0 / 20.0), nbh, 300u);   /* (the tone, then silence) */
        for (i = 300u * CTL; i < nbh * CTL; i++)
            if (out_l[i] || out_r[i]) last = i;
        tape_reset();                                   /* (silence only, from the start) */
        tape_g(0, 0, 0, 100, 0);
        tape_tone(1000, 0, 100);
        for (k = 0; k < 100u * CTL; k++) zero |= (uint32_t)(out_l[k] | out_r[k]);
        snprintf(what, sizeof what, "HISS 100: %.1f dB under a -12 dB tone (24..42), %.1f dB lower under a -18 dB one (6 +-1.5); none over "
                 "silence; 0 %u blocks after a tone", t12 - r12, r12 - r18, last / CTL + 1u - 300u);
        ck(what, t12 - r12 >= 24.0 && t12 - r12 <= 42.0 && fabs(r12 - r18 - 6.0) <= 1.5 && !zero &&
           last / CTL + 1u - 300u <= (uint32_t)(D0 + 2300) / CTL + 2u);
    }
    tape_reset();
}

/* ---- 3. in and out ---- */
/* a 220 Hz tone at -12 dB, TAPE in (WOW 40) at block 100, out at 400: the output's largest second difference (*ms: in the
 * chain, away from the edges); *off: the blocks from 400 until out of the chain; *runs: tape_block's count then. hard: in
 * at once, no fade (a click, to see the measure catch it) */
static double in_out_d2(int hard, double *ms, uint32_t *off, uint32_t *runs)
{
    uint32_t b, i;
    double amp = 32768.0 * pow(10.0, -12.0 / 20.0), w = 2.0 * M_PI * 220.0 / FS, m = 0, ph = 0;
    int32_t p1 = 0, p2 = 0;
    tape_reset();
    *ms = 0; *off = 0; *runs = 0;
    for (b = 0; b < 600u; b++) {
        if (b == 100u) tape_g(40, 0, 0, 0, 0);
        if (b == 400u) tape_g(0, 0, 0, 0, 0);
        for (i = 0; i < CTL; i++) {
            mix_l[i] = mix_r[i] = (int32_t)lrint(amp * sin(ph));
            ph += w;
            wet[i] = 0;
        }
        tape_begin();
        if (hard && tape.on && b < 400u)
            tape.g = TAPE_FULL;                         /* (no fade, no hold) */
        if (tape.on)
            tape_block(CTL);
        for (i = 0; i < CTL; i++) {
            int32_t y = mix_l[i] + wet[i], d2 = y - 2 * p1 + p2;
            if (b >= 4u && fabs((double)d2) > m) m = fabs((double)d2);
            if (b >= 200u && b < 380u && fabs((double)d2) > *ms) *ms = fabs((double)d2);
            p2 = p1;
            p1 = y;
        }
        if (b >= 400u && !tape.on && !*off) { *off = b + 1u - 400u; *runs = tape_runs; }
    }
    tape_reset();
    return m;
}
static void test_in_out(void)
{
    char what[200];
    uint32_t off, runs, o2, r2;
    double w = 2.0 * M_PI * 220.0 / FS, tone = 32768.0 * pow(10.0, -12.0 / 20.0) * w * w, ms, ms2, m, h;
    m = in_out_d2(0, &ms, &off, &runs);
    snprintf(what, sizeof what, "in at block 100, out at 400: second difference %.0f (in, between: %.0f; the tone's %.0f, at most x4); "
             "out %u blocks after (<= %u), not run since", m, ms, tone, off, TAPE_FADE / CTL + 1u);
    ck(what, m < 4.0 * tone && off && off <= TAPE_FADE / CTL + 1u && tape_runs == runs);
    h = in_out_d2(1, &ms2, &o2, &r2);
    snprintf(what, sizeof what, "(the measure: TAPE switched in at once is a click: %.0f, past x4)", h);
    ck(what, h > 4.0 * tone);
}

/* ---- 4. at rest ---- */
static void test_rest(void)
{
    static int32_t o[2u * CTL];
    char what[180];
    uint32_t b, i, tail = 0, idle = 0, rest0;
    ui_power_on();
    load_depth++; fm6_init(); power_on_parts(); load_depth--;
    song.master_q12 = 4096;
    mc_host = 1;
    tape_reset();
    tape_g(40, 0, 0, 50, 50);
    for (i = 0; i < 3u; i++) trk_note_on(&trk[4], 48u + 4u * i, 100);
    for (b = 0; b < 600u; b++) mix_block(o, CTL);
    for (i = 0; i < 3u; i++) trk_note_off(&trk[4], 48u + 4u * i);
    rest0 = prf_cur.fxrest;
    for (b = 0; b < 40000u; b++) {
        if (b % 32u == 0u) { ui_input(); song_poll(); host_ticks += 20000u; fm1_ms += 20u; }
        tape_in_any = 0;
        mix_block(o, CTL);
        if (tape_in_any) { tail = b; idle = 0; }
        else if (!idle && tape.on && tape_idle()) idle = b;
        if (idle && prf_cur.fxrest > rest0 + 10u) break;
    }
    snprintf(what, sizeof what, "a note, then silence: TAPE at rest %u blocks after the tail's end (<= %u), its states 0, the silence "
             "path taken (%u blocks)", idle - tail, TAPE_QUIET + 2u, prf_cur.fxrest - rest0);
    ck(what, tape.on && idle && idle - tail <= TAPE_QUIET + 2u && !tape.env && !tape.tlp[0] && !tape.tlp[1] &&
       prf_cur.fxrest > rest0 + 10u);
    tape_reset();
}

/* ---- 5. stored ---- */
static void test_store(void)
{
    static project_store_t s0, s1, s2;
    project_t a, c;
    uint32_t n0, n1, i, k;
    int ok;
    char what[180];
    ui_power_on();
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 100};
    song.g[G_PLAYS1 + 1] = 2;                           /* (a 'P' 'L' block: TAPE's after it) */
    tape_reset();
    project_capture(&a);
    n0 = proj_packed_size(&a);
    ok = proj_pack(&s0, &a) && !proj_tp_used(&a);
    for (i = n0; i < PROJC_PFX_OFF; i++)
        ok &= s0.raw[i] == 0u;
    ok &= s0.raw[n0 - 2u - PROJ_PL_N] == 'P' && s0.raw[n0 - 1u - PROJ_PL_N] == 'L';
    ck("TAPE all 0: packed as before (no 'T' 'P', 'P' 'L' the last block, the size)", ok);
    song.g[G_TWOW] = 40; song.g[G_TTONE] = 70;
    project_capture(&a);
    n1 = proj_packed_size(&a);
    ok = proj_pack(&s1, &a) && n1 == n0 + 2u + PROJ_TP_N && !memcmp(s0.raw, s1.raw, n0) && s1.raw[n0] == 'T' &&
         s1.raw[n0 + 1u] == 'P';
    ok &= s1.raw[n0 + 2u] == 0u && s1.raw[n0 + 3u] == 40u && s1.raw[n0 + 7u] == 70u && s1.raw[n0 + 8u] == 0u;
    for (i = n1; i < PROJC_PFX_OFF; i++)
        ok &= s1.raw[i] == 0u;
    tape_reset();
    ok &= proj_import(&c, &s1, sizeof s1) && !project_restore_runtime(&c);
    for (k = G_TAGE; k <= G_TTONE; k++)
        ok &= song.g[k] == (k == G_TWOW ? 40 : k == G_TTONE ? 70 : 0);
    ok &= proj_pack(&s2, &c) && !memcmp(&s1, &s2, sizeof s1);
    ck("WOW 40, TONE 70: 'T' 'P' the last block (after 'P' 'L'), six bytes and a count; the round trip byte for byte", ok);
    song.g[G_PLAYS1 + 1] = 0;
    song.g[G_MRSEL] = 1;                                /* (an 'M' 'R' block, no 'P' 'L') */
    song.g[G_TFLUT] = 9;
    project_capture(&a);
    n0 = proj_packed_size(&a);
    ok = proj_pack(&s1, &a) && s1.raw[n0 - 2u - PROJ_TP_N] == 'T' && s1.raw[n0 - 4u - PROJ_TP_N - PROJ_MR_N] == 'M';
    ck("no PLAYS: 'T' 'P' right after 'M' 'R'", ok && s1.raw[n0 - 3u - PROJ_TP_N - PROJ_MR_N] == 'R');
    song.g[G_MRSEL] = 0;
    for (k = G_TAGE; k <= G_TTONE; k++)
        song.g[k] = 33;
    ok = proj_import(&c, &s0, sizeof s0) && !project_restore_runtime(&c);
    for (k = G_TAGE; k <= G_TTONE; k++)
        ok &= c.g[k] == 0 && song.g[k] == 0;
    ck("a project without 'T' 'P' (0.7's): TAPE all 0, whatever the song held", ok);
    {
        uint32_t sum;
        song.g[G_TWOW] = 40; song.g[G_TTONE] = 70;
        project_capture(&a);
        n1 = proj_packed_size(&a);
        ok = proj_pack(&s1, &a) && s1.raw[n1 - 2u - PROJ_TP_N + 2u + 5u] == 70u;
        s1.raw[n1 - 2u] = 255u;                         /* (TONE: a value no firmware wrote) */
        sum = proj_hash(s1.raw, PROJ_STORE_SIZE - 4u);
        memcpy(s1.raw + PROJ_STORE_SIZE - 4u, &sum, 4);
        ok &= proj_import(&c, &s1, sizeof s1) && !project_restore_runtime(&c) && song.g[G_TTONE] == 100 && song.g[G_TWOW] == 40;
        snprintf(what, sizeof what, "a stored 255: clamped (TONE %d)", song.g[G_TTONE]);
        ck(what, ok);
    }
    for (k = G_TAGE; k <= G_TTONE; k++)
        song.g[k] = (int16_t)(k - G_TAGE + 10u);
    system_init();
    for (ok = 1, k = G_TAGE; k <= G_TTONE; k++)
        ok &= song.g[k] == 0;
    ck("NEW PROJECT (system_init): TAPE all 0", ok);
    song.g[G_TSAT] = 60; song.g[G_TAGE] = 50;
    undo_mark();
    song.g[G_TSAT] = 0; song.g[G_TAGE] = 0;
    ok = !undo_do() && song.g[G_TSAT] == 60 && song.g[G_TAGE] == 50;
    ck("UNDO brings TAPE back (SAT 60, AGE 50)", ok);
    ui_power_on();
    tape_reset();
}

int main(void)
{
    test_off();
    test_tone_through();
    test_in_out();
    test_rest();
    test_store();
    printf(bad ? "tape: %d FAILED\n" : "tape: all checks ok\n", bad);
    return bad != 0;
}
